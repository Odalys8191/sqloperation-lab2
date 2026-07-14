#include "../../include/consumer/Executor.h"
#include "../../include/consumer/compute.h"
#include "../../include/common/aes256.openssl.h"
#include "../../include/common/base64.openssl.h"

/**
 * @brief Set the Data Capsule AES key
 * @param dc_key The AES key for the data capsule
 */
void Executor::setDCKey(const std::string &dc_key)
{
    this->dc_key_ = dc_key;
}

/**
 * @brief Decrypt the data in the data capsule using AES
 * @param dc The JSON object of the data capsule, must contain the "Data" field
 * @param iv The initialization vector for AES decryption
 * @param plain_text Output of the decrypted plaintext
 * @return true if decryption is successful, false otherwise
 */
bool Executor::decryptDataCapsuleDataAES(const json &dc, const std::string &iv, std::string &plain_text)
{
    // std::cout << "[decryptDataCapsuleDataAES] iv" << iv << std::endl;
    // std::cout << "[decryptDataCapsuleDataAES] iv size: " << iv.size() << std::endl;
    if (!dc.contains("Data"))
    {
        std::cerr << "[decryptDataCapsuleData] Data Capsule is invalid!" << std::endl;
        return false;
    }
    // decode the data from base64
    std::string data_base64_decoded = base64Decode(dc["Data"].get<std::string>());
    // decrypt the data
    plain_text = decryptAES256(data_base64_decoded, this->dc_key_, iv);
    return true;
}

/**
 * @brief Compute the result based on the access policy provided by data user
 * @param dc_id Data Capsule ID
 * @param plain_text The decrypted data capsule content
 * @param data_capsule_info The data capsule information (for office operation)
 * @param access_policy The access policy provided by data user
 * @param compute_result The computed result
 * @return true if the computation is successful, false otherwise
 */
bool Executor::computeResult(const std::string &dc_id, const std::string &plain_text, const json &data_capsule_info, const json &access_policy, std::string &compute_result, const json &program_policy)
{
    std::vector<ComputeType> compute_types = access_policy["ComputeType"];
    for (const auto &compute_type : compute_types)
    {
        switch (compute_type)
        {
        case ComputeType::DATA_STATISTICS:
        {
            if (!computeDataStatistics(access_policy, plain_text, compute_result))
            {
                std::cerr << "[computeResult] Failed to compute Data Statistics!" << std::endl;
                return false;
            }
            break;
        }
        case ComputeType::SQL_OPERATION:
        {
            // if (!computeSQLOperation(access_policy, plain_text, compute_result))
            // {
            //     std::cerr << "[computeResult] Failed to execute SQL Operation!" << std::endl;
            //     return false;
            // }
            if (!computecodeSQLOperation(access_policy, plain_text, compute_result, program_policy))
            {
                std::cerr << "[computeResult] Failed to execute SQL Operation!" << std::endl;
                return false;
            }
            break;
        }
        case ComputeType::DATABASE_OPERATION:
        {
            if (!computeDatabasedOperation(dc_id, access_policy, plain_text, compute_result))
            {
                std::cerr << "[computeResult] Failed to execute Database Operation!" << std::endl;
                return false;
            }
            break;
        }
        case ComputeType::MODEL_FINETUNE:
        {
            if (!computeModelFinetune(dc_id, access_policy, plain_text, compute_result))
            {
                std::cerr << "[computeResult] Failed to execute Model Finetune!" << std::endl;
                return false;
            }
            break;
        }
        case ComputeType::PLAIN_TEXT:
        {
            compute_result = plain_text;
            break;
        }
        case ComputeType::OFFICE_OPERATION:
        {
            if (!computeOfficeOperation(dc_id, access_policy, plain_text, data_capsule_info, compute_result))
            {
                std::cerr << "[computeResult] Failed to execute Office Operation!" << std::endl;
                return false;
            }
            break;
        }
        default:
        {
            break;
        }
        }
    }
    return true;
}

// ---------------------------------------------------------------------------------

// bool Executor::computeResult(const std::string &dc_id, const std::string &plain_text, const json &access_policy, std::string &compute_result)
// {
// #ifdef DEBUG
//     compute_result += "==========================================================================\n";
//     compute_result += "=========================== [compute Result] =============================\n";
//     compute_result += "==========================================================================\n\n";
// #endif
//     std::vector<ComputeType> compute_types = access_policy["ComputeType"];
//     for (const auto &compute_type : compute_types)
//     {
//         switch (compute_type)
//         {
//         case ComputeType::DATA_STATISTICS:
//         {
// #ifdef DEBUG
//             compute_result += "--------------------------------------------------------------------------\n";
//             compute_result += "------------------------ [Data Statistics Result] ------------------------\n";
//             compute_result += "--------------------------------------------------------------------------\n";
// #endif
//             if (!computeDataStatistics(access_policy, plain_text, compute_result))
//             {
//                 std::cerr << "[computeResult] Failed to compute Data Statistics!" << std::endl;
//                 return false;
//             }
// // std::cout << "[computeResult] Data Statistics Result: " << compute_result << std::endl;
// #ifdef DEBUG
//             compute_result += "--------------------------------------------------------------------------\n";
//             compute_result += "-------------------- [End of Data Statistics Result] ---------------------\n";
//             compute_result += "--------------------------------------------------------------------------\n";
// #endif
//             break;
//         }
//         case ComputeType::SQL_OPERATION:
//         {
// #ifdef DEBUG
//             compute_result += "--------------------------------------------------------------------------\n";
//             compute_result += "------------------------- [SQL Operation Result] -------------------------\n";
//             compute_result += "--------------------------------------------------------------------------\n";
// #endif
//             if (!computeSQLOperation(access_policy, plain_text, compute_result))
//             {
//                 std::cerr << "[computeResult] Failed to execute SQL Operation!" << std::endl;
//                 return false;
//             }
// #ifdef DEBUG
//             compute_result += "--------------------------------------------------------------------------\n";
//             compute_result += "--------------------- [End of SQL Operation Result] ----------------------\n";
//             compute_result += "--------------------------------------------------------------------------\n";
// #endif
//             break;
//         }
//         case ComputeType::DATABASE_OPERATION:
//         {
// #ifdef DEBUG
//             compute_result += "--------------------------------------------------------------------------\n";
//             compute_result += "---------------------- [Database Operation Result] -----------------------\n";
//             compute_result += "--------------------------------------------------------------------------\n";
// #endif
//             if (!computeDatabasedOperation(dc_id, access_policy, plain_text, compute_result))
//             {
//                 std::cerr << "[computeResult] Failed to execute Database Operation!" << std::endl;
//                 return false;
//             }
// #ifdef DEBUG
//             compute_result += "--------------------------------------------------------------------------\n";
//             compute_result += "------------------- [End of Database Operation Result] -------------------\n";
//             compute_result += "--------------------------------------------------------------------------\n";
// #endif
//             break;
//         }
//         case ComputeType::MODEL_FINETUNE:
//         {
//             if (!computeModelFinetune(dc_id, access_policy, plain_text, compute_result))
//             {
//                 std::cerr << "[computeResult] Failed to execute Model Finetune!" << std::endl;
//                 return false;
//             }
//             break;
//         }
//         case ComputeType::PLAIN_TEXT:
//         {
//             compute_result = plain_text;
//             break;
//         }
//         case ComputeType::OFFICE_OPERATION:
//         {
//             // Office operation requires data capsule info, cannot be processed here
//             break;
//         }
//         default:
//         {
//             break;
//         }
//         }
//     }
// #ifdef DEBUG
//     compute_result += "\n==========================================================================\n";
//     compute_result += "======================== [End of compute Result] =========================\n";
//     compute_result += "==========================================================================\n\n";
// #endif
//     return true;
// }