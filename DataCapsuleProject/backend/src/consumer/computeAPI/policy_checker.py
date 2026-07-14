# policy_checker.py
import json
import pandas as pd
from typing import Dict, List, Optional, Tuple
from policy_shadow import build_shadow_table, load_policy_from_dict, TablePolicy
from sql_parser import parse_sql
from sqlglot import parse, parse_one
import sqlglot.expressions as exp
from sqlglot.expressions import (
    EQ, NEQ, GT, LT, GTE, LTE, And, Or, Column, Literal
)
from sql_executor import execute_sql_with_sqlite
from io import StringIO


ALLOWED_ACCESS = {"read", "agg", "full", "deny"}
ALLOWED_LABEL_KEYS = {
    "access", "agg", "distinct", "order_by", "limit_with_order"
}
ALLOWED_AGGS = {"count", "sum", "avg", "min", "max"}
EXTRA_PERMISSIONS = {"distinct", "order_by", "limit_with_order"}


def expr_to_mask(expr, df: pd.DataFrame) -> pd.Series:
    """将受支持的 WHERE 条件转换为与 df 对齐的布尔 mask。"""

    def as_bool_series(value) -> pd.Series:
        if isinstance(value, pd.Series):
            return value.fillna(False).astype(bool)
        if isinstance(value, bool):
            return pd.Series(value, index=df.index, dtype=bool)
        raise ValueError("WHERE 条件没有产生布尔结果")

    def literal_value(node: Literal):
        if node.is_string:
            return node.this
        raw = str(node.this)
        lowered = raw.lower()
        if lowered == "true":
            return True
        if lowered == "false":
            return False
        try:
            return int(raw)
        except ValueError:
            try:
                return float(raw)
            except ValueError as exc:
                raise ValueError(f"不支持的字面量: {raw}") from exc

    def evaluate(node):
        if isinstance(node, exp.Paren):
            return evaluate(node.this)
        if isinstance(node, Column):
            column = node.name
            if column not in df.columns:
                raise ValueError(f"WHERE 引用了不存在的列 {column}")
            return df[column]
        if isinstance(node, Literal):
            return literal_value(node)
        if isinstance(node, And):
            return as_bool_series(evaluate(node.this)) & as_bool_series(
                evaluate(node.expression)
            )
        if isinstance(node, Or):
            return as_bool_series(evaluate(node.this)) | as_bool_series(
                evaluate(node.expression)
            )
        if isinstance(node, exp.Not):
            return ~as_bool_series(evaluate(node.this))
        if isinstance(node, (EQ, NEQ, GT, LT, GTE, LTE)):
            left = evaluate(node.this)
            right = evaluate(node.expression)
            if isinstance(node, EQ):
                return left == right
            if isinstance(node, NEQ):
                return left != right
            if isinstance(node, GT):
                return left > right
            if isinstance(node, LT):
                return left < right
            if isinstance(node, GTE):
                return left >= right
            return left <= right
        raise ValueError(f"不支持的 WHERE 表达式: {node.sql()}")

    return as_bool_series(evaluate(expr))


def label_contains(label: str, keyword: str) -> bool:
    """精确检查标签，不使用可能把 distinct:deny 当成主 deny 的子串匹配。"""
    parts = label_get_parts(label)
    keyword = keyword.strip().lower()
    access = set(parts.get("access", []))
    if "deny" in access:
        return keyword == "deny"
    if keyword in access:
        return True
    if keyword == "agg" and "agg" in parts:
        return True
    if keyword in EXTRA_PERMISSIONS:
        values = set(parts.get(keyword, []))
        if "deny" in values:
            return False
        return "allow" in values or "full" in access
    return "full" in access

def label_get_parts(label: str) -> Dict[str, List[str]]:
    result: Dict[str, List[str]] = {}
    if not isinstance(label, str):
        return {"__invalid__": ["标签不是字符串"]}
    for raw_part in label.split("|"):
        part = raw_part.strip()
        if not part:
            result.setdefault("__invalid__", []).append("存在空标签段")
            continue
        if ":" in part:
            k, v = part.split(":", 1)
            key = k.strip().lower()
            values = [item.strip().lower() for item in v.split(",") if item.strip()]
            if not key or not values:
                result.setdefault("__invalid__", []).append(part)
            else:
                result.setdefault(key, []).extend(values)
        else:
            result.setdefault("access", []).append(part.lower())
    return result


def _label_state(label: str) -> Tuple[Dict[str, List[str]], Optional[str], Optional[str]]:
    """返回标签各段、主权限和错误；未知或矛盾标签按 fail-closed 处理。"""
    parts = label_get_parts(label)
    if "__invalid__" in parts:
        return parts, None, "标签格式无效"

    unknown_keys = set(parts) - ALLOWED_LABEL_KEYS
    if unknown_keys:
        return parts, None, f"包含未知标签键 {sorted(unknown_keys)}"

    access_values = set(parts.get("access", []))
    if "deny" in access_values:
        return parts, "deny", None
    unknown_access = access_values - ALLOWED_ACCESS
    if unknown_access:
        return parts, None, f"包含未知主权限 {sorted(unknown_access)}"

    non_deny_access = access_values - {"deny"}
    if len(non_deny_access) > 1:
        return parts, None, f"存在冲突的主权限 {sorted(non_deny_access)}"
    if non_deny_access:
        access = next(iter(non_deny_access))
    elif "agg" in parts:
        # policy_shadow 使用 agg:count 表示 agg 主权限及其白名单。
        access = "agg"
    else:
        return parts, None, "缺少主权限"

    if "agg" in parts:
        unknown_aggs = set(parts["agg"]) - ALLOWED_AGGS
        if unknown_aggs:
            return parts, None, f"包含未知聚合函数 {sorted(unknown_aggs)}"

    for key in EXTRA_PERMISSIONS:
        values = set(parts.get(key, []))
        unknown_values = values - {"allow", "deny"}
        if unknown_values:
            return parts, None, f"{key} 包含未知权限值 {sorted(unknown_values)}"

    return parts, access, None


def _column_labels(
    shadow: pd.DataFrame,
    column: str,
    row_mask: Optional[pd.Series] = None,
) -> Tuple[List[Tuple[object, str]], Optional[str]]:
    if not isinstance(column, str) or not column:
        return [], "字段名缺失"
    if column not in shadow.columns:
        return [], f"字段 {column} 不存在"

    series = shadow[column]
    if row_mask is not None and bool(row_mask.any()):
        series = series.loc[row_mask]
    if series.empty:
        return [], f"字段 {column} 在影子表中没有可校验的权限标签"
    return list(series.items()), None


def _check_main_access(
    shadow: pd.DataFrame,
    column: str,
    position: str,
    allow_agg: bool,
    row_mask: Optional[pd.Series] = None,
) -> List[str]:
    labels, error = _column_labels(shadow, column, row_mask)
    if error:
        return [f"{position}: {error}"]
    for row_index, label in labels:
        _, access, label_error = _label_state(label)
        if label_error:
            return [
                f"{position}: 字段 {column} 第 {row_index} 行标签无效: "
                f"{label_error}（标签: {label}）"
            ]
        if access == "deny":
            return [f"{position}: 字段 {column} 被 deny 策略拒绝"]
        if access in {"read", "full"}:
            continue
        if allow_agg and access == "agg":
            continue
        return [
            f"{position}: 字段 {column} 非聚合访问权限不足"
            f"（当前权限: {label}）"
        ]
    return []


def _check_extra_permission(
    shadow: pd.DataFrame,
    column: str,
    position: str,
    permission: str,
) -> List[str]:
    labels, error = _column_labels(shadow, column)
    if error:
        return [f"{position}: {error}"]
    for row_index, label in labels:
        parts, access, label_error = _label_state(label)
        if label_error:
            return [
                f"{position}: 字段 {column} 第 {row_index} 行标签无效: "
                f"{label_error}（标签: {label}）"
            ]
        if access == "deny":
            return [f"{position}: 字段 {column} 被 deny 策略拒绝"]
        values = set(parts.get(permission, []))
        if "deny" in values:
            return [f"{position}: 字段 {column} 的 {permission} 权限为 deny"]
        if "allow" not in values and access != "full":
            return [f"{position}: 字段 {column} 未明确允许 {permission}"]
    return []


def _selected_columns(parsed_sql: Dict, shadow: pd.DataFrame) -> Tuple[List[str], List[str]]:
    columns: List[str] = []
    errors: List[str] = []
    select_items = parsed_sql.get("select")
    if not isinstance(select_items, list) or not select_items:
        return [], ["SELECT: 缺少可校验的选择项"]

    for item in select_items:
        if not isinstance(item, dict):
            errors.append("SELECT: 解析结果中的选择项格式无效")
            continue
        item_type = item.get("type")
        if item_type == "column":
            columns.append(item.get("column"))
        elif item_type in {"aggregate", "expression"}:
            raw = str(item.get("raw", "")).strip()
            item_columns = item.get("columns")
            if item_type == "expression" and raw in {"*"}:
                columns.extend(str(col) for col in shadow.columns)
            elif isinstance(item_columns, list):
                columns.extend(item_columns)
            else:
                errors.append(f"SELECT: {item_type} 缺少字段列表")
        else:
            errors.append(f"SELECT: 未知选择项类型 {item_type}")

    valid_columns = [column for column in columns if isinstance(column, str) and column]
    if len(valid_columns) != len(columns):
        errors.append("SELECT: 存在空字段名")
    return list(dict.fromkeys(valid_columns)), errors


def _select_row_mask(parsed_sql: Dict, df: pd.DataFrame) -> Optional[pd.Series]:
    where_info = parsed_sql.get("where")
    if where_info is None:
        return None
    if not isinstance(where_info, dict) or not isinstance(where_info.get("condition"), str):
        raise ValueError("WHERE 解析结构缺少 condition")
    condition = parse_one(where_info["condition"])
    return expr_to_mask(condition, df)

def is_aggregate_allowed(label: str, func: str) -> bool:
    """判断影子表标签是否允许对该列使用指定聚合函数。"""
    parts, access, label_error = _label_state(label)
    if label_error or access == "deny":
        return False
    func_lower = func.lower()
    if func_lower not in ALLOWED_AGGS:
        return False
    if access in {"full", "read"}:
        # read：允许聚合；若存在 agg 段则按白名单收紧
        if "agg" in parts:
            return func_lower in parts["agg"]
        return True
    if access == "agg":
        return func_lower in parts.get("agg", [])
    return False

def check_select_clause(shadow: pd.DataFrame, parsed_sql: Dict, df: pd.DataFrame) -> List[str]:
    """检查 SELECT 子句（含聚合、表达式等）是否符合策略。返回违规说明列表，合规则返回 []。"""
    errors: List[str] = []
    try:
        row_mask = _select_row_mask(parsed_sql, df)
    except Exception as exc:
        return [f"SELECT: 无法确定查询涉及的行: {exc}"]

    select_items = parsed_sql.get("select")
    if not isinstance(select_items, list) or not select_items:
        return ["SELECT: 缺少可校验的选择项"]

    for item in select_items:
        if not isinstance(item, dict):
            errors.append("SELECT: 解析结果中的选择项格式无效")
            continue
        item_type = item.get("type")
        if item_type == "column":
            errors += _check_main_access(
                shadow, item.get("column"), "SELECT", False, row_mask
            )
        elif item_type == "expression":
            raw = str(item.get("raw", "")).strip()
            columns = item.get("columns")
            if raw == "*":
                columns = list(shadow.columns)
            if not isinstance(columns, list) or not columns:
                errors.append(f"SELECT: 无法安全校验表达式 {raw or '<unknown>'}")
                continue
            for column in columns:
                errors += _check_main_access(
                    shadow, column, f"SELECT 表达式 {raw}", False, row_mask
                )
        elif item_type == "aggregate":
            func = item.get("func")
            columns = item.get("columns")
            if not isinstance(func, str) or not isinstance(columns, list) or not columns:
                errors.append("SELECT 聚合: 聚合函数或字段缺失，COUNT(*) 未获显式授权")
                continue
            for column in columns:
                labels, label_error = _column_labels(shadow, column, row_mask)
                if label_error:
                    errors.append(f"SELECT 聚合: {label_error}")
                    continue
                for row_index, label in labels:
                    _, access, state_error = _label_state(label)
                    if state_error:
                        errors.append(
                            f"SELECT 聚合: 字段 {column} 第 {row_index} 行标签无效: "
                            f"{state_error}（标签: {label}）"
                        )
                        break
                    if access == "deny":
                        errors.append(f"SELECT 聚合: 字段 {column} 被 deny 策略拒绝")
                        break
                    if not is_aggregate_allowed(label, func):
                        errors.append(
                            f"SELECT 聚合: 字段 {column} 不允许使用 "
                            f"{func.upper()}（当前权限: {label}）"
                        )
                        break
        else:
            errors.append(f"SELECT: 未知选择项类型 {item_type}")
    return errors


def check_where_clause(shadow: pd.DataFrame, parsed_sql: Dict, df: pd.DataFrame) -> List[str]:
    """检查 WHERE 子句是否符合策略。"""
    where_info = parsed_sql.get("where")
    if where_info is None:
        return []
    if not isinstance(where_info, dict) or not isinstance(where_info.get("columns"), list):
        return ["WHERE: 解析结果格式无效"]

    errors: List[str] = []
    for column in dict.fromkeys(where_info["columns"]):
        errors += _check_main_access(shadow, column, "WHERE", True)
    return errors


def check_order_by_clause(shadow: pd.DataFrame, parsed_sql: Dict) -> List[str]:
    """检查 ORDER BY 子句是否符合策略。"""
    order_items = parsed_sql.get("order_by", [])
    if not isinstance(order_items, list):
        return ["ORDER BY: 解析结果格式无效"]

    errors: List[str] = []
    for item in order_items:
        if not isinstance(item, dict):
            errors.append("ORDER BY: 排序项格式无效")
            continue
        column = item.get("column")
        direction = item.get("direction")
        if direction not in {"asc", "desc"}:
            errors.append(f"ORDER BY: 字段 {column} 的排序方向无效")
            continue
        errors += _check_main_access(shadow, column, "ORDER BY", True)
        errors += _check_extra_permission(
            shadow, column, "ORDER BY", "order_by"
        )
    return errors


def check_limit_combination(shadow: pd.DataFrame, parsed_sql: Dict) -> List[str]:
    """检查 LIMIT 与 ORDER BY 组合相关策略。"""
    limit_value = parsed_sql.get("limit")
    order_items = parsed_sql.get("order_by", [])
    if limit_value is None or not order_items:
        return []
    if not isinstance(limit_value, int) or isinstance(limit_value, bool) or limit_value < 0:
        return ["LIMIT: LIMIT 必须是非负整数"]
    if not isinstance(order_items, list):
        return ["LIMIT + ORDER BY: 排序项格式无效"]

    errors: List[str] = []
    for item in order_items:
        if not isinstance(item, dict):
            errors.append("LIMIT + ORDER BY: 排序项格式无效")
            continue
        errors += _check_extra_permission(
            shadow,
            item.get("column"),
            "LIMIT + ORDER BY",
            "limit_with_order",
        )
    return errors


def check_distinct(parsed_sql: Dict, shadow: pd.DataFrame) -> List[str]:
    """检查 DISTINCT 相关策略。"""
    if not parsed_sql.get("distinct", False):
        return []
    columns, errors = _selected_columns(parsed_sql, shadow)
    if not columns:
        errors.append("DISTINCT: 没有可映射到影子表的字段")
        return errors
    for column in columns:
        errors += _check_extra_permission(
            shadow, column, "DISTINCT", "distinct"
        )
    return errors


def check_group_by(parsed_sql: Dict, shadow: pd.DataFrame) -> List[str]:
    """检查 GROUP BY 子句是否符合策略。"""
    columns = parsed_sql.get("group_by", [])
    if not isinstance(columns, list):
        return ["GROUP BY: 解析结果格式无效"]

    errors: List[str] = []
    for column in dict.fromkeys(columns):
        # GROUP BY 会暴露分组键，只允许 read/full，不把 agg 当成读取授权。
        errors += _check_main_access(shadow, column, "GROUP BY", False)
    return errors


def check_op_rules(policy: TablePolicy, parsed_sql: Dict) -> List[str]:
    """检查 join / 子查询 / 窗口函数等操作是否被 op_rules 禁止。policy 来自 load_policy_from_dict。"""
    operations = parsed_sql.get("operations", [])
    if not isinstance(operations, list):
        return ["操作规则: SQL 操作列表格式无效"]
    if not isinstance(policy.op_rules, dict):
        return ["操作规则: 策略中的 op_rules 格式无效"]

    supported = {"join", "nested_query", "window_functions"}
    errors: List[str] = []
    for operation in dict.fromkeys(operations):
        if operation not in supported:
            errors.append(f"操作规则: 未知 SQL 操作 {operation}")
            continue
        setting = policy.op_rules.get(operation)
        if not isinstance(setting, str) or setting.lower() != "allow":
            errors.append(
                f"操作规则: {operation} 未被策略明确允许"
                f"（当前值: {setting if setting is not None else 'missing'}）"
            )
    return errors


def _parse_and_enrich_sql(sql: str) -> Dict:
    if not isinstance(sql, str) or not sql.strip():
        raise ValueError("SQL 为空")
    statements = [statement for statement in parse(sql) if statement is not None]
    if len(statements) != 1:
        raise ValueError("只允许提交一条 SQL 语句")

    tree = statements[0]
    if not isinstance(tree, exp.Select):
        raise ValueError("只允许 SELECT 查询")

    parsed_sql = parse_sql(sql)
    if not isinstance(parsed_sql, dict):
        raise ValueError("SQL 解析器返回了无效结构")

    from_clause = tree.args.get("from") or tree.args.get("from_")
    table_expr = getattr(from_clause, "this", None) if from_clause else None
    if table_expr is None and from_clause is not None:
        expressions = getattr(from_clause, "expressions", None) or []
        table_expr = expressions[0] if expressions else None
    parsed_sql["table"] = table_expr.name if isinstance(table_expr, exp.Table) else None

    limit_expr = tree.args.get("limit")
    if limit_expr is not None:
        literal = getattr(limit_expr, "expression", None) or getattr(limit_expr, "this", None)
        if not isinstance(literal, Literal) or literal.is_string:
            raise ValueError("LIMIT 必须是整数常量")
        limit_value = int(literal.this)
        if limit_value < 0:
            raise ValueError("LIMIT 必须是非负整数")
        parsed_sql["limit"] = limit_value
    if tree.args.get("offset") is not None:
        raise ValueError("当前策略检查不支持 OFFSET")

    # 同时覆盖 SELECT DISTINCT 和 COUNT(DISTINCT column) 等形式。
    if tree.find(exp.Distinct):
        parsed_sql["distinct"] = True
    return parsed_sql


def check_sql_policy(sql: str, csv_str: str, policy_str: str) -> dict:
    try:
        parsed = _parse_and_enrich_sql(sql)
    except Exception as e:
        return {
            "violation": True,
            "errors": [f"SQL解析失败: {str(e)}"],
            "result": None
        }

    # 将 JSON 字符串反序列化为 dict
    try:
        policy_dict = json.loads(policy_str)
    except Exception as e:
        return {
            "violation": True,
            "errors": [f"策略 JSON 解析失败: {str(e)}"],
            "result": None
        }

    # 解析 CSV 数据
    try:
        df = pd.read_csv(StringIO(csv_str))
    except Exception as e:
        return {
            "violation": True,
            "errors": [f"CSV解析失败: {str(e)}"],
            "result": None
        }

    try:
        policy = load_policy_from_dict(policy_dict)    #从 program_policy["SQLOperation"] 建 TablePolicy
        shadow = build_shadow_table(df, policy)        # 从行/列规则构建影子表
    except Exception as e:
        return {
            "violation": True,
            "errors": [f"策略或影子表构建失败: {str(e)}"],
            "result": None
        }

    # 根据影子表检查各子句是否符合策略
    errors = []
    parsed_table = parsed.get("table")
    policy_table = policy.table
    if not isinstance(parsed_table, str) or not isinstance(policy_table, str):
        errors.append("FROM: 查询表或策略表名缺失")
    elif parsed_table.lower() != policy_table.lower():
        errors.append(
            f"FROM: 查询表 {parsed_table} 与策略允许的表 {policy_table} 不一致"
        )

    try:
        errors += check_select_clause(shadow, parsed, df) or []
        errors += check_where_clause(shadow, parsed, df) or []
        errors += check_order_by_clause(shadow, parsed) or []
        errors += check_limit_combination(shadow, parsed) or []
        errors += check_distinct(parsed, shadow) or []
        errors += check_group_by(parsed, shadow) or []
        errors += check_op_rules(policy, parsed) or []
    except Exception as e:
        errors.append(f"策略检查异常（已拒绝执行）: {str(e)}")

    errors = [str(e) for e in errors]

    if errors:
        return {
            "violation": True,
            "errors": errors,
            "result": None
        }
    try:
        result_df = execute_sql_with_sqlite(sql, csv_str)      # 检查通过后内存 SQLite 执行 SQL
        result_records = result_df.to_dict(orient="records")
        return {
            "violation": False,
            "errors": [],
            "result": result_records,
        }
    except Exception as e:
        return {
            "violation": True,
            "errors": [f"SQL执行异常: {str(e)}"],
            "result": None,
        }

# if __name__ == "__main__":
#     sql = """
#     SELECT DISTINCT name, salary + bonus, COUNT(*) FROM users
#     WHERE region = 'EU'
#     GROUP BY name
#     ORDER BY salary DESC
#     LIMIT 10
#     """
#     check_query(sql, "data/users.csv", "policy/users_policy.json")
