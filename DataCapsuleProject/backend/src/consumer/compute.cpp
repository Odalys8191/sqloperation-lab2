#include "../../include/consumer/compute.h"
#include <sstream>
#include <set>

bool computeDataStatistics(const json &access_policy, const std::string &content, std::string &result)
{
    CSVDataStatistics csvSQLOperation;
    if (!csvSQLOperation.readData(content))
    {
        std::cerr << "[computeDataStatistics] Failed to read data from content!" << std::endl;
        return false;
    }

    std::set<DataStatisticsType> statistics_types = access_policy["DataStatistics"]["Type"];

    // 首先计算出target列的并集
    std::set<std::string> num_target_columns;
    std::set<std::string> count_target_columns;
    for (const auto &type : statistics_types)
    {
        switch (type)
        {
        case DataStatisticsType::COUNT:
        {
            std::set<std::string> target_columns = access_policy["DataStatistics"]["Count"]["Column"];
            count_target_columns.insert(target_columns.begin(), target_columns.end());
            break;
        }
        case DataStatisticsType::MIN:
        {
            std::set<std::string> target_columns = access_policy["DataStatistics"]["Min"]["Column"];
            num_target_columns.insert(target_columns.begin(), target_columns.end());
            break;
        }
        case DataStatisticsType::MAX:
        {
            std::set<std::string> target_columns = access_policy["DataStatistics"]["Max"]["Column"];
            num_target_columns.insert(target_columns.begin(), target_columns.end());
            break;
        }
        case DataStatisticsType::AVG:
        {
            std::set<std::string> target_columns = access_policy["DataStatistics"]["Avg"]["Column"];
            num_target_columns.insert(target_columns.begin(), target_columns.end());
            break;
        }
        default:
        {
            std::cerr << "Invalid Data Statistics Type!" << std::endl;
            break;
        }
        }
    }

    // 执行统计计算
    csvSQLOperation.executeStatisticsNum(num_target_columns);
    csvSQLOperation.executeStatisticsCount(count_target_columns);

    // 将计算结果保存
    std::ostringstream oss;
    for (const auto &type : statistics_types)
    {
        switch (type)
        {
        case DataStatisticsType::COUNT:
        {
            std::set<std::string> target_columns = access_policy["DataStatistics"]["Count"]["Column"];
            for (const auto &column : target_columns)
            {
                oss << csvSQLOperation.getColumnCount(column) << std::endl;
            }
            break;
        }
        case DataStatisticsType::MIN:
        {
            std::set<std::string> target_columns = access_policy["DataStatistics"]["Min"]["Column"];
            for (const auto &column : target_columns)
            {
                oss << "[Column Min]" << "Column " << column << ": " << " min value is " << csvSQLOperation.getColumnMin(column) << std::endl;
            }
            break;
        }
        case DataStatisticsType::MAX:
        {
            std::set<std::string> target_columns = access_policy["DataStatistics"]["Max"]["Column"];
            for (const auto &column : target_columns)
            {
                oss << "[Column Max]" << "Column " << column << ": " << " max value is " << csvSQLOperation.getColumnMax(column) << std::endl;
            }
            break;
        }
        case DataStatisticsType::AVG:
        {
            std::set<std::string> target_columns = access_policy["DataStatistics"]["Avg"]["Column"];
            for (const auto &column : target_columns)
            {
                oss << "[Column Avg] " << "Column " << column << ": " << " average is " << csvSQLOperation.getColumnAvg(column) << std::endl;
            }
            break;
        }
        default:
        {
            std::cerr << "Invalid Data Statistics Type!" << std::endl;
            break;
        }
        }
    }

    result += oss.str();
    return true;
}

bool computeSQLOperation(const json &access_policy, const std::string &content, std::string &result)
{
    CSVSQLOperation csvSQLOperation;
    if (!csvSQLOperation.readData(content))
    {
        std::cerr << "[computeDataStatistics] Failed to read data from content!" << std::endl;
        return false;
    }

    try
    {
        // 从JSON字符串中解析出查询条件
        std::vector<std::string> select_columns = access_policy["SQLOperation"]["Select"]["SelectColumns"].get<std::vector<std::string>>();
        std::string filter_column = access_policy["SQLOperation"]["Select"]["FilterColumn"].get<std::string>();
        std::string filter_value = access_policy["SQLOperation"]["Select"]["FilterValue"].get<std::string>();

        // 从CSV文件内容中解析出CSV数据
        std::string csvData = content;

        // 过滤条件函数
        std::function<bool(const std::string &)> filterFunc = nullptr;
        if (!filter_column.empty() && !filter_value.empty())
        {
            filterFunc = [filter_value](const std::string &value)
            {
                return value == filter_value;
            };
        }

        // 执行查询操作
        sqlOperationSelect(result, csvData, select_columns, filter_column, filterFunc);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
        return false;
    }

    return true;
}

//使用方提交SQL语句用这个新的逻辑
bool computecodeSQLOperation(const json &access_policy, const std::string &content, std::string &result, const json &program_policy)
{
    // 提取SQL
    std::string sql;
    try {
        sql = access_policy.at("SQLOperation").at("sql").get<std::string>();
    } catch (const std::exception &e) {
        result = std::string("[computecodeSQLOperation] Failed to extract SQL: ") + e.what();
        return false;
    }

    // 调用校验
    bool ok = run_sql_policy_check(sql, content, program_policy, result);

    // 错误优先判断
    if (result.find("Python 错误") != std::string::npos ||
        result.find("check failed") != std::string::npos) {
        std::cerr << "[computecodeSQLOperation] SQL check failed:\n" << result << std::endl;
        return false; // 系统错误，直接返回
    }

    if (!ok) {
        // 策略违规，但系统正常，允许继续
        std::cerr << "[computecodeSQLOperation] SQL violated policy:\n" << result << std::endl;
        return true;
    } else {
        std::cout << "[computecodeSQLOperation] SQL check passed:\n" << result << std::endl;
        return true;
    }
    return true;
}

bool computeDatabasedOperation(const json &access_policy, const std::string &content, std::string &result)
{
    try
    {
#ifdef DEBUG
        std::cout << "[computeDatabasedOperation] Access Policy: " << access_policy.dump(4) << std::endl;
#endif
        MySQLDatabase tempDb;
        std::string dc_id = "00000001";
        std::string query = access_policy["DatabaseOperation"]["Query"].get<std::string>();

        tempDb.dcSQLOperation(dc_id, content, query, result);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool computeDatabasedOperation(const std::string &dc_id, const json &access_policy, const std::string &content, std::string &result)
{
    try
    {
#ifdef DEBUG
        std::cout << "[computeDatabasedOperation] Access Policy: " << access_policy.dump(4) << std::endl;
#endif
        MySQLDatabase tempDb;
        std::string query = access_policy["DatabaseOperation"]["Query"].get<std::string>();

        tempDb.dcSQLOperation(dc_id, content, query, result);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool computeDataAnalysis(const std::string &content, std::string &result)
{
    result = "result:\ncompute Data Analysis";
    return true;
}

bool computeMachineLearning(const std::string &content, std::string &result)
{
    result = "result:\ncompute Machine Learning";
    return true;
}

static ModelFinetuneType getModelFinetuneType(const std::string &model_name)
{
    if (model_name == "DeepSeek_R1_Distill_Qwen_1_5B")
    {
        return ModelFinetuneType::DeepSeek_R1_Distill_Qwen_1_5B;
    }
    else if (model_name == "DeepSeek_R1_Distill_Qwen_7B")
    {
        return ModelFinetuneType::DeepSeek_R1_Distill_Qwen_7B;
    }
    else if (model_name == "DeepSeek_R1_Distill_Qwen_14B")
    {
        return ModelFinetuneType::DeepSeek_R1_Distill_Qwen_14B;
    }
    else if (model_name == "DeepSeek_R1_Distill_Qwen_32B")
    {
        return ModelFinetuneType::DeepSeek_R1_Distill_Qwen_32B;
    }
    else
    {
        std::cerr << "[getModelFinetuneType] Invalid model name!" << std::endl;
        return ModelFinetuneType::DeepSeek_R1_Distill_Qwen_1_5B; // default
    }
}

bool computeModelFinetune(const std::string &dc_id, const json &access_policy, const std::string &content, std::string &result)
{
    std::string selected_base_model = access_policy["ModelFinetune"].get<std::string>();
    std::cout << "[computeModelFinetune] Selected Base Model: " << selected_base_model << std::endl;
    ModelFinetuneType model_type = getModelFinetuneType(selected_base_model);
    std::cout << "[computeModelFinetune] Model Type: " << model_type << std::endl;

    ModelFineTune modelFineTune;
    // 将胶囊内容创建为微调数据集
    if (modelFineTune.createDataset(dc_id, content))
    {
        std::cout << "[computeModelFinetune] Dataset successfully created!" << std::endl;
    }
    else
    {
        std::cerr << "[computeModelFinetune] Failed to create dataset!" << std::endl;
        return false;
    }
    // 执行微调
    std::string dataset = dc_id;
    std::string output_model_path = "finetunedModels/" + dc_id + "/";
    if (!modelFineTune.executeFineTune(model_type, dataset, output_model_path))
    {
        std::cerr << "[computeModelFinetune] Failed to execute fine-tuning!" << std::endl;
        return false;
    }
    // std::cout << "[computeModelFinetune] Fine-tuning completed!" << std::endl;
    // 返回微调结果
    result = "Model Finetuning Task is in progress...\n";

    return true;
}

std::string getOfficeFileExtension(OfficeOperationType type)
{
    switch (type)
    {
    case OfficeOperationType::DOCX:
        return ".docx";
    case OfficeOperationType::XLSX:
        return ".xlsx";
    case OfficeOperationType::PPTX:
        return ".pptx";
    case OfficeOperationType::DOC:
        return ".doc";
    case OfficeOperationType::XLS:
        return ".xls";
    case OfficeOperationType::PPT:
        return ".ppt";
    default:
        return "";
    }
}

bool computeOfficeOperation(const std::string &dc_id, const json &access_policy, const std::string &content, const json &data_capsule_info, std::string &result)
{
    OfficeOperation officeOperation;
    OfficeOperationType office_type = data_capsule_info["OfficeOperation"]["Type"].get<OfficeOperationType>();
    std::string file_extension = getOfficeFileExtension(office_type);
    // 创建 Office 文件
    if (!officeOperation.createOfficeFile(dc_id, file_extension, content))
    {
        std::cerr << "[computeOfficeOperation] Failed to create office file!" << std::endl;
        return false;
    }
    // 执行 Office 操作
    if (!officeOperation.executeOfficeOperation(data_capsule_info, result))
    {
        std::cerr << "[computeOfficeOperation] Failed to execute office operation!" << std::endl;
        return false;
    }
    return true;
}

bool computeOfficeOperation(const std::string &dc_id, const json &data_capsule_info, const json &access_policy, const std::string &content, std::string &result)
{
    OfficeOperation officeOperation;
    OfficeOperationType office_type = access_policy["OfficeOperation"]["Type"].get<OfficeOperationType>();
    std::string file_extension = getOfficeFileExtension(office_type);
    // 创建 Office 文件
    if (!officeOperation.createOfficeFile(dc_id, file_extension, content))
    {
        std::cerr << "[computeOfficeOperation] Failed to create office file!" << std::endl;
        return false;
    }
    // 执行 Office 操作
    if (!officeOperation.executeOfficeOperation(data_capsule_info, result))
    {
        std::cerr << "[computeOfficeOperation] Failed to execute office operation!" << std::endl;
        return false;
    }
    return true;
}

// ------------------------- [For Test] -------------------------

void testDataStatisticsString(const std::string &csvData)
{
    dataStatisticsStringTest(csvData);
}
