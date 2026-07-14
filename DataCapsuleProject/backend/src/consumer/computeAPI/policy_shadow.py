# policy_shadow.py
import json
import pandas as pd
from dataclasses import dataclass
from typing import List, Dict, Optional, Literal, Any

AccessType = Literal["none", "read", "agg", "full"]

@dataclass
class ColumnRule:
    access: AccessType
    aggs: Optional[List[str]] = None
    expressions: Optional[List[Dict[str, Any]]] = None
    distinct: Optional[str] = None
    order_by: Optional[str] = None
    limit_with_order: Optional[str] = None

@dataclass
class TablePolicy:
    table: str
    default_access: str
    row_rules: List[Dict[str, str]]
    column_rules: Dict[str, ColumnRule]
    op_rules: Dict[str, str]

# def load_policy_from_dict(obj: dict) -> TablePolicy:
#     col_rules = {}
#     for col, rule in obj["rules"]["column_rules"].items():
#         col_rules[col] = ColumnRule(
#             access=rule["access"],
#             aggs=rule.get("aggs"),
#             expressions=rule.get("expressions"),
#             distinct=rule.get("distinct"),
#             order_by=rule.get("order_by"),
#             limit_with_order=rule.get("limit_with_order")
#         )

#     return TablePolicy(
#         table=obj["table"],
#         default_access=obj["default_access"],
#         row_rules=obj["rules"]["row_rules"],
#         column_rules=col_rules,
#         op_rules=obj["rules"].get("op_rules", {})
#     )
def load_policy_from_dict(obj: dict) -> TablePolicy:
    if "SQLOperation" not in obj:
        raise ValueError("不是SQL操作，分支错误")

    operation_policy = obj["SQLOperation"]

    col_rules = {}
    for col, rule in operation_policy["rules"]["column_rules"].items():
        col_rules[col] = ColumnRule(
            access=rule["access"],
            aggs=rule.get("aggs"),
            expressions=rule.get("expressions"),
            distinct=rule.get("distinct"),
            order_by=rule.get("order_by"),
            limit_with_order=rule.get("limit_with_order")
        )

    return TablePolicy(
        table=operation_policy["table"],
        default_access=operation_policy["default_access"],
        row_rules=operation_policy["rules"]["row_rules"],
        column_rules=col_rules,
        op_rules=operation_policy["rules"].get("op_rules", {})
    )

def load_policy_from_file(path: str) -> TablePolicy:
    with open(path, 'r') as f:
        obj = json.load(f)
    return load_policy_from_dict(obj)

def normalize_access_label(access: str) -> str:
    return "deny" if access in ("none", "deny") else access

def build_shadow_table(df: pd.DataFrame, policy: TablePolicy) -> pd.DataFrame:
    shadow = pd.DataFrame(index=df.index, columns=df.columns)
    row_mask = ['allow'] * len(df)

    for i, row in df.iterrows():
        for rule in policy.row_rules:
            try:
                context = row.to_dict()
                if eval(rule["condition"], {}, context):
                    if normalize_access_label(rule["access"]) == "deny":
                        row_mask[i] = "deny"
                        break
            except Exception:
                row_mask[i] = "error"

    for col in df.columns:
        col_rule = policy.column_rules.get(col)
        for i in df.index:
            # 基础单元格访问来自行规则（row_mask）或列默认
            if row_mask[i] == "deny":
                cell_label = "deny"
            elif row_mask[i] == "error":
                cell_label = "error"
            elif not col_rule:
                cell_label = normalize_access_label(policy.default_access)
            else:
                access = normalize_access_label(col_rule.access)
                if access == "agg" and col_rule.aggs:
                    cell_label = f"agg:{','.join(col_rule.aggs)}"
                elif access == "read":
                    cell_label = "read"
                    if col_rule.aggs:
                        cell_label = f"read|agg:{','.join(col_rule.aggs)}"
                else:
                    cell_label = access

            # 如果列规则里有按行生效的 expressions，则基于当前行的上下文对单元格做覆盖
            # expressions 是一个 dict 列表，每项可能包含 'condition' 和 'access' (以及可选的 'aggs')
            if col_rule and col_rule.expressions:
                context = df.loc[i].to_dict()
                for expr_rule in col_rule.expressions:
                    try:
                        cond = expr_rule.get("condition")
                        if cond and eval(cond, {}, context):
                            # 符合表达式条件，按 expr_rule 中描述覆盖访问权限
                            expr_access = expr_rule.get("access")
                            if expr_access:
                                expr_access_norm = normalize_access_label(expr_access)
                                if expr_access_norm == "agg":
                                    aggs = expr_rule.get("aggs") or col_rule.aggs
                                    if aggs:
                                        cell_label = f"agg:{','.join(aggs)}"
                                    else:
                                        cell_label = "agg"
                                else:
                                    cell_label = expr_access_norm
                            # 表达式中也可能直接指定 'deny'/'allow' 等，处理后跳出（优先级最高）
                            break
                    except Exception:
                        cell_label = "error"
                        break

            # 将列级别的额外权限标记附加到单元格 label 中，便于检查器读取
            if col_rule:
                # 聚合允许列表（read 已在基础标签中合并 agg 段时跳过）
                if col_rule.aggs and "agg:" not in cell_label and "|agg:" not in cell_label:
                    cell_label = f"{cell_label}|agg:{','.join(col_rule.aggs)}"
                # distinct
                if col_rule.distinct is not None and "distinct:" not in cell_label:
                    cell_label = f"{cell_label}|distinct:{col_rule.distinct}"
                # order_by
                if col_rule.order_by is not None and "order_by:" not in cell_label:
                    cell_label = f"{cell_label}|order_by:{col_rule.order_by}"
                # limit_with_order
                if col_rule.limit_with_order is not None and "limit_with_order:" not in cell_label:
                    cell_label = f"{cell_label}|limit_with_order:{col_rule.limit_with_order}"

            shadow.at[i, col] = cell_label
    return shadow

def print_policy_details(policy: TablePolicy):
    print("=== 策略详情 ===")
    print(f"默认访问权限: {policy.default_access}")
    print("\n行规则:")
    for rule in policy.row_rules:
        print(f"  条件: {rule['condition']}, 访问: {rule['access']}")
    print("\n列规则:")
    for col, rule in policy.column_rules.items():
        print(f"  列: {col}")
        print(f"    访问权限: {rule.access}")
        if rule.aggs:
            print(f"    允许聚合: {rule.aggs}")
        if rule.distinct:
            print(f"    DISTINCT: {rule.distinct}")
        if rule.order_by:
            print(f"    ORDER BY: {rule.order_by}")
        if rule.limit_with_order:
            print(f"    LIMIT with ORDER BY: {rule.limit_with_order}")
        if rule.expressions:
            print(f"    表达式限制:")
            for expr in rule.expressions:
                print(f"      {expr}")
    print("\n操作规则 (op_rules):")
    for op, val in policy.op_rules.items():
        print(f"  {op}: {val}")
    print("================\n")

# if __name__ == "__main__":
#     df = pd.read_csv("data/users.csv")
#     print("=== 原始数据表 ===")
#     print(df)
#     print("================\n")

#     policy = load_policy("policy/users_policy.json")
#     print_policy_details(policy)
#     shadow = build_shadow_table(df, policy)
#     print("=== 生成的影子表 ===")
#     print(shadow)
#     print("================")
