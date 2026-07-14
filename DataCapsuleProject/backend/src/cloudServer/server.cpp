#include "../../include/cloudServer/server.h"
#include "../../include/common/encryptAlgType.h"
#include "../../include/common/computeType.h"
#include "../../include/common/tools.h"
#include "../../include/common/aes256.openssl.h"
#include "../../include/common/base64.openssl.h"
#include "../../include/common/signature.openssl.h"

#include <fstream>
#include <map>
#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;

#define DATA_CAPSULES_STORAGE_PATH "dataCapsules/"
#define EXISTING_DATA_CAPSULES_INFO_FILE DATA_CAPSULES_STORAGE_PATH "existingDataCapsules.json"

// 将 JSON 对象存储到文件
bool storeDataCapsuleToJsonFile(const nlohmann::json &capsule, const std::string &dc_id)
{
    // 确保目录存在，如果不存在则创建
    if (!fs::exists(DATA_CAPSULES_STORAGE_PATH))
    {
        fs::create_directories(DATA_CAPSULES_STORAGE_PATH);
    }

    std::ofstream file(DATA_CAPSULES_STORAGE_PATH + dc_id + ".dc");
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + dc_id + ".dc");
        return false;
    }
    file << capsule.dump(4); // 以缩进 4 格式化存储 JSON
    file.close();
    return true;
}

// 从文件读取 JSON 对象
bool readDataCapsuleFromJsonFile(const std::string &dc_id, nlohmann::json &capsule)
{
    std::ifstream file(DATA_CAPSULES_STORAGE_PATH + dc_id + ".dc");
    if (!file.is_open())
    {
        return false;
    }

    file >> capsule; // 读取 JSON
    file.close();
    return true;
}

grpc::Status CloudStorageServiceImpl::GetDataCapsule(grpc::ServerContext *context, const serverInteraction::GetDataCapsuleRequest *request,
                                                     serverInteraction::GetDataCapsuleResponse *response)
{
    const std::string &dc_id = request->dc_id();
    json data_capsule;
    // 在本地寻找数据胶囊
    if (readDataCapsuleFromJsonFile(dc_id, data_capsule))
    {
        response->set_data_capsule(data_capsule.dump());
        return grpc::Status::OK;
    }
    return grpc::Status(grpc::NOT_FOUND, "Data Capsule not found");
}

grpc::Status CloudStorageServiceImpl::GetProgramPolicy(grpc::ServerContext *context,
                                                       const serverInteraction::GetProgramPolicyRequest *request,
                                                       serverInteraction::GetProgramPolicyResponse *response)
{
    const std::string &dc_id = request->dc_id();
    json data_capsule;
    if (readDataCapsuleFromJsonFile(dc_id, data_capsule))
    {
        response->set_program_policy(data_capsule["MetaData"]["Policy"].dump());
        return grpc::Status::OK;
    }
    return grpc::Status(grpc::NOT_FOUND, "Data Capsule ID not found");
}

static bool storeDataCapsuleInfo(const json &data_capsule)
{
    // File path to store the data capsule information
    const std::string file_path = EXISTING_DATA_CAPSULES_INFO_FILE;

    // Step 1: Ensure the directory exists
    std::filesystem::create_directories("dataCapsules");

    // Step 2: Initialize a JSON object to store the data
    json data_capsule_info;
    data_capsule_info["DCID"] = data_capsule["MetaData"]["DCID"];
    data_capsule_info["PTEEID"] = data_capsule["MetaData"]["PTEEID"];
    data_capsule_info["Policy"] = data_capsule["MetaData"]["Policy"];
    data_capsule_info["DataIntro"] = data_capsule["MetaData"]["DataIntro"];

    // Step 3: Read or create the JSON file
    json existing_data;
    std::ifstream input_file(file_path);

    if (input_file.is_open())
    {
        try
        {
            input_file >> existing_data; // Parse existing JSON data
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error reading JSON file: " << e.what() << std::endl;
            existing_data = json::array(); // Reset to empty array if parsing fails
        }
        input_file.close();
    }
    else
    {
        existing_data = json::array(); // Initialize as an empty array if file doesn't exist
    }

    // Step 4: Append the new data capsule info
    existing_data.push_back(data_capsule_info);

    // Step 5: Write back to the file
    std::ofstream output_file(file_path);
    if (output_file.is_open())
    {
        try
        {
            output_file << existing_data.dump(4); // Pretty print with 4 spaces
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error writing JSON file: " << e.what() << std::endl;
            return false;
        }
        output_file.close();
    }
    else
    {
        std::cerr << "Unable to open file for writing: " << file_path << std::endl;
        return false;
    }

    return true;
}

// 将胶囊存储到本地
grpc::Status CloudStorageServiceImpl::StoreDataCapsule(grpc::ServerContext *context,
                                                       const serverInteraction::StoreDataCapsuleRequest *request,
                                                       serverInteraction::StoreDataCapsuleResponse *response)
{
    const std::string &data_capsule_str = request->data_capsule();
    json data_capsule = json::parse(data_capsule_str);
    if (storeDataCapsuleToJsonFile(data_capsule, data_capsule["MetaData"]["DCID"]) && storeDataCapsuleInfo(data_capsule))
    {
        response->set_success(true);
        return grpc::Status::OK;
    }
    return grpc::Status(grpc::INTERNAL, "[Error] Failed to store the data capsule!");
}

void runCloudServer(const std::string &server_address)
{
    CloudStorageServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Cloud Server listening on " << server_address << std::endl;

    // 确保目录存在，如果不存在则创建
    if (!fs::exists(DATA_CAPSULES_STORAGE_PATH))
    {
        fs::create_directories(DATA_CAPSULES_STORAGE_PATH);
    }

    server->Wait();
}

grpc::Status CloudStorageServiceImpl::GetExistingDataCapsuleInformation(grpc::ServerContext *context,
                                                                        const serverInteraction::GetExistingDataCapsuleInformationRequest *request,
                                                                        serverInteraction::GetExistingDataCapsuleInformationResponse *response)
{
    std::cout << "[GetExistingDataCapsuleInformation] Get Request!" << std::endl;
    const std::string &message = request->message();
    std::string data_capsules = readFromFile(EXISTING_DATA_CAPSULES_INFO_FILE);
    response->set_data_capsules(data_capsules);
    return grpc::Status::OK;
}

grpc::Status CloudStorageServiceImpl::GetExistingDataCapsuleNumber(grpc::ServerContext *context,
                                                                   const serverInteraction::GetExistingDataCapsuleNumberRequest *request,
                                                                   serverInteraction::GetExistingDataCapsuleNumberResponse *response)
{
    std::cout << "[GetExistingDataCapsuleNumber] Get Request!" << std::endl;
    const std::string &message = request->message();
    std::string data_capsules = readFromFile(EXISTING_DATA_CAPSULES_INFO_FILE);
    json data_capsules_json = json::parse(data_capsules);
    int data_capsule_number = data_capsules_json.size();
    response->set_data_capsules_number(data_capsule_number);
    return grpc::Status::OK;
}

// -------------------------------- useless functions --------------------------------

// grpc::Status CloudStorageServiceImpl::GetEncryptedDataCapsulePassword(grpc::ServerContext *context,
//                                                                       const serverInteraction::GetEncryptedDataCapsulePasswordRequest *request,
//                                                                       serverInteraction::GetEncryptedDataCapsulePasswordResponse *response)
// {
//     const std::string &dc_id = request->dc_id();
//     const std::string &user_id = request->user_id();
//     json data_capsule;
//     if (readDataCapsuleFromJsonFile(dc_id, data_capsule))
//     {
//         std::map<std::string, std::string> encrypted_passwords = data_capsule["MetaData"]["EncryptedPassword"].get<std::map<std::string, std::string>>();
//         // to check if the user_id is in the encrypted_passwords
//         if (encrypted_passwords.find(user_id) == encrypted_passwords.end())
//         {
//             return grpc::Status(grpc::NOT_FOUND, "User ID not found in the encrypted passwords");
//         }
//         std::string encrypted_password = encrypted_passwords[user_id];
//         response->set_encrypted_dc_password(encrypted_password);
//         // response->set_encrypted_dc_password(data_capsule["MetaData"]["EncryptedPassword"]);
//         return grpc::Status::OK;
//     }
//     return grpc::Status(grpc::NOT_FOUND, "Data Capsule ID not found");
// }

// grpc::Status CloudStorageServiceImpl::GetEncryptedDataCapsulePassword(grpc::ServerContext *context,
//                                                                       const serverInteraction::GetEncryptedDataCapsulePasswordRequest *request,
//                                                                       serverInteraction::GetEncryptedDataCapsulePasswordResponse *response)
// {
//     const std::string &dc_id = request->dc_id();
//     json data_capsule;
//     if (readDataCapsuleFromJsonFile(dc_id, data_capsule))
//     {
//         if (data_capsule["MetaData"].find("EncryptedPassword") == data_capsule["MetaData"].end())
//         {
//             return grpc::Status(grpc::NOT_FOUND, "Encrypted Password not found");
//         }
//         response->set_encrypted_dc_password(data_capsule["MetaData"]["EncryptedPassword"]);
//         return grpc::Status::OK;
//     }
//     return grpc::Status(grpc::NOT_FOUND, "Data Capsule ID not found");
// }

// grpc::Status CloudStorageServiceImpl::GetDataCapsuleKeyAdditionalInformation(grpc::ServerContext *context,
//                                                                              const serverInteraction::GetDataCapsuleKeyAdditionalInformationRequest *request,
//                                                                              serverInteraction::GetDataCapsuleKeyAdditionalInformationResponse *response)
// {
//     const std::string &dc_id = request->dc_id();
//     json data_capsule;
//     if (readDataCapsuleFromJsonFile(dc_id, data_capsule))
//     {
//         std::string additional_info = data_capsule["MetaData"]["SymmetricEncryption"].dump();
//         response->set_additional_information(additional_info);
//         return grpc::Status::OK;
//     }
//     return grpc::Status(grpc::NOT_FOUND, "Data Capsule ID not found");
// }
