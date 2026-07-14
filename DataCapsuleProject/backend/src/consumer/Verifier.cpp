#include "../../include/consumer/Verifier.h"
#include "../../include/common/signature.openssl.h"
#include "../../include/common/base64.openssl.h"
#include "../../include/common/aes256.openssl.h"
#include "../../include/common/tools.h"
#include "../../include/common/computeType.h"
#include "../../include/common/encryptAlgType.h"
#include "../../include/userServer/password_utils.h"
#include <set>
#include <map>

bool Verifier::verifyDataCapsuleState(const json &state)
{
    if (state["UsableTimes"] > 0)
    {
        return true;
    }
    return false;
}

void Verifier::setDCKey(const std::string &dc_key)
{
    this->dc_key_ = dc_key;
}

void Verifier::setProducerSignaturePublicKey(const std::string &producer_signature_public_key)
{
    this->producer_signature_public_key_ = producer_signature_public_key;
}

bool Verifier::verifyDataCapsuleAvailability(const std::string &encrypted_password, const json &dc_key_info)
{
    switch (dc_key_info["SymmetricEncryption"]["CryptoAlg"].get<EncryptAlgType>())
    {
    case EncryptAlgType::Encrypt_AES256_CBC:
    {
        // std::string dc_password = decryptAES256(encrypted_password, dc_key_info["EncryptKey"].get<std::string>(), base64Decode(dc_key_info["SymmetricEncryption"]["AESIV"].get<std::string>()));
        std::string dc_password = decryptAES256(encrypted_password, dc_key_info["EncryptKey"].get<std::string>());
        std::cout << dc_password  << std::endl;
        std::cout << dc_password_input_ << std::endl;
        // if (dc_password == dc_password_input_)
        if (verify_password_argon2id(dc_password, dc_password_input_))
        {
#ifdef DEBUG
            std::cout << "[verifyDataCapsuleAvailability] Password is valid!" << std::endl;
#endif
            return true;
        }
        return false;
    }
    default:
    {
        std::cerr << "[verifyDataCapsuleAvailability] Unsupported Encryption Algorithm!" << std::endl;
        return false;
    }
    }
}

bool Verifier::verifyDataCapsuleSignature(const json &dc, const std::string &producer_signature_public_key)
{
    // for test
    return true;
    // check if dc has MetaData, Data, Signature
    if (!dc.contains("MetaData") || !dc.contains("Data") || !dc.contains("Signature"))
    {
        std::cerr << "[verifyDataCapsuleSignature] Data Capsule is invalid!" << std::endl;
        return false;
    }

    std::string data = jsonToString(dc["MetaData"]) + jsonToString(dc["Data"]);
    // base64 decode the signature
    std::string signature = base64Decode(dc["Signature"]);
    if (verifyRSASignature(data, signature, producer_signature_public_key))
    {

#ifdef DEBUG
        std::cout << "[verifyDataCapsuleSignature] RSA Signature is valid!" << std::endl;
#endif
        return true;
    }

    std::cout << "[verifyDataCapsuleSignature] RSA Signature is invalid!" << std::endl;
    return false;
}

bool Verifier::verifyDataCapsuleSignature(const json &dc)
{
    // for test
    return true;

    // check if dc has MetaData, Data, Signature
    if (!dc.contains("MetaData") || !dc.contains("Data") || !dc.contains("Signature"))
    {
        std::cerr << "[verifyDataCapsuleSignature] Data Capsule is invalid!" << std::endl;
        return false;
    }

    std::string data = jsonToString(dc["MetaData"]) + jsonToString(dc["Data"]);
    // base64 decode the signature
    std::string signature = base64Decode(dc["Signature"]);
    if (verifyRSASignature(data, signature, producer_signature_public_key_))
    {
#ifdef DEBUG
        std::cout << "[verifyDataCapsuleSignature] RSA Signature is valid!" << std::endl;
#endif
        std::cout << "[verifyDataCapsuleSignature] RSA Signature is valid!" << std::endl;
        return true;
    }

    std::cout << "[verifyDataCapsuleSignature] RSA Signature is invalid!" << std::endl;
    return false;
}

static std::string computeTypeToString(ComputeType compute_type)
{
    switch (compute_type)
    {
    case ComputeType::DATA_STATISTICS:
        return "DataStatistics";
    case ComputeType::MACHINE_LEARNING:
        return "MachineLearning";
    case ComputeType::MODEL_FINETUNE:
        return "ModelTraining";
    case ComputeType::SQL_OPERATION:
        return "SQLOperation";
    case ComputeType::DATABASE_OPERATION:
        return "DatabaseOperation";
    case ComputeType::OFFICE_OPERATION:
        return "OfficeOperation";
    default:
        return "Unknown";
    }
}

bool Verifier::verifyDataCapsuleProgramPolicy(const json &program_policy, const json &access_policy)
{
    // user's access policy
    std::set<ComputeType> access_compute_types = access_policy["ComputeType"];

    // // program policy for the user
    // // std::set<ComputeType> program_compute_types = program_policy.get<std::map<std::string, json>>()[user_id_input_]["ComputeType"];
    // std::map<std::string, json> policy_map = program_policy.get<std::map<std::string, json>>();
    // // to check if the user_id_input_ exists in the program policy
    // if (policy_map.find(user_id_input_) == policy_map.end())
    // {
    //     std::cerr << "[verifyDataCapsuleProgramPolicy] User ID: " << user_id_input_ << " is not found in the Program Policy!" << std::endl;
    //     return false;
    // }
    // json user_policy = policy_map[user_id_input_];
    // // std::cout << "[verifyDataCapsuleProgramPolicy] user_policy: " << user_policy << std::endl;

    json user_policy = program_policy;

    std::set<ComputeType> program_compute_types = user_policy["ComputeType"];

    // check if the compute type in the access policy is allowed in the program policy
    for (const auto &access_compute_type : access_compute_types)
    {
        if (program_compute_types.find(access_compute_type) == program_compute_types.end())
        {
            std::cerr << "[verifyDataCapsuleProgramPolicy] Access Policy Compute Type: " << computeTypeToString(access_compute_type) << " is not allowed in the Program Policy!" << std::endl;
            return false;
        }
        std::cout << "[verifyDataCapsuleProgramPolicy] Compute Type: " << computeTypeToString(access_compute_type) << " is allowed in the Program Policy!" << std::endl;
    }

    // check if the sub task type in the access policy is allowed in the program policy
    for (const auto &access_compute_type : access_compute_types)
    {
        switch (access_compute_type)
        {
        case ComputeType::DATA_STATISTICS:
        {
            std::set<DataStatisticsType> access_statistics_types = access_policy["DataStatistics"]["Type"];
            std::set<DataStatisticsType> program_statistics_types = user_policy["DataStatistics"]["Type"];
            for (const auto &access_statistics_type : access_statistics_types)
            {
                // check if statistics type is allowed in the program policy
                if (program_statistics_types.find(access_statistics_type) == program_statistics_types.end())
                {
                    std::cerr << "[verifyDataCapsuleProgramPolicy] Access Policy Data Statistics Type is not allowed in the Program Policy!" << std::endl;
                    return false;
                }
                // check if column is allowed in the program policy
                switch (access_statistics_type)
                {
                case DataStatisticsType::COUNT:
                {
                    std::set<std::string> access_count_columns = access_policy["DataStatistics"]["Count"]["Column"];
                    std::set<std::string> program_count_columns = user_policy["DataStatistics"]["Count"]["Column"];
                    for (const auto &access_count_column : access_count_columns)
                    {
                        if (program_count_columns.find(access_count_column) == program_count_columns.end())
                        {
                            std::cerr << "[verifyDataCapsuleProgramPolicy] Access Policy Data Statistics Count Column is not allowed in the Program Policy!" << std::endl;
                            return false;
                        }
                    }
                    break;
                }
                case DataStatisticsType::MIN:
                {
                    std::set<std::string> access_min_columns = access_policy["DataStatistics"]["Min"]["Column"];
                    std::set<std::string> program_min_columns = user_policy["DataStatistics"]["Min"]["Column"];
                    for (const auto &access_min_column : access_min_columns)
                    {
                        if (program_min_columns.find(access_min_column) == program_min_columns.end())
                        {
                            std::cerr << "[verifyDataCapsuleProgramPolicy] Access Policy Data Statistics Min Column is not allowed in the Program Policy!" << std::endl;
                            return false;
                        }
                    }
                    break;
                }
                case DataStatisticsType::MAX:
                {
                    std::set<std::string> access_max_columns = access_policy["DataStatistics"]["Max"]["Column"];
                    std::set<std::string> program_max_columns = user_policy["DataStatistics"]["Max"]["Column"];
                    for (const auto &access_max_column : access_max_columns)
                    {
                        if (program_max_columns.find(access_max_column) == program_max_columns.end())
                        {
                            std::cerr << "[verifyDataCapsuleProgramPolicy] Access Policy Data Statistics Max Column is not allowed in the Program Policy!" << std::endl;
                            return false;
                        }
                    }
                    break;
                }
                case DataStatisticsType::AVG:
                {
                    std::set<std::string> access_avg_columns = access_policy["DataStatistics"]["Avg"]["Column"];
                    std::set<std::string> program_avg_columns = user_policy["DataStatistics"]["Avg"]["Column"];
                    for (const auto &access_avg_column : access_avg_columns)
                    {
                        if (program_avg_columns.find(access_avg_column) == program_avg_columns.end())
                        {
                            std::cerr << "[verifyDataCapsuleProgramPolicy] Access Policy Data Statistics Avg Column is not allowed in the Program Policy!" << std::endl;
                            return false;
                        }
                    }
                    break;
                }
                }
                break;
            }
            break;
        }
        case ComputeType::SQL_OPERATION:
        {
            // std::set<SQLOperationType> access_sql_types = access_policy["SQLOperation"]["Type"];
            // std::set<SQLOperationType> program_sql_types = user_policy["SQLOperation"]["Type"];
            // for (const auto &access_sql_type : access_sql_types)
            // {
            //     // check if sql type is allowed in the program policy
            //     if (program_sql_types.find(access_sql_type) == program_sql_types.end())
            //     {
            //         std::cerr << "[verifyDataCapsuleProgramPolicy] Access Policy SQL Operation Type is not allowed in the Program Policy!" << std::endl;
            //         return false;
            //     }
            //     // check if column is allowed in the program policy
            //     switch (access_sql_type)
            //     {
            //     case SQLOperationType::SELECT:
            //     {
            //         std::set<std::string> access_select_columns = access_policy["SQLOperation"]["Select"]["SelectColumns"];
            //         std::set<std::string> program_select_columns = user_policy["SQLOperation"]["Select"]["SelectColumns"];
            //         for (const auto &access_select_column : access_select_columns)
            //         {
            //             if (program_select_columns.find(access_select_column) == program_select_columns.end())
            //             {
            //                 std::cerr << "[verifyDataCapsuleProgramPolicy] Access Policy SQL Operation Select Column is not allowed in the Program Policy!" << std::endl;
            //                 return false;
            //             }
            //         }
            //         std::string access_filter_column = access_policy["SQLOperation"]["Select"]["FilterColumn"];
            //         std::string program_filter_column = user_policy["SQLOperation"]["Select"]["FilterColumn"];
            //         if (access_filter_column != program_filter_column)
            //         {
            //             std::cerr << "[verifyDataCapsuleProgramPolicy] Access Policy SQL Operation Select Filter Column is not allowed in the Program Policy!" << std::endl;
            //             return false;
            //         }
            //         break;
            //     }
            //     default:
            //     {
            //         std::cerr << "[verifyDataCapsuleProgramPolicy] Unsupported SQL Operation Type!" << std::endl;
            //         return false;
            //     }
            //     }
            //     break;
            // }
            //使用方改为交代码，就不需要在这里验证了
            break;
        }
        case ComputeType::DATABASE_OPERATION:
        {
            // for test
            break;
        }
        case ComputeType::MACHINE_LEARNING:
        {
            break;
        }
        case ComputeType::MODEL_FINETUNE:
        {
            break;
        }
        case ComputeType::PLAIN_TEXT:
        {
            break;
        }
        case ComputeType::OFFICE_OPERATION:
        {
            break;
        }
        default:
        {
            std::cerr << "[verifyDataCapsuleProgramPolicy] Unsupported Compute Type!" << std::endl;
            return false;
        }
        }
    }
    return true;
}