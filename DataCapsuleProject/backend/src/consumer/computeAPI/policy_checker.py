# policy_checker.py
import json
import pandas as pd
from typing import Dict, List
from policy_shadow import build_shadow_table, load_policy_from_dict, TablePolicy
from sql_parser import parse_sql
from sqlglot import parse_one
from sqlglot.expressions import (
    EQ, NEQ, GT, LT, GTE, LTE, And, Or, Column, Literal
)
from sql_executor import execute_sql_with_sqlite
from io import StringIO

def expr_to_mask(expr, df: pd.DataFrame) -> pd.Series:
    """可选实现：将 WHERE 条件转为行级布尔 mask。实现方式自定，也可不使用本函数。"""
    
    
    raise NotImplementedError


def label_contains(label: str, keyword: str) -> bool:
    # 如果有 full，视为拥有所有权限
    parts = [part.strip() for part in label.split("|")]
    if "full" in parts:
        return True
    return any(part == keyword or part.startswith(keyword + ":") for part in parts)

def label_get_parts(label: str) -> Dict[str, List[str]]:
    result = {}
    for part in label.split("|"):
        if ":" in part:
            k, v = part.split(":", 1)
            result.setdefault(k.strip(), []).extend(v.strip().split(","))
        else:
            result.setdefault("access", []).append(part.strip())
    return result

def is_aggregate_allowed(label: str, func: str) -> bool:
    """判断影子表标签是否允许对该列使用指定聚合函数。"""
    parts = label_get_parts(label)
    if "deny" in parts.get("access", []):
        return False
    func_lower = func.lower()
    if "full" in parts.get("access", []) or "read" in parts.get("access", []):
        # read：允许聚合；若存在 agg 段则按白名单收紧
        if "agg" in parts:
            return func_lower in [a.lower() for a in parts["agg"]]
        return True
    if "agg" in parts.get("access", []) or "agg" in parts:
        aggs = parts.get("agg", [])
        return func_lower in [a.lower() for a in aggs]
    return False

def check_select_clause(shadow: pd.DataFrame, parsed_sql: Dict, df: pd.DataFrame) -> List[str]:
    """检查 SELECT 子句（含聚合、表达式等）是否符合策略。返回违规说明列表，合规则返回 []。"""



    return []


def check_where_clause(shadow: pd.DataFrame, parsed_sql: Dict, df: pd.DataFrame) -> List[str]:
    """检查 WHERE 子句是否符合策略。"""
    
    
    
    return []


def check_order_by_clause(shadow: pd.DataFrame, parsed_sql: Dict) -> List[str]:
    """检查 ORDER BY 子句是否符合策略。"""
    


    return []


def check_limit_combination(shadow: pd.DataFrame, parsed_sql: Dict) -> List[str]:
    """检查 LIMIT 与 ORDER BY 组合相关策略。"""
    
    

    return []


def check_distinct(parsed_sql: Dict, shadow: pd.DataFrame) -> List[str]:
    """检查 DISTINCT 相关策略。"""
    
    

    return []


def check_group_by(parsed_sql: Dict, shadow: pd.DataFrame) -> List[str]:
    """检查 GROUP BY 子句是否符合策略。"""
    
    

    return []


def check_op_rules(policy: TablePolicy, parsed_sql: Dict) -> List[str]:
    """检查 join / 子查询 / 窗口函数等操作是否被 op_rules 禁止。policy 来自 load_policy_from_dict。"""
    
    
    
    return []


def check_sql_policy(sql: str, csv_str: str, policy_str: str) -> dict:
    parsed = parse_sql(sql)         # sqlglot 解析 SQL → dict
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

    policy = load_policy_from_dict(policy_dict)    #从 program_policy["SQLOperation"] 建 TablePolicy
    shadow = build_shadow_table(df, policy)        # 从行/列规则构建影子表

    # 根据影子表检查各子句是否符合策略
    errors = []
    errors += check_select_clause(shadow, parsed, df) or []
    errors += check_where_clause(shadow, parsed, df) or []
    errors += check_order_by_clause(shadow, parsed) or []
    errors += check_limit_combination(shadow, parsed) or []
    errors += check_distinct(parsed, shadow) or []
    errors += check_group_by(parsed, shadow) or []
    errors += check_op_rules(policy, parsed) or []

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
