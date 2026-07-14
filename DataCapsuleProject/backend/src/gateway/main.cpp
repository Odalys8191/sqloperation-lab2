#include "../../include/common/grpcClient.h"
#include "../../include/common/abeKeys.h"
#include "../../include/common/computeType.h"
#include "../../include/common/base64.openssl.h"
#include "../../include/common/abe/CPABE.h"
#include "../../include/common/abeKeys.h"
#include <httplib.h>
#include <ctime>
#include <fstream>
#include <mutex>
#include <random>
#include <sstream>
#include <unordered_map>

namespace {

struct AuthSession
{
    std::string user_id;
    std::time_t expires_at;
};

std::mutex auth_mutex;
std::unordered_map<std::string, AuthSession> auth_sessions;
constexpr int AUTH_SESSION_HOURS = 24;

bool readBinaryFile(const std::string &path, std::string &content)
{
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open())
    {
        return false;
    }
    content.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return true;
}

std::string generateAuthToken()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    std::ostringstream oss;
    for (int i = 0; i < 32; ++i)
    {
        oss << std::hex << dis(gen);
    }
    return oss.str();
}

std::string createAuthToken(const std::string &user_id)
{
    const std::string token = generateAuthToken();
    const std::time_t expires_at = std::time(nullptr) + AUTH_SESSION_HOURS * 3600;
    std::lock_guard<std::mutex> lock(auth_mutex);
    auth_sessions[token] = {user_id, expires_at};
    return token;
}

bool checkAuth(const httplib::Request &req, httplib::Response &res, std::string &out_user_id)
{
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");

    const std::string auth = req.get_header_value("Authorization");
    if (auth.size() <= 7 || auth.compare(0, 7, "Bearer ") != 0)
    {
        res.status = 401;
        res.set_content("{\"error\": \"Missing or invalid Authorization header\"}", "application/json");
        return false;
    }

    const std::string token = auth.substr(7);
    std::lock_guard<std::mutex> lock(auth_mutex);
    const auto it = auth_sessions.find(token);
    if (it == auth_sessions.end() || std::time(nullptr) > it->second.expires_at)
    {
        if (it != auth_sessions.end())
        {
            auth_sessions.erase(it);
        }
        res.status = 401;
        res.set_content("{\"error\": \"Invalid or expired token\"}", "application/json");
        return false;
    }

    out_user_id = it->second.user_id;
    return true;
}

} // namespace

void testOfficeOperation(const nlohmann::json &requestBody, ConsumerClient &consumerClient)
{
    // 获取各字段
    std::string dc_id = requestBody.value("dc_id", "default_dc_id");
    std::string user_id = requestBody.value("user_id", "default_user_id");
    std::string password = requestBody.value("password", "default_password");

    std::vector<int> selected_categories = requestBody.value("selected_categories", std::vector<int>());

    std::vector<int> selected_data_statistics = requestBody.value("selected_data_statistics", std::vector<int>());
    std::vector<std::string> data_statistics_count_target_columns = requestBody.value("data_statistics_count_target_columns", std::vector<std::string>());
    std::vector<std::string> data_statistics_min_target_columns = requestBody.value("data_statistics_min_target_columns", std::vector<std::string>());
    std::vector<std::string> data_statistics_max_target_columns = requestBody.value("data_statistics_max_target_columns", std::vector<std::string>());
    std::vector<std::string> data_statistics_avg_target_columns = requestBody.value("data_statistics_avg_target_columns", std::vector<std::string>());
    std::vector<std::string> data_statistics_sum_target_columns = requestBody.value("data_statistics_sum_target_columns", std::vector<std::string>());

    nlohmann::json database_operation = requestBody.value("database_operation", nlohmann::json());

    std::vector<std::string> selected_machine_learning = requestBody.value("selected_machine_learning", std::vector<std::string>());

    std::string selected_model_finetune = requestBody.value("selected_model_finetune", "default_model_finetune");

#define DEBUG
#ifdef DEBUG
    // 输出接收的数据（调试用）
    std::cout << "----------------- Received POST request on /consumer/consumeDataCapsule -----------------" << std::endl;
    std::cout << "Data Capsule ID: " << dc_id << std::endl;
    std::cout << "User ID: " << user_id << std::endl;
    std::cout << "Password: " << password << std::endl;
    std::cout << "Selected Categories: ";
    for (const auto &category : selected_categories)
    {
        std::cout << category << ", ";
    }
    std::cout << std::endl;
    std::cout << "Selected Data Statistics: ";
    for (const auto &statistics : selected_data_statistics)
    {
        std::cout << statistics << ", ";
    }
    std::cout << std::endl;
    std::cout << "Data Statistics Count Limit Column: ";
    for (const auto &column : data_statistics_count_target_columns)
    {
        std::cout << column << ", ";
    }
    std::cout << std::endl;
    std::cout << "Data Statistics Min Limit Column: ";
    for (const auto &column : data_statistics_min_target_columns)
    {
        std::cout << column << ", ";
    }
    std::cout << std::endl;
    std::cout << "Data Statistics Max Limit Column: ";
    for (const auto &column : data_statistics_max_target_columns)
    {
        std::cout << column << ", ";
    }
    std::cout << std::endl;
    std::cout << "Data Statistics Avg Limit Column: ";
    for (const auto &column : data_statistics_avg_target_columns)
    {
        std::cout << column << ", ";
    }
    std::cout << std::endl;
    std::cout << "Data Statistics Sum Limit Column: ";
    for (const auto &column : data_statistics_sum_target_columns)
    {
        std::cout << column << ", ";
    }
    std::cout << std::endl;
    std::cout << "Database Operation: " << database_operation.dump() << std::endl;
    std::cout << "Selected Machine Learning: ";
    for (const auto &ml : selected_machine_learning)
    {
        std::cout << ml << ", ";
    }
    std::cout << std::endl;
    std::cout << "Selected Model Finetune: " << selected_model_finetune << std::endl;
    std::cout << "-------------------------------------------------------------------------------------------" << std::endl;
#endif

    json user_request;
    user_request["ComputeType"] = selected_categories;
    user_request["DataStatistics"]["Type"] = selected_data_statistics;
    user_request["DataStatistics"]["Count"]["Column"] = data_statistics_count_target_columns;
    user_request["DataStatistics"]["Min"]["Column"] = data_statistics_min_target_columns;
    user_request["DataStatistics"]["Max"]["Column"] = data_statistics_max_target_columns;
    user_request["DataStatistics"]["Avg"]["Column"] = data_statistics_avg_target_columns;
    user_request["DataStatistics"]["Sum"]["Column"] = data_statistics_sum_target_columns;
    user_request["database_operation"] = database_operation;
    user_request["MachineLearning"]["Type"] = selected_machine_learning;
    user_request["ModelFinetune"] = selected_model_finetune;

    // 读取用户私钥
    pairing_t pairing;
    init_pairing(pairing, PUBLIC_PARAM_PATH);
    json sk_json;
    CP_ABE_SK abe_sk1;
    init_CP_ABE_SK(abe_sk1, 10);                          // 第一层私钥
    deSerializeFromFile(abe_sk1, USER_SK1_PATH, pairing); // 第一层私钥
    sk_json = cpabe_sk_to_json(abe_sk1);
    user_request["SK"] = sk_json;

    // 调用 gRPC 服务
    std::string compute_result = consumerClient.consumeDataCapsule(user_id, dc_id, user_request.dump());

    std::cout << "User Request: " << user_request.dump(4) << std::endl;

    json response_message;
    if (compute_result == "")
    {
        response_message["message"] = "Data capsule consumption failed";
        response_message["compute_result"] = "Error: Failed to consume data capsule.";
    }
    else
    {
        response_message["message"] = "Data capsule consumption success";
        response_message["compute_result"] = compute_result;
    }
}

int main()
{
    // 初始化 cloud 客户端
    CloudClient cloudClient(grpc::CreateChannel(CLOUD_SERVICE_ADDRESS, grpc::InsecureChannelCredentials()));
    ProducerClient producerClient(grpc::CreateChannel(PRODUCER_SERVICE_ADDRESS, grpc::InsecureChannelCredentials()));
    ConsumerClient consumerClient(grpc::CreateChannel(CONSUMER_SERVICE_ADDRESS, grpc::InsecureChannelCredentials()));
    UserClient userClient(grpc::CreateChannel(USER_SERVICE_ADDRESS, grpc::InsecureChannelCredentials()));

    // 初始化 REST 服务
    httplib::Server svr;

    // Debug: 确认路由是否注册
    std::cout << "Registering routes..." << std::endl;

    // -------------------------------------- 云存储路由 --------------------------------------
    svr.Get("/cloud/getExistingDataCapsuleInformation", [&](const httplib::Request &req, httplib::Response &res)
            {
                // 设置 CORS 头
                res.set_header("Access-Control-Allow-Origin", "*"); // 允许所有域名跨域请求
                res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS"); // 允许的 HTTP 方法
                // res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization"); // 允许的请求头

                if (req.has_param("message")) 
                {
                    std::string message = req.get_param_value("message");

                    // 调用 gRPC 服务
                    std::string data_capsules = cloudClient.getExistingDataCapsuleInformation(message);

                    // 构造 REST 响应
                    res.set_content(data_capsules, "application/json");
                } 
                else 
                {
                    std::cout << "Missing 'message' parameter" << std::endl;
                    res.status = 400;
                    res.set_content("{\"error\": \"Missing 'message' parameter\"}", "application/json");
                } });

    // -------------------------------------- 云存储路由 - 获取胶囊数量 --------------------------------------
    svr.Get("/cloud/getExistingDataCapsuleNumber", [&](const httplib::Request &req, httplib::Response &res)
            {
                // 设置 CORS 头
                res.set_header("Access-Control-Allow-Origin", "*"); // 允许所有域名跨域请求
                res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS"); // 允许的 HTTP 方法
                // res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization"); // 允许的请求头

                if (req.has_param("message")) 
                {
                    std::string message = req.get_param_value("message");

                    // 调用 gRPC 服务
                    int data_capsules_number = cloudClient.getExistingDataCapsuleNumber(message);

                    // 构造 REST 响应
                    if (data_capsules_number != -1) 
                    {
                        std::cout << "Successfully retrieved data capsule number: " << data_capsules_number << std::endl;
                        json response_json;
                        response_json["data_capsules_number"] = data_capsules_number;
                        res.set_content(response_json.dump(), "application/json");
                    } 
                    else 
                    {
                        res.status = 500;
                        res.set_content("{\"error\": \"Failed to get data capsule number\"}", "application/json");
                    }
                } 
                else 
                {
                    std::cout << "Missing 'message' parameter" << std::endl;
                    res.status = 400;
                    res.set_content("{\"error\": \"Missing 'message' parameter\"}", "application/json");
                } });

    // -------------------------------------- 打包数据胶囊路由 --------------------------------------
    svr.Post("/producer/generateDataCapsule", [&](const httplib::Request &req, httplib::Response &res)
             {
    // 设置 CORS 头
    res.set_header("Access-Control-Allow-Origin", "*"); // 允许所有域名跨域请求
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS"); // 允许的 HTTP 方法
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");

    std::string authenticated_user_id;
    if (!checkAuth(req, res, authenticated_user_id))
    {
        return;
    }

    try {
        // 检查请求是否有正文
        if (req.body.empty()) {
            std::cout << "Request body is empty" << std::endl;
            res.status = 400;
            res.set_content("{\"error\": \"Request body is empty\"}", "application/json");
            return;
        }

        // 解析 JSON 请求体
        nlohmann::json requestBody = nlohmann::json::parse(req.body);

        // 获取各字段（owner_id 从 token 获取，不信任请求体）
        std::string owner_id = authenticated_user_id;
        std::string abe_policy = requestBody.value("abe_policy", "default_abe_policy");
        int usable_times = requestBody.value("usable_times", 0);
        std::string data_description = requestBody.value("data_description", "no data description provided");
        std::string columnInfo = requestBody.value("column_information", "no columns info provided");
        std::vector<int> selected_categories = requestBody.value("selected_categories", std::vector<int>());

        json policy;
        policy["ComputeType"] = selected_categories;

        // 根据 selected_categories 判断是否需要处理各种计算类型
        if (std::find(selected_categories.begin(), selected_categories.end(), ComputeType::DATA_STATISTICS) != selected_categories.end())
        {
            std::vector<int> selected_data_statistics = requestBody.value("selected_data_statistics", std::vector<int>());
            policy["DataStatistics"]["Type"] = selected_data_statistics;
            // 根据 selected_data_statistics 获取各列的限制
            if (std::find(selected_data_statistics.begin(), selected_data_statistics.end(), DataStatisticsType::COUNT) != selected_data_statistics.end())
            {
                std::vector<std::string> data_statistics_count_limit_columns = requestBody.value("data_statistics_count_limit_columns", std::vector<std::string>());
                policy["DataStatistics"]["Count"]["Column"] = data_statistics_count_limit_columns;
            }
            if (std::find(selected_data_statistics.begin(), selected_data_statistics.end(), DataStatisticsType::MIN) != selected_data_statistics.end())
            {
                std::vector<std::string> data_statistics_min_limit_columns = requestBody.value("data_statistics_min_limit_columns", std::vector<std::string>());
                policy["DataStatistics"]["Min"]["Column"] = data_statistics_min_limit_columns;
            }
            if (std::find(selected_data_statistics.begin(), selected_data_statistics.end(), DataStatisticsType::MAX) != selected_data_statistics.end())
            {
                std::vector<std::string> data_statistics_max_limit_columns = requestBody.value("data_statistics_max_limit_columns", std::vector<std::string>());
                policy["DataStatistics"]["Max"]["Column"] = data_statistics_max_limit_columns;
            }
            if (std::find(selected_data_statistics.begin(), selected_data_statistics.end(), DataStatisticsType::AVG) != selected_data_statistics.end())
            {
                std::vector<std::string> data_statistics_avg_limit_columns = requestBody.value("data_statistics_avg_limit_columns", std::vector<std::string>());
                policy["DataStatistics"]["Avg"]["Column"] = data_statistics_avg_limit_columns;
            }
            if (std::find(selected_data_statistics.begin(), selected_data_statistics.end(), DataStatisticsType::SUM) != selected_data_statistics.end())
            {
                std::vector<std::string> data_statistics_sum_limit_columns = requestBody.value("data_statistics_sum_limit_columns", std::vector<std::string>());
                policy["DataStatistics"]["Sum"]["Column"] = data_statistics_sum_limit_columns;
            }
        }

        if(std::find(selected_categories.begin(), selected_categories.end(), ComputeType::SQL_OPERATION) != selected_categories.end()) 
        {
            // 优先处理完整策略
            if (requestBody.contains("sql_operation_policy")) {
                policy["SQLOperation"] = requestBody["sql_operation_policy"];
            } else {
                std::string sql_operation_limit = requestBody.value("sql_operation_limit", "");
                policy["SQLOperation"]["Limit"] = sql_operation_limit;
            }
        }
        
        if( std::find(selected_categories.begin(), selected_categories.end(), ComputeType::MACHINE_LEARNING) != selected_categories.end()) 
        {
            std::vector<std::string> selected_machine_learning = requestBody.value("selected_machine_learning", std::vector<std::string>());
            policy["MachineLearning"]["Type"] = selected_machine_learning;
        }

        // if( std::find(selected_categories.begin(), selected_categories.end(), ComputeType::MODEL_FINETUNE) != selected_categories.end()) ) 
        // {
        //     std::string selected_model_finetune = requestBody.value("selected_model_finetune", "default_model_finetune");
        //     policy["ModelFinetune"]["Model"] = selected_model_finetune;
        // }

        if (std::find(selected_categories.begin(), selected_categories.end(), ComputeType::OFFICE_OPERATION) != selected_categories.end())
        {
            int office_users_number = requestBody["office_users_number"].get<int>(); // 必须字段 // 必须字段
            if (office_users_number <= 0)
            {
                std::cout << "Invalid office_users_number: must be greater than 0" << std::endl;
                res.status = 400;
                res.set_content("{\"error\": \"Invalid office_users_number: must be greater than 0\"}", "application/json");
                return;
            }
            OfficeOperationType office_operation_type = static_cast<OfficeOperationType>(requestBody["selected_office_operation_type"].get<int>()); // 必须字段
            std::vector<std::string> office_abe_policys = requestBody["office_operation_user_policies"].get<std::vector<std::string>>();            // 必须字段
            if (office_abe_policys.size() != office_users_number)
            {
                std::cout << "Mismatch between office_users_number and size of office_operation_user_policies" << std::endl;
                res.status = 400;
                res.set_content("{\"error\": \"Mismatch between office_users_number and size of office_operation_user_policies\"}", "application/json");
                return;
            }
            policy["OfficeOperation"]["Type"] = office_operation_type;
            policy["OfficeOperation"]["UsersNumber"] = office_users_number;
            policy["OfficeOperation"]["ABEPolicys"] = office_abe_policys;
        }

        std::string file_data_url = requestBody["file"].get<std::string>();
        // 移除 "data:[<mediatype>];base64," 前缀
        size_t comma_pos = file_data_url.find(',');
        if (comma_pos != std::string::npos)
        {
            file_data_url = file_data_url.substr(comma_pos + 1);
        }
        std::string file_content = base64Decode(file_data_url);

#ifdef TEST_OFFICE
        // 将file存为.docx文件
        std::string office_file_path = "officeFiles/office_file.docx";
        std::ofstream office_file(office_file_path, std::ios::binary | std::ios::out);
        if (office_file.is_open())
        {
            // Write the json data to the file with indentation (4 spaces)
            office_file << file_content;
            office_file.close();
        }
        else
        {
            std::cerr << "Error: opening file for writing result." << std::endl;
        }
#endif

        json attributes;
        attributes["DataDescription"] = data_description;
        attributes["Columns"] = columnInfo;
#ifdef ABE_POLICY_TEST
        attributes["AccessStructure"]["1"] = "(role:admin AND clearance:high) OR (dept:finance AND role:user)"; // 用户策略
#else
        attributes["AccessStructure"]["1"] = abe_policy; // 用户策略
#endif
        attributes["AccessStructure"]["2"] = "((team:alpha AND project:X) OR (team:beta AND project:Y) OR dept:hr)"; // TEE

        // 调用 gRPC 服务
        std::string dc_id = producerClient.generateDataCapsule(owner_id, policy, abe_policy, usable_times, file_content, attributes); 

        if (dc_id.empty())
        {
            res.status = 502;
            res.set_content("{\"error\": \"Failed to generate data capsule\"}", "application/json");
            return;
        }

        json response_message;
        response_message["message"] = "Data capsule generated successfully";
        response_message["dc_id"] = dc_id;

        // 构造 REST 响应
        res.set_content(response_message.dump(), "application/json");
    } catch (const nlohmann::json::exception &e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        res.status = 400;
        res.set_content("{\"error\": \"Invalid JSON format\"}", "application/json");
    } catch (const std::exception &e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        res.status = 500;
        res.set_content("{\"error\": \"Internal server error\"}", "application/json");
    } });

    // -------------------------------------- 消费数据胶囊路由 --------------------------------------
    svr.Post("/consumer/consumeDataCapsule", [&](const httplib::Request &req, httplib::Response &res)
             {
    // 设置 CORS 头
    res.set_header("Access-Control-Allow-Origin", "*"); // 允许所有域名跨域请求
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS"); // 允许的 HTTP 方法
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");

    std::string authenticated_user_id;
    if (!checkAuth(req, res, authenticated_user_id))
    {
        return;
    }

    try {
        // 检查请求是否有正文
        if (req.body.empty()) {
            std::cout << "Request body is empty" << std::endl;
            res.status = 400;
            res.set_content("{\"error\": \"Request body is empty\"}", "application/json");
            return;
        }

        // 解析 JSON 请求体
        nlohmann::json requestBody = nlohmann::json::parse(req.body);

        // 获取各字段（user_id 从 token 获取，不信任请求体）
        std::string dc_id = requestBody.value("dc_id", "");
        std::string user_id = authenticated_user_id;
        if (dc_id.empty())
        {
            res.status = 400;
            res.set_content("{\"error\": \"dc_id must not be empty\"}", "application/json");
            return;
        }
        std::vector<int> selected_categories = requestBody.value("selected_categories", std::vector<int>());

        // 读取用户 ABE 私钥
        std::string abe_private_key_encoded = requestBody["user_abe_private_key"].get<std::string>();
        size_t comma_pos = abe_private_key_encoded.find(',');
        if (comma_pos != std::string::npos)
        {
            abe_private_key_encoded = abe_private_key_encoded.substr(comma_pos + 1);
        }
        std::string abe_private_key = base64Decode(abe_private_key_encoded);
        
        json user_request;
        user_request["ComputeType"] = selected_categories;

        if( std::find(selected_categories.begin(), selected_categories.end(), ComputeType::DATA_STATISTICS) != selected_categories.end()) 
        {
            std::vector<int> selected_data_statistics = requestBody.value("selected_data_statistics", std::vector<int>());
            user_request["DataStatistics"]["Type"] = selected_data_statistics;
            if(std::find(selected_data_statistics.begin(), selected_data_statistics.end(), DataStatisticsType::COUNT) != selected_data_statistics.end())
            {
                std::vector<std::string> data_statistics_count_target_columns = requestBody.value("data_statistics_count_target_columns", std::vector<std::string>());
                user_request["DataStatistics"]["Count"]["Column"] = data_statistics_count_target_columns;
            }
            if(std::find(selected_data_statistics.begin(), selected_data_statistics.end(), DataStatisticsType::MIN) != selected_data_statistics.end())
            {
                std::vector<std::string> data_statistics_min_target_columns = requestBody.value("data_statistics_min_target_columns", std::vector<std::string>());
                user_request["DataStatistics"]["Min"]["Column"] = data_statistics_min_target_columns;
            }
            if(std::find(selected_data_statistics.begin(), selected_data_statistics.end(), DataStatisticsType::MAX) != selected_data_statistics.end())
            {
                std::vector<std::string> data_statistics_max_target_columns = requestBody.value("data_statistics_max_target_columns", std::vector<std::string>());
                user_request["DataStatistics"]["Max"]["Column"] = data_statistics_max_target_columns;
            }
            if(std::find(selected_data_statistics.begin(), selected_data_statistics.end(), DataStatisticsType::AVG) != selected_data_statistics.end())
            {
                std::vector<std::string> data_statistics_avg_target_columns = requestBody.value("data_statistics_avg_target_columns", std::vector<std::string>());
                user_request["DataStatistics"]["Avg"]["Column"] = data_statistics_avg_target_columns;
            }
            if(std::find(selected_data_statistics.begin(), selected_data_statistics.end(), DataStatisticsType::SUM) != selected_data_statistics.end())
            {
                std::vector<std::string> data_statistics_sum_target_columns = requestBody.value("data_statistics_sum_target_columns", std::vector<std::string>());
                user_request["DataStatistics"]["Sum"]["Column"] = data_statistics_sum_target_columns;
            }
        }
        if(std::find(selected_categories.begin(), selected_categories.end(), ComputeType::SQL_OPERATION) != selected_categories.end()) 
        {
            // 支持结构化SQLOperation策略，结构为 { sql: ... }
            if (requestBody.contains("SQLOperation") && requestBody["SQLOperation"].is_object() && requestBody["SQLOperation"].contains("sql")) {
                user_request["SQLOperation"] = requestBody["SQLOperation"];
            }
        }
        if( std::find(selected_categories.begin(), selected_categories.end(), ComputeType::DATABASE_OPERATION) != selected_categories.end()) 
        {
            nlohmann::json database_operation = requestBody.value("database_operation", nlohmann::json());
            user_request["DatabaseOperation"] = database_operation;
        }
        if( std::find(selected_categories.begin(), selected_categories.end(), ComputeType::MACHINE_LEARNING) != selected_categories.end()) 
        {
            std::vector<std::string> selected_machine_learning = requestBody.value("selected_machine_learning", std::vector<std::string>());
            user_request["MachineLearning"]["Type"] = selected_machine_learning;
        }
        if( std::find(selected_categories.begin(), selected_categories.end(), ComputeType::MODEL_FINETUNE) != selected_categories.end()) 
        {
            std::string selected_model_finetune = requestBody.value("selected_model_finetune", "default_model_finetune");
            user_request["ModelFinetune"] = selected_model_finetune;
        }

        // 读取用户私钥
        pairing_t pairing;
        init_pairing(pairing, PUBLIC_PARAM_PATH);
        json sk_json;
        CP_ABE_SK abe_sk1;
        init_CP_ABE_SK(abe_sk1, 10); // 第一层私钥
#ifdef ABE_TEST
        // 测试用，直接从文件读取私钥
        deSerializeFromFile(abe_sk1, USER_SK1_PATH, pairing); // 第一层私钥
#else
        deSerializeFromString(abe_sk1, abe_private_key, pairing); // 第一层私钥
#endif
        sk_json = cpabe_sk_to_json(abe_sk1);
        user_request["SK"] = sk_json;

        // 调用 gRPC 服务
        std::string compute_result = consumerClient.consumeDataCapsule(user_id, dc_id, user_request.dump());

        std::cout << "User Request: " << user_request.dump(4) << std::endl;

        json response_message;
        const std::string error_prefix = "__ERROR__:";
        if (compute_result.rfind(error_prefix, 0) == 0)
        {
            const std::string backend_error = compute_result.substr(error_prefix.size());
            response_message["message"] = "Data capsule consumption failed";
            if (backend_error.find("ABE attributes insufficient") != std::string::npos)
            {
                res.status = 403;
                response_message["compute_result"] = "Error: ABE attributes insufficient.";
            }
            else
            {
                res.status = 502;
                response_message["compute_result"] = "Error: " + backend_error;
            }
        }
        else if (compute_result == "")
        {
            res.status = 502;
            response_message["message"] = "Data capsule consumption failed";
            response_message["compute_result"] = "Error: Failed to consume data capsule.";
        }
        else
        {
            res.status = 200;
            response_message["message"] = "Data capsule consumption success";
            response_message["compute_result"] = compute_result;
        }

        // 构造 REST 响应
        res.set_content(response_message.dump(), "application/json");
    } catch (const nlohmann::json::exception &e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        res.status = 400;
        res.set_content("{\"error\": \"Invalid JSON format\"}", "application/json");
    } catch (const std::exception &e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        res.status = 500;
        res.set_content("{\"error\": \"Internal server error\"}", "application/json");
    } });

    // -------------------------------------- 用户登录路由 --------------------------------------
    svr.Post("/user/login", [&](const httplib::Request &req, httplib::Response &res)
             {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");

        try {
            if (req.body.empty()) {
                std::cout << "Request body is empty" << std::endl;
                res.status = 400;
                res.set_content("{\"error\": \"Request body is empty\"}", "application/json");
                return;
            }

            nlohmann::json requestBody = nlohmann::json::parse(req.body);
            std::string user_id = requestBody.value("user_id", "");
            std::string password = requestBody.value("password", "");

            std::cout << "user_id: " << user_id << ", Password: " << password << std::endl;

            if (user_id.empty() || password.empty()) {
                res.status = 400;
                res.set_content("{\"error\": \"user_id and password must not be empty\"}", "application/json");
                return;
            }

            std::string response_message = userClient.loginUser(user_id, password);

            // 如果登录成功，返回 200 和 token 信息
            if (response_message == "success") {
                const std::string access_token = createAuthToken(user_id);
                json token_response;
                token_response["access"] = access_token;
                token_response["refresh"] = access_token;
                token_response["user_id"] = user_id;
                res.status = 200;
                res.set_content(token_response.dump(), "application/json");
            } else {
                // 登录失败时返回 401 Unauthorized
                res.status = 401;
                res.set_content("{\"error\": \"Invalid user_id or password\"}", "application/json");
            }

        } catch (const nlohmann::json::exception &e) {
            std::cerr << "JSON parsing error: " << e.what() << std::endl;
            res.status = 400;
            res.set_content("{\"error\": \"Invalid JSON format\"}", "application/json");
        } catch (const std::exception &e) {
            std::cerr << "Server error: " << e.what() << std::endl;
            res.status = 500;
            res.set_content("{\"error\": \"Internal server error\"}", "application/json");
        } });

    // -------------------------------------- 用户注册路由 --------------------------------------
    svr.Post("/user/register", [&](const httplib::Request &req, httplib::Response &res)
             {
        // 设置 CORS 头
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");

        try {
            // 检查请求是否有正文
            if (req.body.empty()) {
                res.status = 400;
                res.set_content("{\"error\": \"Request body is empty\"}", "application/json");
                return;
            }

            // 解析 JSON 请求体
            nlohmann::json requestBody = nlohmann::json::parse(req.body);

            // 获取用户名和密码
            std::string username = requestBody.value("username", "");
            std::string password = requestBody.value("password", "");

            if (username.empty() || password.empty()) {
                res.status = 400;
                res.set_content("{\"error\": \"Username and password must not be empty\"}", "application/json");
                return;
            }

            // 调用 gRPC 用户注册服务
            //std::string user_id = userClient.registerUser(username, password).first;
            //std::cout<<user_id<<std::endl;
            auto [user_id, message] = userClient.registerUser(username, password);
            if (user_id.empty()) {
                // 注册失败，返回错误信息
                res.status = 500;
                res.set_content("{\"error\": \"" + message + "\"}", "application/json");
                return;
            }

            std::string private_key_content;
            if (!readBinaryFile(USER_SK1_PATH, private_key_content))
            {
                res.status = 500;
                res.set_content("{\"error\": \"Failed to load private key file\"}", "application/json");
                return;
            }

            // 构造响应（返回用户ID + 现有私钥文件，供前端直接下载）
            json register_response;
            register_response["user_id"] = user_id;
            register_response["abe_private_key_file_name"] = "abe_secret_key_1.bin";
            register_response["abe_private_key_base64"] = base64Encode(
                reinterpret_cast<const unsigned char *>(private_key_content.data()),
                static_cast<unsigned int>(private_key_content.size()));
            res.set_content(register_response.dump(), "application/json");
        } catch (const nlohmann::json::exception &e) {
            std::cerr << "JSON parsing error: " << e.what() << std::endl;
            res.status = 400;
            res.set_content("{\"error\": \"Invalid JSON format\"}", "application/json");
        } catch (const std::exception &e) {
            std::cerr << "Server error: " << e.what() << std::endl;
            res.status = 500;
            res.set_content("{\"error\": \"Internal server error\"}", "application/json");
        } });

    // -------------------------------------- 下载用户私钥路由 --------------------------------------
    svr.Get("/user/downloadPrivateKey", [&](const httplib::Request &req, httplib::Response &res)
            {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");

        std::string authenticated_user_id;
        if (!checkAuth(req, res, authenticated_user_id))
        {
            return;
        }

        try
        {
            std::string private_key_content;
            if (!readBinaryFile(USER_SK1_PATH, private_key_content))
            {
                res.status = 500;
                res.set_content("{\"error\": \"Failed to load private key file\"}", "application/json");
                return;
            }

            json key_response;
            key_response["user_id"] = authenticated_user_id;
            key_response["abe_private_key_file_name"] = "abe_secret_key_1.bin";
            key_response["abe_private_key_base64"] = base64Encode(
                reinterpret_cast<const unsigned char *>(private_key_content.data()),
                static_cast<unsigned int>(private_key_content.size()));
            res.status = 200;
            res.set_content(key_response.dump(), "application/json");
        }
        catch (const std::exception &e)
        {
            std::cerr << "Server error: " << e.what() << std::endl;
            res.status = 500;
            res.set_content("{\"error\": \"Internal server error\"}", "application/json");
        } });

#ifdef OFFICE_TEST
    // -------------------------------------- 测试Office --------------------------------------------
    std::string dc_id = "dc12345";
    std::string user_id = "00000001";
    std::string password = "password123";
    std::vector<int> selected_categories = {6};
    std::vector<int> selected_data_statistics = {};
    std::vector<std::string> data_statistics_count_target_columns = {};
    std::vector<std::string> data_statistics_min_target_columns = {};
    std::vector<std::string> data_statistics_max_target_columns = {};
    std::vector<std::string> data_statistics_avg_target_columns = {};
    std::vector<std::string> data_statistics_sum_target_columns = {};
    nlohmann::json database_operation = {};
    std::vector<std::string> selected_machine_learning = {};
    std::string selected_model_finetune = "";

    nlohmann::json testOfficeRequestBody = {
        {"dc_id", dc_id},
        {"user_id", user_id},
        {"password", password},
        {"selected_categories", selected_categories},
        {"selected_data_statistics", selected_data_statistics},
        {"data_statistics_count_target_columns", data_statistics_count_target_columns},
        {"data_statistics_min_target_columns", data_statistics_min_target_columns},
        {"data_statistics_max_target_columns", data_statistics_max_target_columns},
        {"data_statistics_avg_target_columns", data_statistics_avg_target_columns},
        {"data_statistics_sum_target_columns", data_statistics_sum_target_columns},
        {"database_operation", database_operation},
        {"selected_machine_learning", selected_machine_learning},
        {"selected_model_finetune", selected_model_finetune}};
    testOfficeOperation(testOfficeRequestBody, consumerClient);
#endif

    // -------------------------------------- 启动 REST Gateway --------------------------------------
    std::cout << "REST Gateway running on http://0.0.0.0:5000" << std::endl;
    svr.listen("0.0.0.0", 5000);

    return 0;
}
