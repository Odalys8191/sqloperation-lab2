#pragma once
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <limits>
#include <iomanip>
#include <algorithm>
#include <set>

class CSVDataStatistics
{
public:
    CSVDataStatistics() {}
    bool readData(const std::string &csvData);                        // 从string中读取csv数据
    void printData();                                                 // 直接打印数据
    void executeStatisticsNum(std::set<std::string> targetColumns);   // 计算目标列的最大值、最小值和平均值，目标列的值预期为整数
    void executeStatisticsCount(std::set<std::string> targetColumns); // 记录目标列不同值的数量，对应列的值预期为string类型
    std::string getColumnCount(const std::string &column);
    int getColumnMax(const std::string &column);
    int getColumnMin(const std::string &column);
    double getColumnAvg(const std::string &column);

private:
    std::vector<std::unordered_map<std::string, std::string>> records_;                          // {column name, value}
    std::vector<std::unordered_map<std::string, int>> records_max_;                              // {column name, max}
    std::vector<std::unordered_map<std::string, int>> records_min_;                              // {column name, min}
    std::vector<std::unordered_map<std::string, double>> records_avg_;                           // {column name, avg}
    std::unordered_map<std::string, std::unordered_map<std::string, int>> records_column_count_; // {column name, {value, count}}

    std::string cleanField(const std::string &field);     // 去除字段两端的空格和引号
    static int stringToInt(const std::string &str);       // 将字符串转换为整数
    static double stringToDouble(const std::string &str); // 将字符串转换为浮点数
};

void dataStatisticsStringTest(const std::string &csvData);