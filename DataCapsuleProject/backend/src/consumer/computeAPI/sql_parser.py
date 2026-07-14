# sql_parser.py

import sqlglot
from sqlglot import parse_one
import sqlglot.expressions as exp

def extract_columns(expr: exp.Expression):
    """
    提取表达式中涉及的列名
    """
    return [e.name for e in expr.find_all(exp.Column)]

def parse_sql(sql: str) -> dict:
    """
    将 SQL 查询结构化，便于策略检查。
    支持解析：
    - FROM 表名
    - SELECT 字段 / 聚合函数 / 表达式，区分类型
    - DISTINCT 标识
    - WHERE 子句字段及条件
    - GROUP BY 字段
    - ORDER BY 字段及排序方向
    - LIMIT 限制条数
    - 操作规则（join, nested_query, window_functions）简单标记
    """
    parsed = parse_one(sql)

    result = {
        "table": None,
        "select": [],
        "distinct": False,
        "where": None,
        "group_by": [],
        "order_by": [],
        "limit": None,
        "operations": [],
    }

    # 解析 FROM 子句
    from_ = parsed.args.get("from")
    if from_ and from_.expressions:
        table_expr = from_.expressions[0]
        if isinstance(table_expr, exp.Table):
            result["table"] = table_expr.name
        elif hasattr(table_expr, "this") and hasattr(table_expr.this, "name"):
            result["table"] = table_expr.this.name

    # 解析 DISTINCT
    result["distinct"] = parsed.args.get("distinct") is not None

    # 解析 SELECT 项
    for proj in parsed.expressions:
        func_expr = proj.find(exp.Func)
        if func_expr:
            func_name = func_expr.sql_name().lower()
            columns = extract_columns(func_expr)
            result["select"].append({
                "type": "aggregate",
                "func": func_name,
                "columns": columns
            })
        elif isinstance(proj, exp.Column):
            result["select"].append({
                "type": "column",
                "column": proj.name
            })
        else:
            # 复杂表达式（算术、函数嵌套等）
            result["select"].append({
                "type": "expression",
                "columns": extract_columns(proj),
                "raw": proj.sql()
            })

    # # WHERE
    # where_expr = parsed.args.get("where")
    # if where_expr:
    #     result["where"] = {
    #         "columns": extract_columns(where_expr),
    #         "condition": where_expr.sql()
    #     }
    # WHERE
    where_expr = parsed.args.get("where")
    if where_expr:
        condition_expr = where_expr.args["this"]
        result["where"] = {
            "columns": extract_columns(condition_expr),
            "condition": condition_expr.sql()  # 不含 WHERE 关键字
        }

    # GROUP BY
    group_exprs = parsed.args.get("group")
    if group_exprs:
        result["group_by"] = [
            g.name for g in group_exprs.expressions if isinstance(g, exp.Column)
        ]

    # ORDER BY
    order_exprs = parsed.args.get("order")
    if order_exprs:
        result["order_by"] = []
        for o in order_exprs.expressions:
            col = o.this
            result["order_by"].append({
                "column": col.name if isinstance(col, exp.Column) else col.sql(),
                "direction": "desc" if o.args.get("desc") else "asc"
            })

    # LIMIT
    limit_expr = parsed.args.get("limit")
    if limit_expr and isinstance(limit_expr.this, exp.Literal):
        result["limit"] = int(limit_expr.this.name)

    # 解析操作规则: join, nested_query, window_functions
    if parsed.find(exp.Join):
        result["operations"].append("join")
    if parsed.find(exp.Subquery):
        result["operations"].append("nested_query")
    if parsed.find(exp.Window):
        result["operations"].append("window_functions")

    return result

if __name__ == "__main__":
    sql = """
    SELECT DISTINCT name, salary + bonus, COUNT(*) FROM users
    WHERE region = 'EU'
    GROUP BY name
    ORDER BY salary DESC
    LIMIT 10
    """
    parsed_result = parse_sql(sql)
    import pprint
    pprint.pprint(parsed_result)
