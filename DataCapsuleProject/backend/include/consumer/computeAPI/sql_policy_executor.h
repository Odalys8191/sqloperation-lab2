// sql_policy_executor.h

#ifndef SQL_POLICY_EXECUTOR_H
#define SQL_POLICY_EXECUTOR_H

#include <string>
#include "../../common/json.hpp"

// 执行 SQL 策略检查并返回输出信息到 output_msg
bool run_sql_policy_check(const std::string& sql,
                          const std::string& csv_content,
                          const nlohmann::json& policy_json,
                          std::string& output_msg);

#endif // SQL_POLICY_EXECUTOR_H