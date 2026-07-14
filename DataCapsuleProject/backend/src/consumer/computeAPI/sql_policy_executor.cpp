// sql_policy_executor.cpp

#include "../../../include/consumer/computeAPI/sql_policy_executor.h"
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <sstream>

namespace py = pybind11;

std::string py_to_string(const py::handle& obj) {
    if (obj.is_none()) return "None";
    else if (py::isinstance<py::str>(obj)) return obj.cast<std::string>();
    else if (py::isinstance<py::int_>(obj)) return std::to_string(obj.cast<int>());
    else if (py::isinstance<py::float_>(obj)) return std::to_string(obj.cast<double>());
    else if (py::isinstance<py::bool_>(obj)) return obj.cast<bool>() ? "True" : "False";
    else return py::str(obj).cast<std::string>();
}

bool run_sql_policy_check(const std::string& sql,
                          const std::string& csv_content,
                          const nlohmann::json& policy_json,
                          std::string& output_msg) {
    //py::scoped_interpreter guard{};
    // static pybind11::scoped_interpreter guard{}; // 只初始化一次
    try {
        py::gil_scoped_acquire gil;
        py::module_ sys = py::module_::import("sys");
        // sys.attr("path").attr("insert")(0, ".");  // 当前目录
        // sys.attr("path").attr("insert")(0, "./computeAPI");  // computeAPI 目录
        // sys.attr("path").attr("insert")(0, "/home/yukino/DataCapsule/DataCapsule/backend/src/consumer/computeAPI");
        // std::filesystem::path exeDir = get_executable_dir();
        // std::filesystem::path computeApiPath = exeDir / "../../src/consumer/computeAPI";
        // computeApiPath = std::filesystem::canonical(computeApiPath);
        // sys.attr("path").attr("insert")(0, computeApiPath.string());
        sys.attr("path").attr("insert")(0, "../../src/consumer/computeAPI");

        auto checker = py::module_::import("policy_checker");
        auto check_func = checker.attr("check_sql_policy");

        // ✅ 将 nlohmann::json 转为字符串再传入 Python
        std::string policy_str = policy_json.dump();
        auto result = check_func(sql, csv_content, policy_str);

        bool violation = result["violation"].cast<bool>();
        py::list py_errors = result["errors"];

        std::ostringstream oss;
        if (violation) {
            oss << "❌ SQL 违规或执行异常:\n";
            for (auto item : py_errors) {
                oss << " - " << py::cast<std::string>(item) << "\n";
            }
        } else {
            oss << "✅ SQL 合规\n";
            auto py_result = result["result"];
            if (!py_result.is_none()) {
                oss << "查询结果：\n";
                for (auto row : py_result) {
                    py::dict dict_row = row.cast<py::dict>();
                    for (auto item : dict_row) {
                        oss << py_to_string(item.first) << ": " << py_to_string(item.second) << "\t";
                    }
                    oss << "\n";
                }
            } else {
                oss << "无查询结果。\n";
            }
        }

        output_msg = oss.str();
        return !violation;
    } catch (const py::error_already_set& e) {
        output_msg = std::string("Python 错误: ") + e.what();
        return false;
    }
}