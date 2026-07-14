#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <string>

class CSVSQLOperation
{
public:
    CSVSQLOperation() {}
    bool readData(const std::string &csvData);
    std::vector<std::unordered_map<std::string, std::string>> select(
        const std::vector<std::string> &select_columns,
        const std::string &filter_column = "",
        const std::function<bool(const std::string &)> &filterFunc = nullptr);                                                   // SELECT 查询
    void printResult(const std::vector<std::unordered_map<std::string, std::string>> &sql_result) const;                         // 打印查询结果
    void outputResult(const std::vector<std::unordered_map<std::string, std::string>> &sql_result, std::string &compute_result); // 传递返回结果

private:
    std::vector<std::unordered_map<std::string, std::string>> data;
    std::vector<std::string> headers;

    std::string trimQuotesAndSpaces(const std::string &str); // 去除字符串两端的引号和空格
};

void sqlOperationTest(const std::string &csvData);
void sqlOperationSelect(std::string &result,
                        const std::string &csvData,
                        const std::vector<std::string> &select_columns,
                        const std::string &filter_column = "",
                        const std::function<bool(const std::string &)> &filterFunc = nullptr);