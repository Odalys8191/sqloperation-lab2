#include "../../../include/consumer/computeAPI/sqlOperation.h"
#include "../../../include/common/tools.h"

bool CSVSQLOperation::readData(const std::string &csvData)
{
    std::stringstream ss(csvData);
    std::string line;
    bool isHeader = true;
    while (std::getline(ss, line))
    {
        std::istringstream stream(line);
        std::vector<std::string> row;
        std::string cell;
        char ch;

        // 手动解析CSV中的每个字段（考虑到引号和逗号分隔）
        bool insideQuote = false;
        std::string currentField;
        while (stream.get(ch))
        {
            if (ch == '"')
            {
                insideQuote = !insideQuote; // 切换引号内外状态
            }
            else if (ch == ',' && !insideQuote)
            {
                row.push_back(trimQuotesAndSpaces(currentField));
                currentField.clear();
            }
            else
            {
                currentField += ch;
            }
        }
        if (!currentField.empty())
        {
            row.push_back(trimQuotesAndSpaces(currentField));
        }

        // 处理第一行作为表头
        if (isHeader)
        {
            headers = row;
            isHeader = false;
        }
        else
        {
            if (row.size() != headers.size())
            {
                throw std::runtime_error("Data row does not match header size");
            }
            std::unordered_map<std::string, std::string> rowMap;
            for (size_t i = 0; i < headers.size(); ++i)
            {
                rowMap[headers[i]] = row[i];
            }
            data.push_back(rowMap);
        }
    }
    return true;
}

// SELECT 查询
std::vector<std::unordered_map<std::string, std::string>> CSVSQLOperation::select(
    const std::vector<std::string> &select_columns,
    const std::string &filter_column,
    const std::function<bool(const std::string &)> &filterFunc)
{
    std::vector<std::unordered_map<std::string, std::string>> result;

    for (const auto &row : data)
    {
        // 过滤操作
        if (!filter_column.empty() && filterFunc)
        {
            if (!filterFunc(row.at(filter_column)))
            {
                continue;
            }
        }

        // 按列选择
        std::unordered_map<std::string, std::string> selectedRow;
        for (const auto &col : select_columns)
        {
            if (row.find(col) != row.end())
            {
                selectedRow[col] = row.at(col);
            }
            else
            {
                throw std::runtime_error("Column not found: " + col);
            }
        }
        result.push_back(selectedRow);
    }

    return result;
}

// 打印查询结果
void CSVSQLOperation::printResult(const std::vector<std::unordered_map<std::string, std::string>> &sql_result) const
{
    // 打印列名
    if (!sql_result.empty())
    {
        for (const auto &[key, value] : sql_result[0])
        {
            std::cout << key << "\t";
        }
        std::cout << std::endl;

        // 打印数据
        for (const auto &row : sql_result)
        {
            for (const auto &[key, value] : row)
            {
                std::cout << value << "\t";
            }
            std::cout << std::endl;
        }
    }
}

void CSVSQLOperation::outputResult(const std::vector<std::unordered_map<std::string, std::string>> &sql_result, std::string &compute_result)
{
    std::ostringstream oss;
    // 打印列名
    if (!sql_result.empty())
    {
        for (const auto &[key, value] : sql_result[0])
        {
            oss << key << "\t";
        }
        oss << std::endl;

        // 打印数据
        for (const auto &row : sql_result)
        {
            for (const auto &[key, value] : row)
            {
                oss << value << "\t";
            }
            oss << std::endl;
        }
    }
    compute_result += oss.str();
}

// 去除字符串两端的引号和空格
std::string CSVSQLOperation::trimQuotesAndSpaces(const std::string &str)
{
    std::string result = str;
    if (!result.empty())
    {
        // 去除首尾的引号
        if (result.front() == '"')
            result.erase(0, 1);
        if (result.back() == '"')
            result.erase(result.size() - 1, 1);
        // 去除首尾的空格
        result.erase(0, result.find_first_not_of(" \t"));
        result.erase(result.find_last_not_of(" \t") + 1);
    }
    return result;
}

static std::string readFileToString(const std::string &filename)
{
    // 创建输入文件流对象
    std::ifstream file(filename);

    // 检查文件是否成功打开
    if (!file.is_open())
    {
        std::cerr << "Error opening file: " << filename << std::endl;
        return "";
    }

    // 使用stringstream读取文件内容
    std::stringstream buffer;
    buffer << file.rdbuf(); // 将文件内容写入stringstream

    // 返回文件的内容作为字符串
    return buffer.str();
}

void sqlOperationTest(const std::string &csvData)
{
    try
    {
        CSVSQLOperation csvOp;
        csvOp.readData(csvData);

        // 查询 "empid" 和 "empname" 列
        auto result = csvOp.select({"empid", "empname"});
        csvOp.printResult(result);

        // 查询 "empid"、"empname"、"cost"，并根据 "cost" 列进行过滤（例如：cost > 2000）
        auto filteredResult = csvOp.select({"empid", "empname", "cost"}, "cost", [](const std::string &cost)
                                           {
                                               return std::stod(cost) > 2000; // 过滤条件：cost > 2000
                                           });
        csvOp.printResult(filteredResult);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
}

void sqlOperationSelect(std::string &result,
                        const std::string &csvData,
                        const std::vector<std::string> &select_columns,
                        const std::string &filter_column,
                        const std::function<bool(const std::string &)> &filterFunc)
{
    try
    {
        CSVSQLOperation csvOp;
        csvOp.readData(csvData);

        auto filteredResult = csvOp.select(select_columns, filter_column, filterFunc);
        csvOp.outputResult(filteredResult, result);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
}
