#include "../../../include/consumer/computeAPI/dataStatistics.h"
#include "../../../include/common/tools.h"

bool CSVDataStatistics::readData(const std::string &csvData)
{
    std::stringstream ss(csvData);
    std::string line;
    std::vector<std::string> headers;

    // Read the header line to determine the column names
    if (std::getline(ss, line))
    {
        std::stringstream headerStream(line);
        std::string header;
        while (std::getline(headerStream, header, ','))
        {
            headers.push_back(cleanField(header));
        }
    }
    else
    {
        std::cerr << "[CSVDataStatistics::readData] CSV data is empty or malformed." << std::endl;
        return false;
    }

    // Read each data line
    while (std::getline(ss, line))
    {
        std::stringstream recordStream(line);
        std::unordered_map<std::string, std::string> record;
        std::string field;
        size_t columnIndex = 0;

        // Read each field and map it to its respective header
        while (std::getline(recordStream, field, ',') && columnIndex < headers.size())
        {
            record[headers[columnIndex]] = cleanField(field);
            ++columnIndex;
        }

        // If the number of columns in the line doesn't match the header, we ignore it
        if (columnIndex == headers.size())
        {
            records_.push_back(record);
        }
    }

    return true;
}

// print plain data
void CSVDataStatistics::printData()
{
    if (records_.empty())
    {
        std::cout << "No data available to print." << std::endl;
        return;
    }

    // Find the maximum width of each column
    std::unordered_map<std::string, size_t> columnWidths;
    for (const auto &record : records_)
    {
        for (const auto &pair : record)
        {
            columnWidths[pair.first] = std::max(columnWidths[pair.first], pair.second.length());
        }
    }

    // Print the header
    for (const auto &pair : columnWidths)
    {
        std::cout << std::setw(pair.second) << pair.first << " | ";
    }
    std::cout << std::endl;

    // Print the data
    for (const auto &record : records_)
    {
        for (const auto &pair : record)
        {
            std::cout << std::setw(columnWidths[pair.first]) << pair.second << " | ";
        }
        std::cout << std::endl;
    }
}

// 计算目标列的最大值、最小值和平均值，目标列的值预期为整数/浮点数
void CSVDataStatistics::executeStatisticsNum(std::set<std::string> targetColumns)
{
    if (records_.empty())
    {
        std::cout << "No data available for statistics." << std::endl;
        return;
    }

    std::unordered_map<std::string, std::pair<int, int>> columnStats; // {column name, {max, min}}
    std::unordered_map<std::string, int> columnCount;                 // {column name, count}
    std::unordered_map<std::string, double> columnSum;                // {column name, sum}

    for (const auto &record : records_)
    {
        for (const auto &column : targetColumns)
        {
            if (record.find(column) != record.end())
            {
                int value = stringToDouble(record.at(column));
                if (columnStats.find(column) == columnStats.end())
                {
                    columnStats[column] = {value, value};
                    columnCount[column] = 1;
                    columnSum[column] = value;
                }
                else
                {
                    columnStats[column].first = std::max(columnStats[column].first, value);
                    columnStats[column].second = std::min(columnStats[column].second, value);
                    columnCount[column]++;
                    columnSum[column] += value;
                }
            }
        }
    }

#ifdef DEBUG
    std::cout << "----------------- [Statistics Num] -----------------" << std::endl;
#endif
    for (const auto &column : targetColumns)
    {
        if (columnStats.find(column) != columnStats.end())
        {
            double average = columnSum[column] / columnCount[column];
#ifdef DEBUG
            std::cout << "Column: " << column << std::endl;
            std::cout << "Max: " << columnStats[column].first << std::endl;
            std::cout << "Min: " << columnStats[column].second << std::endl;
            std::cout << "Average: " << std::fixed << std::setprecision(2) << average << std::endl;
            std::cout << std::endl;
#endif

            // store the statistics for later use
            records_max_.push_back({{column, columnStats[column].first}});
            records_min_.push_back({{column, columnStats[column].second}});
            records_avg_.push_back({{column, average}});
        }
    }
}

// 记录目标列不同值的数量，对应列的值预期为string类型
void CSVDataStatistics::executeStatisticsCount(std::set<std::string> targetColumns)
{
    if (records_.empty())
    {
        std::cout << "No data available for statistics." << std::endl;
        return;
    }

    std::unordered_map<std::string, std::unordered_map<std::string, int>> columnCount; // {column name, {value, count}}

    for (const auto &record : records_)
    {
        for (const auto &column : targetColumns)
        {
            if (record.find(column) != record.end())
            {
                std::string value = record.at(column);
                if (columnCount[column].find(value) == columnCount[column].end())
                {
                    columnCount[column][value] = 1;
                }
                else
                {
                    columnCount[column][value]++;
                }
            }
        }
    }

    // store the count result to records_column_count_
    records_column_count_ = columnCount;

#ifdef DEBUG
    std::cout << "----------------- [Statistics Count] -----------------" << std::endl;
    for (const auto &column : targetColumns)
    {
        if (columnCount.find(column) != columnCount.end())
        {
            std::cout << "Column: " << column << std::endl;
            for (const auto &pair : columnCount[column])
            {
                std::cout << pair.first << ": " << pair.second << std::endl;
            }
            std::cout << std::endl;
        }
    }
#endif
}

std::string CSVDataStatistics::getColumnCount(const std::string &column)
{
    if (records_column_count_.find(column) != records_column_count_.end())
    {
        std::ostringstream oss;
        oss << "[Column Count]" << "Column " << column << ": " << std::endl;
        for (const auto &pair : records_column_count_[column])
        {
            oss << pair.first << ": " << pair.second << std::endl;
        }

#ifdef DEBUG
        std::cout << "Column: " << column << std::endl;
        for (const auto &pair : records_column_count_[column])
        {
            std::cout << pair.first << ": " << pair.second << std::endl;
        }
        std::cout << std::endl;
#endif

        return oss.str();
    }
    return "";
}

int CSVDataStatistics::getColumnMax(const std::string &column)
{
    // find the column in the records
    for (const auto &record : records_max_)
    {
        if (record.find(column) != record.end())
        {
            return record.at(column);
        }
    }
    std::cerr << "[CSVDataStatistics] Column not found." << std::endl;
    return 0;
}

int CSVDataStatistics::getColumnMin(const std::string &column)
{
    // find the column in the records
    for (const auto &record : records_min_)
    {
        if (record.find(column) != record.end())
        {
            return record.at(column);
        }
    }
    std::cerr << "[CSVDataStatistics] Column not found." << std::endl;
    return 0;
}

double CSVDataStatistics::getColumnAvg(const std::string &column)
{
    // find the column in the records
    for (const auto &record : records_avg_)
    {
        if (record.find(column) != record.end())
        {
            return record.at(column);
        }
    }
    std::cerr << "[CSVDataStatistics] Column not found." << std::endl;
    return 0.0;
}

// 去除字段两端的空格和引号
std::string CSVDataStatistics::cleanField(const std::string &field)
{
    std::string cleaned = field;
    // 去除前后的空白字符
    cleaned.erase(0, cleaned.find_first_not_of(" \t\r\n"));
    cleaned.erase(cleaned.find_last_not_of(" \t\r\n") + 1);

    // 去除引号
    if (!cleaned.empty() && cleaned.front() == '"' && cleaned.back() == '"')
    {
        cleaned = cleaned.substr(1, cleaned.size() - 2);
    }

    return cleaned;
}

// 将字符串转换为整数
int CSVDataStatistics::stringToInt(const std::string &str)
{
    try
    {
        return std::stoi(str);
    }
    catch (const std::invalid_argument &)
    {
        std::cerr << "Invalid value: " << str << std::endl;
        return 0; // If conversion fails, return 0
    }
    catch (const std::out_of_range &)
    {
        std::cerr << "Value out of range: " << str << std::endl;
        return 0;
    }
}

// 将字符串转换为浮点数
double CSVDataStatistics::stringToDouble(const std::string &str)
{
    try
    {
        return std::stod(str);
    }
    catch (const std::invalid_argument &)
    {
        std::cerr << "Invalid value: " << str << std::endl;
        return 0.0; // If conversion fails, return 0
    }
    catch (const std::out_of_range &)
    {
        std::cerr << "Value out of range: " << str << std::endl;
        return 0.0;
    }
}

void dataStatisticsStringTest(const std::string &csvData)
{
    std::set<std::string> target_columns_1 = {"age", "salary"}; // 要计算统计信息的列
    std::set<std::string> target_columns_2 = {"sex", "depid"};

    if (!csvData.empty())
    {
        // std::cout << "CSV content:\n"
        //           << csvData << std::endl;
        CSVDataStatistics statistics;
        statistics.readData(csvData);
        statistics.executeStatisticsNum(target_columns_1);
        std::cout << "Max age: " << statistics.getColumnMax("age") << std::endl;
        std::cout << "Min age: " << statistics.getColumnMin("age") << std::endl;
        std::cout << "Average age: " << statistics.getColumnAvg("age") << std::endl;
        std::cout << "Max salary: " << statistics.getColumnMax("salary") << std::endl;
        std::cout << "Min salary: " << statistics.getColumnMin("salary") << std::endl;
        std::cout << "Average salary: " << statistics.getColumnAvg("salary") << std::endl;

        statistics.executeStatisticsCount(target_columns_2);
        statistics.getColumnCount("sex");
        statistics.getColumnCount("depid");
    }
}
