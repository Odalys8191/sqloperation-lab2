#include "../../include/consumer/api.h"
#include "../../include/consumer/Verifier.h"
#include "../../include/consumer/Executor.h"
#include "../../include/consumer/compute.h"
#include "../../include/common/tools.h"
#include "../../include/common/encryptAlgType.h"
#include "../../include/common/base64.openssl.h"
#include "../../include/common/abe/CPABE.h"
#include "../../include/common/abeKeys.h"
#include "../../include/common/aes256.openssl.h"

/* APIs for cloud storage interaction */
// get the dc from cloud storage
bool getDataCapsule(const std::string &dc_id,
                    const std::string &cloud_server_ip,
                    const std::string &cloud_server_port,
                    json &data_capsule)
{
    Client client_cloud(grpc::CreateChannel(cloud_server_ip + ":" + cloud_server_port, grpc::InsecureChannelCredentials()));
    try
    {
        // 示例：获取数据胶囊
        if (client_cloud.GetDataCapsule(dc_id, data_capsule))
        {
#ifdef DEBUG
            std::cout << "[getDataCapsule] Data Capsule: " << data_capsule << std::endl;
#endif
            return true;
        }
        else
        {
            std::cerr << "[getDataCapsule] Failed to get data capsule!" << std::endl;
            return false;
        }
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

// get the dc's encrypted password from cloud storage
bool getEncryptedDataCapsulePasswordProgramPolicy(const std::string &dc_id,
                                                  const std::string &cloud_server_ip,
                                                  const std::string &cloud_server_port,
                                                  std::string &encrypted_password,
                                                  json &program_policy)
{
    Client client_cloud(grpc::CreateChannel(cloud_server_ip + ":" + cloud_server_port, grpc::InsecureChannelCredentials()));
    try
    {
        // 获取加密后的数据胶囊密码
        //         if (client_cloud.GetEncryptedDataCapsulePassword(dc_id, encrypted_password))
        //         {
        // #ifdef DEBUG
        //             std::cout << "[getEncryptedDataCapsulePasswordProgramPolicy] Encrypted Data Capsule Password (Base64 Encoded): " << encrypted_password << std::endl;
        // #endif
        //             // std::cout << "[getEncryptedDataCapsulePasswordProgramPolicy] Encrypted Data Capsule Password (Decoded): " << base64Decode(encrypted_password) << std::endl;
        //         }
        //         else
        //         {
        //             std::cerr << "[getEncryptedDataCapsulePasswordProgramPolicy] Failed to get encrypted data capsule password!" << std::endl;
        //             return false;
        //         }
        // 获取数据胶囊访问策略
        if (client_cloud.GetProgramPolicy(dc_id, program_policy))
        {
#ifdef DEBUG
            std::cout << "[getEncryptedDataCapsulePasswordProgramPolicy] Program Policy: " << program_policy << std::endl;
#endif
        }
        else
        {
            std::cerr << "[getEncryptedDataCapsulePasswordProgramPolicy] Failed to get program policy!" << std::endl;
            return false;
        }
        return true;
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

/* APIs for CertificateAuthority interaction */

// initialize TEE
bool initializeTEE(const std::string &signature_public_key,
                   const std::string &ca_server_ip,
                   const std::string &ca_server_port, std::string &tee_id, std::string &certificate)
{
    Client client_ca(grpc::CreateChannel(ca_server_ip + ":" + ca_server_port, grpc::InsecureChannelCredentials()));
    try
    {
        // 示例：初始化 TEE
        if (client_ca.InitializeTEE(signature_public_key, tee_id, certificate))
        {
            std::cout << "[initializeTEE] Initialized TEE with ID: " << tee_id << std::endl;
            return true;
        }
        else
        {
            std::cerr << "[initializeTEE] Failed to initialize TEE!" << std::endl;
            return false;
        }
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

// // get the dc's data key (password key) from the CertificateAuthority
// bool getDataCapsuleKeyWithAdditionalInformation(const std::string &dc_id,
//                                                 const std::string &ca_server_ip,
//                                                 const std::string &ca_server_port,
//                                                 const std::string &cloud_server_ip,
//                                                 const std::string &cloud_server_port,
//                                                 json &dc_key_info)
// {
//     Client client_ca(grpc::CreateChannel(ca_server_ip + ":" + ca_server_port, grpc::InsecureChannelCredentials()));
//     Client client_cloud(grpc::CreateChannel(cloud_server_ip + ":" + cloud_server_port, grpc::InsecureChannelCredentials()));
//     try
//     {
//         // 从 ca 获取数据胶囊密钥
//         std::string dc_key{};
//         if (client_ca.GetDataCapsuleKey(dc_id, dc_key))
//         {
//             dc_key_info["EncryptKey"] = dc_key;
// #ifdef DEBUG
//             std::cout << "[getDataCapsuleKeyWithAdditionalInformation] Data Capsule Encrypt Key: ";
//             for (const auto &c : dc_key)
//             {
//                 std::cout << (int)c << " ";
//             }
//             std::cout << std::endl;
// #endif
//         }
//         else
//         {
//             std::cerr << "[getDataCapsuleKeyWithAdditionalInformation] Failed to get data capsule key!" << std::endl;
//             return false;
//         }
//         // 从cloud server获取解密相关信息
//         json dc_key_additional_info;
//         if (client_cloud.GetDataCapsuleKeyAdditionalInformation(dc_id, dc_key_additional_info))
//         {
// #ifdef DEBUG
//             std::cout << "[getDataCapsuleKeyWithAdditionalInformation] Data Capsule Key Additional Information: " << dc_key_additional_info << std::endl;
// #endif
//             dc_key_info["SymmetricEncryption"] = dc_key_additional_info;
//         }
//         else
//         {
//             std::cerr << "[getDataCapsuleKeyWithAdditionalInformation] Failed to get data capsule key additional information!" << std::endl;
//             return false;
//         }
//         return true;
//     }
//     catch (const std::runtime_error &e)
//     {
//         std::cerr << e.what() << std::endl;
//         return false;
//     }
// }

// get the Producer(who produces the dc)'s signature public key from the CertificateAuthority
bool getProducerSignaturePublicKey(const std::string &tee_id,
                                   const std::string &ca_server_ip,
                                   const std::string &ca_server_port, std::string &producer_signature_public_key)
{
    Client client_ca(grpc::CreateChannel(ca_server_ip + ":" + ca_server_port, grpc::InsecureChannelCredentials()));
    try
    {
        // 示例：获取生产者签名公钥
        if (client_ca.GetProducerSignaturePublicKey(tee_id, producer_signature_public_key))
        {
#ifdef DEBUG
            std::cout << "[getProducerSignaturePublicKey] Producer Signature Public Key: " << producer_signature_public_key << std::endl;
#endif
            return true;
        }
        else
        {
            std::cerr << "[getProducerSignaturePublicKey] Failed to get producer signature public key!" << std::endl;
            return false;
        }
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

/* APIs for etcd interaction */

// get the dc's state from etcd key-value storage
bool getDataCapsuleState(const std::string &dc_id,
                         const std::string &etcd_server_ip,
                         const std::string &etcd_server_port, json &state)
{
    Client client_etcd(grpc::CreateChannel(etcd_server_ip + ":" + etcd_server_port, grpc::InsecureChannelCredentials()));
    try
    {
        // 示例：获取数据胶囊state信息
        if (client_etcd.GetDataCapsuleState(dc_id, state))
        {
#ifdef DEBUG
            std::cout << "[getDataCapsuleState] Data Capsule State: " << state << std::endl;
#endif
            return true;
        }
        else
        {
            std::cerr << "[getDataCapsuleState] Failed to get data capsule state!" << std::endl;
            return false;
        }
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

// update the dc's state to etcd key-value storage
bool UpdateDataCapsuleState(const std::string &dc_id,
                            const json &state,
                            const std::string &etcd_server_ip,
                            const std::string &etcd_server_port)
{
    // json new_state;
    // new_state["OwnerID"] = state["OwnerID"].get<std::string>();
    // new_state["UsableTimes"] = state["UsableTimes"].get<int>() - 1;

    Client client_etcd(grpc::CreateChannel(etcd_server_ip + ":" + etcd_server_port, grpc::InsecureChannelCredentials()));
    try
    {
        if (client_etcd.UpdateDataCapsuleState(dc_id, state))
        {
#ifdef DEBUG
            std::cout << "[UpdateDataCapsuleState] Update State Success!" << std::endl;
#endif
            return true;
        }
        else
        {
            std::cerr << "[UpdateDataCapsuleState] Failed to update data capsule state!" << std::endl;
            return false;
        }
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

/* APIs for Intergrated operations */

bool consumeDataCapsule(const std::string &user_id_input, // TODO
                        const std::string &dc_id_input,
                        const std::string &dc_password_input,
                        const json &access_policy,
                        std::string &compute_result,
                        const std::string &cloud_server_ip,
                        const std::string &cloud_server_port,
                        const std::string &ca_server_ip,
                        const std::string &ca_server_port,
                        const std::string &etcd_server_ip,
                        const std::string &etcd_server_port,
                        std::string *error_message)
{
    compute_result = "";
    if (error_message)
    {
        error_message->clear();
    }

    // ------------------------------------------------------------------------------------------------------------------
    // Verify Availability
    // 1.verify the <password> (compare to the input password_input) -> to check if the dc is available for the user
    // 2.verify the dc's <state> -> to check if the dc is still valid
    // 3.verify the <access policy> -> to check if the user uses the dc in a valid way
    // ------------------------------------------------------------------------------------------------------------------
    Verifier verifier(user_id_input, dc_id_input, dc_password_input);
#ifdef DEBUG
    std::cout << "[consumeDataCapsule] User ID: " << user_id_input << ", DC ID: " << dc_id_input << ", DC Password: " << dc_password_input << std::endl;
#endif
    // get the dc's <encrypted_password, program policy> from cloud storage
    std::string encrypted_password;
    json program_policy; // dc's policy
    if (!getEncryptedDataCapsulePasswordProgramPolicy(dc_id_input, cloud_server_ip, cloud_server_port, encrypted_password, program_policy))
    {
        std::cerr << "[consumeDataCapsule] Failed to get encrypted data capsule password!" << std::endl;
        return false;
    }

    // // get dc's data <key> (the password key) from the CertificateAuthority
    // json dc_key_info;
    // if (!getDataCapsuleKeyWithAdditionalInformation(dc_id_input, ca_server_ip, ca_server_port, cloud_server_ip, cloud_server_port, dc_key_info))
    // {
    //     std::cerr << "[consumeDataCapsule] Failed to get data capsule key!" << std::endl;
    //     return false;
    // }
    // verifier.setDCKey(dc_key_info["EncryptKey"].get<std::string>());

    // get dc's <state> from etcd
    json state;
    if (!getDataCapsuleState(dc_id_input, etcd_server_ip, etcd_server_port, state))
    {
        std::cerr << "[consumeDataCapsule] Failed to get data capsule state!" << std::endl;
        return false;
    }

#ifdef DEBUG
    std::cout << "[consumeDataCapsule] Data Capsule Key Info: " << dc_key_info << std::endl;
#endif

    // verify the password
    // if (!verifier.verifyDataCapsuleAvailability(base64Decode(encrypted_password), dc_key_info))
    // {
    //     std::cerr << "[consumeDataCapsule] Data Capsule is not available: Password is invalid!" << std::endl;
    //     return false;
    // }

    // verify the state
    if (!verifier.verifyDataCapsuleState(state))
    {
        std::cerr << "[consumeDataCapsule] Data Capsule is not available: Usable Times is Zero!" << std::endl;
        return false;
    }

    // verify the access policy
    if (!verifier.verifyDataCapsuleProgramPolicy(program_policy, access_policy))
    {
        std::cerr << "[consumeDataCapsule] Access Policy is not satisfied!" << std::endl;
        return false;
    }
    std::cout << "[consumeDataCapsule] Access Policy is satisfied!" << std::endl;

    // Availability Verification passed
    // get the <whole data capsule> from cloud storage
    json dc;
    if (!getDataCapsule(dc_id_input, cloud_server_ip, cloud_server_port, dc))
    {
        std::cerr << "[consumeDataCapsule] Failed to get data capsule!" << std::endl;
        return false;
    }
    std::cout << "[consumeDataCapsule] Data Capsule Retrieved Successfully!" << std::endl;

    // ---------------------------------------------------------------------------------------------------------------------
    // Verify data capsule's Signature
    // ---------------------------------------------------------------------------------------------------------------------
    std::string producer_signature_public_key;
    std::string tee_id = dc["MetaData"]["PTEEID"].get<std::string>();
    // get the dc Producer's <Public Key> from the CertificateAuthority
    if (!getProducerSignaturePublicKey(tee_id, ca_server_ip, ca_server_port, producer_signature_public_key))
    {
        std::cerr << "[consumeDataCapsule] Failed to get producer signature public key!" << std::endl;
        return false;
    }
    std::cout << "[consumeDataCapsule] Producer Signature Public Key: " << producer_signature_public_key << std::endl;
    verifier.setProducerSignaturePublicKey(producer_signature_public_key);

    // verify the dc's signature
    if (!verifier.verifyDataCapsuleSignature(dc))
    {
        std::cerr << "[consumeDataCapsule] Data capsule signature is invalid!" << std::endl;
        return false;
    }
    std::cout << "[consumeDataCapsule] Data Capsule Signature Verified Successfully!" << std::endl;

    // -------------------------------------------------------------------------------------------------------------------
    // Execute Computation
    // -------------------------------------------------------------------------------------------------------------------

    // -----------------------------------------------------------------------------------------------------------------
    // ABE decryption to get the symmetric key
    // 1. get the user's ABE private key
    // 2. get the dc's symmetric key ciphertext from dc
    // 3. use the user's ABE private key to decrypt the dc's symmetric key ciphertext
    // 4. use the symmetric key to decrypt the dc's data ciphertext
    // -----------------------------------------------------------------------------------------------------------------
    pairing_t pairing;
    init_pairing(pairing, PUBLIC_PARAM_PATH); // 假设你有一个pairing参数文件
    CP_ABE_SK abe_sk2;
    init_CP_ABE_SK(abe_sk2, 10);                          // 第二层私钥
    deSerializeFromFile(abe_sk2, USER_SK2_PATH, pairing); // 第二层私钥
    int Access1[MAX_ACCESS_SIZE];
    int Access2[MAX_ACCESS_SIZE];
    std::string accessStructure1 = dc["MetaData"]["ABEEncryption"]["AccessStructure"]["1_convert"].get<std::string>();
    std::string accessStructure2 = dc["MetaData"]["ABEEncryption"]["AccessStructure"]["2_convert"].get<std::string>();
    std::cout << "[TEST][ABE] AccessStructure1: " << accessStructure1 << std::endl;
    std::cout << "[TEST][ABE] AccessStructure2: " << accessStructure2 << std::endl;
    parseAccessString(accessStructure1, Access1);
    parseAccessString(accessStructure2, Access2);
    // 1. 解密第二层ABE密文，获得对称密钥key2
    json ct2_json = dc["MetaData"]["ABEEncryption"];
    CT ct2 = jsonToCT(ct2_json, pairing);
    std::cout << "[TEST][ABE] ct2_json: " << ct2_json << std::endl;
    unsigned char *key2 = new unsigned char[32];
    try
    {
        cpabe_Decrypt(Access2, pairing, ct2, abe_sk2, key2); // 用第二层ABE私钥解密得到key2
    }
    catch (const std::exception &e)
    {
        if (error_message)
        {
            *error_message = e.what();
        }
        std::cerr << "[consumeDataCapsule] ABE decrypt stage-2 failed: " << e.what() << std::endl;
        delete[] key2;
        return false;
    }
    std::cout << "[TEST][ABE] key2(hex): ";
    for (int i = 0; i < 32; ++i)
        printf("%02x", key2[i]);
    std::cout << std::endl;
    // 2. 用key2和IV解密ct1（AES密文）
    std::string ciphertext_ct1_base64 = dc["Ciphertext_ct1"];
    std::string iv_ct1_base64 = dc["MetaData"]["SymmetricEncryption"]["AESIV_ct1"];
    std::string ciphertext_ct1 = base64Decode(ciphertext_ct1_base64);
    std::string iv_ct1 = base64Decode(iv_ct1_base64);
    std::cout << "[TEST][ABE] ciphertext_ct1_base64: " << ciphertext_ct1_base64 << std::endl;
    std::cout << "[TEST][ABE] iv_ct1_base64: " << iv_ct1_base64 << std::endl;
    std::string ct1_serialized = decryptAES256(ciphertext_ct1, std::string(reinterpret_cast<char *>(key2), 32), iv_ct1);
    std::cout << "[TEST][ABE] ct1_serialized.size(): " << ct1_serialized.size() << std::endl;
    CT ct1;
    cpabe_DeserializeCT(reinterpret_cast<const unsigned char *>(ct1_serialized.data()), ct1_serialized.size(), ct1, pairing);

    json sk_json = access_policy["SK"];
    std::cout << "[TEST][ABE] abe_sk1: " << sk_json << std::endl;
    CP_ABE_SK abe_sk1 = jsonToCPABESK(sk_json, pairing);

    unsigned char *dec_key = new unsigned char[32];
    try
    {
        cpabe_Decrypt(Access1, pairing, ct1, abe_sk1, dec_key);
    }
    catch (const std::exception &e)
    {
        if (error_message)
        {
            *error_message = e.what();
        }
        std::cerr << "[consumeDataCapsule] ABE decrypt stage-1 failed: " << e.what() << std::endl;
        delete[] key2;
        delete[] dec_key;
        return false;
    }
    std::cout << "解密得到对称密钥key1: " << std::string(reinterpret_cast<char *>(dec_key), 32) << std::endl;
    std::string dec_key_str(reinterpret_cast<char *>(dec_key), 32);
    delete[] key2;
    delete[] dec_key;
    // -----------------------------------------------------------------------------------------------------------------

    Executor executor(dec_key_str);

    // Decrypt the dc's data
    std::string plain_text;
    EncryptAlgType crypto_alg = dc["MetaData"]["SymmetricEncryption"]["CryptoAlg"];
    switch (crypto_alg)
    {
    case EncryptAlgType::Encrypt_AES256_CBC:
    {
        std::string iv = base64Decode(dc["MetaData"]["SymmetricEncryption"]["AESIV_data"].get<std::string>());

        if (!executor.decryptDataCapsuleDataAES(dc, iv, plain_text))
        {
            std::cerr << "[consumeDataCapsule] Failed to decrypt data!" << std::endl;
            return false;
        }
        break;
    }
    default:
    {
        std::cerr << "[consumeDataCapsule] Unsupported Crypto Algorithm: " << dc["MetaData"]["SymmetricEncryption"]["CryptoAlg"] << "!" << std::endl;
        return false;
    }
    }
    std::cout << "[consumeDataCapsule] Data Decrypted Successfully!" << std::endl;

    // build data_capsule_info for Office Operation
    json data_capsule_info = {};
    data_capsule_info["ownerID"] = state["OwnerID"].get<std::string>();
    data_capsule_info["usableTimes"] = 1;
    data_capsule_info["dataDescription"] = dc.value(json::json_pointer("/MetaData/DataIntro/DataDescription"), "");
    if (dc.contains("MetaData") && dc["MetaData"].contains("Policy") && dc["MetaData"]["Policy"].contains("ComputeType"))
    {
        data_capsule_info["selectedCategories"] = dc["MetaData"]["Policy"]["ComputeType"].get<std::vector<int>>();
    }
    else
    {
        data_capsule_info["selectedCategories"] = dc.value(json::json_pointer("/MetaData/DataIntro/SelectedCategories"), std::vector<int>{});
    }
    data_capsule_info["OfficeOperation"] = dc.value(json::json_pointer("/MetaData/Policy/OfficeOperation"), json::object());
    if (dc.contains("MetaData") && dc["MetaData"].contains("ABEEncryption"))
    {
        json parent_access = dc["MetaData"]["ABEEncryption"].value("AccessStructure", json::object());
        if (parent_access.contains("1"))
        {
            data_capsule_info["AccessStructure"]["1"] = parent_access["1"];
        }
        if (parent_access.contains("2"))
        {
            data_capsule_info["AccessStructure"]["2"] = parent_access["2"];
        }
    }

    // execute non-office computation first; defer office docker start until state update succeeds
    std::vector<int> requested_compute_types = access_policy.value("ComputeType", std::vector<int>{});
    std::vector<int> non_office_compute_types;
    bool need_office_operation = false;
    const int office_compute_type = static_cast<int>(ComputeType::OFFICE_OPERATION);
    for (int compute_type : requested_compute_types)
    {
        if (compute_type == office_compute_type)
        {
            need_office_operation = true;
        }
        else
        {
            non_office_compute_types.push_back(compute_type);
        }
    }

    std::cout << "[consumeDataCapsule] Consuming Data Capsule..." << std::endl;
    if (!non_office_compute_types.empty())
    {
        json non_office_policy = access_policy;
        non_office_policy["ComputeType"] = non_office_compute_types;
        if (!executor.computeResult(dc_id_input, plain_text, data_capsule_info, non_office_policy, compute_result, program_policy))
        {
            if (error_message)
            {
                if (!compute_result.empty())
                {
                    *error_message = compute_result;
                }
                else
                {
                    *error_message = "compute execution failed";
                }
            }
            std::cerr << "[consumeDataCapsule] Failed to compute non-office result!" << std::endl;
            return false;
        }
        std::cout << "[consumeDataCapsule] Non-office computation result: " << compute_result << std::endl;
    }

    // ------------------------------------------------------------------------------------------------------------------
    // update the dc's <state> to etcd
    // ------------------------------------------------------------------------------------------------------------------
    json new_state;
    new_state["OwnerID"] = state["OwnerID"].get<std::string>();
    new_state["UsableTimes"] = state["UsableTimes"].get<int>() - 1;
    if (!UpdateDataCapsuleState(dc_id_input, new_state, etcd_server_ip, etcd_server_port))
    {
        std::cerr << "[consumeDataCapsule] Failed to update data capsule state!" << std::endl;
        return false;
    }
    std::cout << "[consumeDataCapsule] Data Capsule State Updated Successfully!" << std::endl;

    if (need_office_operation)
    {
        std::string office_result;
        if (!computeOfficeOperation(dc_id_input, access_policy, plain_text, data_capsule_info, office_result))
        {
            if (error_message)
            {
                if (!office_result.empty())
                {
                    *error_message = office_result;
                }
                else
                {
                    *error_message = "office operation failed";
                }
            }
            std::cerr << "[consumeDataCapsule] Failed to execute deferred office operation!" << std::endl;
            return false;
        }
        if (!compute_result.empty())
        {
            compute_result += "\n";
        }
        compute_result += office_result;
    }

    compute_result += new_state.dump(4) + "\n";

#ifdef DEBUG
    std::cout << "\n[consumeDataCapsule] Compute Result: \n"
              << compute_result << std::endl;
#endif

    return true;
}
