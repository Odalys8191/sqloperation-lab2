# sql_executor.py
import sqlite3
import pandas as pd
from io import StringIO

def execute_sql_with_sqlite(sql: str, csv_str: str) -> pd.DataFrame:
    # 用 StringIO 把字符串包装成类文件对象
    df = pd.read_csv(StringIO(csv_str))
    conn = sqlite3.connect(":memory:")
    df.to_sql("users", conn, index=False, if_exists="replace")
    cursor = conn.cursor()
    cursor.execute(sql)
    rows = cursor.fetchall()
    columns = [desc[0] for desc in cursor.description]
    conn.close()
    result_df = pd.DataFrame(rows, columns=columns)
    return result_df
