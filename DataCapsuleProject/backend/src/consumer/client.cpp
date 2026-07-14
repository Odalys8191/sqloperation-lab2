#include "../../include/common/tools.h"
#include "../../include/consumer/client.h"
#include "../../include/common/signature.openssl.h"

// 调用 CloudStorageService 的方法
bool Client::GetDataCapsule(const std::string &dc_id, json &data_capsule)
{
    serverInteraction::GetDataCapsuleRequest request;
    request.set_dc_id(dc_id);

    serverInteraction::GetDataCapsuleResponse response;
    grpc::ClientContext context;

    grpc::Status status = cloud_stub_->GetDataCapsule(&context, request, &response);
    if (status.ok())
    {
        data_capsule = stringToJson(response.data_capsule());
        return true;
    }
    else
    {
        std::cerr << "[Client::GetDataCapsule] Failed to get data capsule: " << status.error_message() << std::endl;
        return false;
    }
}

bool Client::GetProgramPolicy(const std::string &dc_id, json &program_policy)
{
    serverInteraction::GetProgramPolicyRequest request;
    request.set_dc_id(dc_id);

    serverInteraction::GetProgramPolicyResponse response;
    grpc::ClientContext context;

    grpc::Status status = cloud_stub_->GetProgramPolicy(&context, request, &response);
    if (status.ok())
    {
        program_policy = stringToJson(response.program_policy());
        return true;
    }
    else
    {
        std::cerr << "[Client::GetProgramPolicy] Failed to get program policy: " << status.error_message() << std::endl;
        return false;
    }
}

// 调用 EtcdService 的方法
bool Client::GetDataCapsuleState(const std::string &dc_id, json &state)
{
    serverInteraction::GetDataCapsuleStateRequest request;
    request.set_dc_id(dc_id);

    serverInteraction::GetDataCapsuleStateResponse response;
    grpc::ClientContext context;

    grpc::Status status = etcd_stub_->GetDataCapsuleState(&context, request, &response);
    if (status.ok())
    {
        state = stringToJson(response.state());
        return true;
    }
    else
    {
        std::cerr << "[Client::GetDataCapsuleState] Failed to get data capsule state: " << status.error_message() << std::endl;
        return false;
    }
}

bool Client::UpdateDataCapsuleState(const std::string &dc_id, const json &state)
{
    serverInteraction::UpdateDataCapsuleStateRequest request;
    request.set_dc_id(dc_id);
    request.set_state(jsonToString(state));

    serverInteraction::UpdateDataCapsuleStateResponse response;
    grpc::ClientContext context;

    grpc::Status status = etcd_stub_->UpdateDataCapsuleState(&context, request, &response);
    if (status.ok())
    {
        return response.success();
    }
    else
    {
        std::cerr << "[Client::UpdateDataCapsuleState] Failed to update data capsule state: " << status.error_message() << std::endl;
        return false;
    }
}

// 调用 CertificateAuthorityService 的方法

bool Client::InitializeTEE(const std::string &public_key, std::string &tee_id, std::string &certificate)
{
    serverInteraction::InitializeTEERequest request;
    request.set_public_key(public_key);
    serverInteraction::InitializeTEEResponse response;
    grpc::ClientContext context;

    grpc::Status status = ca_stub_->InitializeTEE(&context, request, &response);
    if (status.ok())
    {
        tee_id = response.tee_id();
        certificate = response.sign_certificate();
        // std::cout << "Initialized TEE with ID: " << tee_id << std::endl;
        return true;
    }
    else
    {
        std::cerr << "[Client::InitializeTEE] Failed to initialize TEE: " << status.error_message() << std::endl;
        return false;
    }
}

bool Client::GetProducerSignaturePublicKey(const std::string &tee_id, std::string &producer_signature_public_key)
{
    serverInteraction::GetProducerSignaturePublicKeyRequest request;
    request.set_tee_id(tee_id);
    serverInteraction::GetProducerSignaturePublicKeyResponse response;
    grpc::ClientContext context;

    grpc::Status status = ca_stub_->GetProducerSignaturePublicKey(&context, request, &response);
    if (status.ok())
    {
        producer_signature_public_key = response.public_key();
        // std::cout << "Public Key: " << producer_signature_public_key << std::endl;
        return true;
    }
    else
    {
        std::cerr << "[Client::GetProducerSignaturePublicKey] Failed to get public key: " << status.error_message() << std::endl;
        return false;
    }
}

// -------------------------------- useless functions --------------------------------

// bool Client::GetEncryptedDataCapsulePassword(const std::string &dc_id, const std::string &user_id, std::string &encrypted_dc_password)
// {
//     serverInteraction::GetEncryptedDataCapsulePasswordRequest request;
//     request.set_dc_id(dc_id);
//     request.set_user_id(user_id);

//     serverInteraction::GetEncryptedDataCapsulePasswordResponse response;
//     grpc::ClientContext context;

//     grpc::Status status = cloud_stub_->GetEncryptedDataCapsulePassword(&context, request, &response);
//     if (status.ok())
//     {
//         encrypted_dc_password = response.encrypted_dc_password();
//         return true;
//     }
//     else
//     {
//         std::cerr << "[Client::GetEncryptedDataCapsulePassword] Failed to get encrypted data capsule password: " << status.error_message() << std::endl;
//         return false;
//     }
// }

// bool Client::GetEncryptedDataCapsulePassword(const std::string &dc_id, std::string &encrypted_dc_password)
// {
//     serverInteraction::GetEncryptedDataCapsulePasswordRequest request;
//     request.set_dc_id(dc_id);

//     serverInteraction::GetEncryptedDataCapsulePasswordResponse response;
//     grpc::ClientContext context;

//     grpc::Status status = cloud_stub_->GetEncryptedDataCapsulePassword(&context, request, &response);
//     if (status.ok())
//     {
//         encrypted_dc_password = response.encrypted_dc_password();
//         return true;
//     }
//     else
//     {
//         std::cerr << "[Client::GetEncryptedDataCapsulePassword] Failed to get encrypted data capsule password: " << status.error_message() << std::endl;
//         return false;
//     }
// }

// bool Client::GetDataCapsuleKey(const std::string &dc_id, std::string &data_capsule_key)
// {
//     serverInteraction::GetDataCapsuleKeyRequest request;
//     request.set_dc_id(dc_id);
//     serverInteraction::GetDataCapsuleKeyResponse response;
//     grpc::ClientContext context;

//     grpc::Status status = ca_stub_->GetDataCapsuleKey(&context, request, &response);
//     if (status.ok())
//     {
//         data_capsule_key = response.dc_key();
//         return true;
//     }
//     else
//     {
//         std::cerr << "[Client::GetDataCapsuleKey] Failed to get DC key: " << status.error_message() << std::endl;
//         return false;
//     }
// }

// bool Client::GetDataCapsuleKeyAdditionalInformation(const std::string &dc_id, json &dc_key_additional_info)
// {
//     serverInteraction::GetDataCapsuleKeyAdditionalInformationRequest request;
//     request.set_dc_id(dc_id);

//     serverInteraction::GetDataCapsuleKeyAdditionalInformationResponse response;
//     grpc::ClientContext context;

//     grpc::Status status = cloud_stub_->GetDataCapsuleKeyAdditionalInformation(&context, request, &response);
//     if (status.ok())
//     {
//         dc_key_additional_info = stringToJson(response.additional_information());
//         // std::cout << "[Client::GetDataCapsuleKeyAdditionalInformation] Additional information: " << dc_key_additional_info << std::endl;
//         return true;
//     }
//     else
//     {
//         std::cerr << "[Client::GetDataCapsuleKeyAdditionalInformation] Failed to get key additional information: " << status.error_message() << std::endl;
//         return false;
//     }
// }

// void Client::TestTransfer(const std::string &data)
// {
//     std::cout << "------------------------- [Test Transfer] -------------------------" << std::endl;
//     serverInteraction::TestTransferRequest request;
//     request.set_data(data);
//     serverInteraction::TestTransferResponse response;
//     grpc::ClientContext context;

//     grpc::Status status = ca_stub_->TestTransfer(&context, request, &response);
//     if (status.ok())
//     {
//         RSASignature rsa;
//         rsa.loadPublicKey(response.public_key());
//         std::cout << "Public Key: " << response.public_key() << std::endl;
//         std::string rsa_signature = response.signature();
//         std::cout << "Signature: " << rsa_signature << std::endl;
//         if (rsa.verify(data, rsa_signature))
//         {
//             std::cout << "RSA Signature is valid!" << std::endl;
//         }
//         else
//         {
//             std::cout << "RSA Signature is invalid!" << std::endl;
//         }
//         std::cout << "Test Data: " << response.test_data() << std::endl;
//     }
//     else
//     {
//         std::cerr << "[Client::TestTransfer] Failed to test signature: " << status.error_message() << std::endl;
//     }
// }

// For test

// void testServerInteraction(const std::string &ca_server_ip, const std::string &ca_server_port,
//                            const std::string &etcd_server_ip, const std::string &etcd_server_port,
//                            const std::string &cloud_server_ip, const std::string &cloud_server_port,
//                            const std::string &dc_id)
// {
//     std::cout << "------------------------- [testServerInteraction] -------------------------" << std::endl;
//     std::cout << "------------------------- Cloud Server -------------------------" << std::endl;
//     Client client_cloud(grpc::CreateChannel(cloud_server_ip + ":" + cloud_server_port, grpc::InsecureChannelCredentials()));
//     try
//     {
//         // 示例：获取数据胶囊
//         json data_capsule;
//         if (client_cloud.GetDataCapsule(dc_id, data_capsule))
//         {
//             std::cout << "[GetDataCapsule] Data Capsule: " << data_capsule << std::endl;
//         }
//         else
//         {
//             std::cerr << "[GetDataCapsule] Failed to get data capsule!" << std::endl;
//             throw std::runtime_error("[GetDataCapsule] Failed to get data capsule!");
//         }

//         // // 示例：获取加密后的数据胶囊密码
//         // std::string encrypted_password;
//         // if (client_cloud.GetEncryptedDataCapsulePassword(dc_id, encrypted_password))
//         // {
//         //     std::cout << "[GetEncryptedDataCapsulePassword] Encrypted Data Capsule Password: " << encrypted_password << std::endl;
//         // }
//         // else
//         // {
//         //     std::cerr << "[GetEncryptedDataCapsulePassword] Failed to get encrypted data capsule password!" << std::endl;
//         //     throw std::runtime_error("[GetEncryptedDataCapsulePassword] Failed to get encrypted data capsule password!");
//         // }
//     }
//     catch (const std::runtime_error &e)
//     {
//         std::cerr << e.what() << std::endl;
//     }

//     std::cout << "------------------------- Etcd Server -------------------------" << std::endl;
//     Client client_etcd(grpc::CreateChannel(etcd_server_ip + ":" + etcd_server_port, grpc::InsecureChannelCredentials()));
//     try
//     {
//         // 示例：获取数据胶囊state信息
//         json state;
//         if (client_etcd.GetDataCapsuleState(dc_id, state))
//         {
//             std::cout << "[GetDataCapsuleState] Data Capsule State: " << state << std::endl;
//         }
//         else
//         {
//             std::cerr << "[GetDataCapsuleState] Failed to get data capsule state!" << std::endl;
//             // throw error
//             throw std::runtime_error("[GetDataCapsuleState] Failed to get data capsule state!");
//         }

//         // 示例：更新数据胶囊state信息
//         json new_state;
//         new_state["OwnerID"] = state["OwnerID"].get<std::string>();
//         new_state["UsableTimes"] = state["UsableTimes"].get<int>() - 1;
//         if (client_etcd.UpdateDataCapsuleState(dc_id, new_state))
//         {
//             std::cout << "[UpdateDataCapsuleState] Update State Success!" << std::endl;
//         }
//         else
//         {
//             std::cerr << "[UpdateDataCapsuleState] Failed to update data capsule state!" << std::endl;
//             // throw error
//             throw std::runtime_error("[UpdateDataCapsuleState] Failed to update data capsule state!");
//         }
//     }
//     catch (const std::runtime_error &e)
//     {
//         std::cerr << e.what() << std::endl;
//     }

//     std::cout << "------------------------- CertificateAuthority -------------------------" << std::endl;
//     Client client_ca(grpc::CreateChannel(ca_server_ip + ":" + ca_server_port, grpc::InsecureChannelCredentials()));
//     try
//     {
//         std::string tee_id;
//         std::string certificate;
//         std::string public_key = "-----BEGIN PUBLIC KEY-----\n\
// MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAkJdBZm8ijjlXnCzWTI8m\n\
// j14+S8hqR5AFy4N9WY70rqY1Tr33PvfpQQ68S5U+8HwUKpeaMo8rlYTD4ULeNt4M\n\
// fqS1By11BmWVf94NHSzs86kqKfEtIM4ujF8hc7Asr0AYqUebUB1QccxDCxtN1csz\n\
// kgF5/p9T/ddRE6TfWryjfTESNe5qqS8b9shyND9SRNeRORlU9XthyoRpwjohqyD3\n\
// p7bSZ1t9h/kTRSaXpSd6JJrSyCK6qJS3knTsDqHvcjtaTkXMlekKcfzOmyj/GpMJ\n\
// Z6RbFFYFmji7knAP85zDnDwOXZ8hp0Y0xQ5FHScQy64N3Za0DYPTVWApYQgEP3L3\n\
// 0wIDAQAB\n\
// -----END PUBLIC KEY-----"; // 示例公钥
//         if (client_ca.InitializeTEE(public_key, tee_id, certificate))
//         {
//             std::cout << "[InitializeTEE] Initialized TEE with ID: " << tee_id << std::endl;
//             std::cout << "[InitializeTEE] Certificate: " << certificate << std::endl;
//         }
//         else
//         {
//             std::cerr << "[InitializeTEE] Failed to initialize TEE!" << std::endl;
//             throw std::runtime_error("[InitializeTEE] Failed to initialize TEE!");
//         }

//         // std::string user_id;
//         // bool success;
//         // std::string message;
//         // if (client_ca.RegisterUser("testuser", "password123", user_id, success, message))
//         // {
//         //     std::cout << "[RegisterUser] User ID: " << user_id << ", Success: "
//         //               << success << ", Message: " << message << std::endl;
//         // }
//         // else
//         // {
//         //     std::cerr << "[RegisterUser] Failed to register user!" << std::endl;
//         // }

//         // std::string test_dc_id;
//         // std::string encrypted_password;
//         // std::string encryption_key;
//         // if (client_ca.GetDataCapsuleInitialInformation("USER-1", test_dc_id, encrypted_password, encryption_key))
//         // {
//         //     std::cout << "[GetDataCapsuleInitialInformation] DCID: " << test_dc_id << ", Encrypted Password: "
//         //               << encrypted_password << ", Encryption Key: " << encryption_key << std::endl;
//         // }
//         // else
//         // {
//         //     std::cerr << "[GetDataCapsuleInitialInformation] Failed to get initial information!" << std::endl;
//         // }

//         std::string data_capsule_key;
//         if (client_ca.GetDataCapsuleKey(dc_id, data_capsule_key))
//         {
//             std::cout << "[GetDataCapsuleKey] DC Key: " << data_capsule_key << std::endl;
//         }
//         else
//         {
//             std::cerr << "[GetDataCapsuleKey] Failed to get DC key!" << std::endl;
//             throw std::runtime_error("[GetDataCapsuleKey] Failed to get DC key!");
//         }

//         std::string producer_signature_public_key;
//         if (client_ca.GetProducerSignaturePublicKey(tee_id, producer_signature_public_key))
//         {
//             std::cout << "[GetProducerSignaturePublicKey] Public Key: " << producer_signature_public_key << std::endl;
//         }
//         else
//         {
//             std::cerr << "[GetProducerSignaturePublicKey] Failed to get producer signature public key!" << std::endl;
//             throw std::runtime_error("[GetProducerSignaturePublicKey] Failed to get producer signature public key!");
//         }

//         // client_ca.TestTransfer("Hello, this is a test message.");
//     }
//     catch (const std::runtime_error &e)
//     {
//         std::cerr << e.what() << std::endl;
//     }
// }

// void testCertificateAuthorityInteraction(const std::string &ca_server_address, const std::string &dc_id)
// {
//     std::cout << "------------------------- testCertificateAuthorityInteraction -------------------------" << std::endl;
//     // 创建 gRPC 客户端，连接到 CertificateAuthority 服务
//     Client client(grpc::CreateChannel(ca_server_address, grpc::InsecureChannelCredentials()));

//     // 1. 测试 TEE 公钥提交与 TEEID 分配
//     std::string public_key =
//         "-----BEGIN PUBLIC KEY-----\n\
// MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAkJdBZm8ijjlXnCzWTI8m\n\
// j14+S8hqR5AFy4N9WY70rqY1Tr33PvfpQQ68S5U+8HwUKpeaMo8rlYTD4ULeNt4M\n\
// fqS1By11BmWVf94NHSzs86kqKfEtIM4ujF8hc7Asr0AYqUebUB1QccxDCxtN1csz\n\
// kgF5/p9T/ddRE6TfWryjfTESNe5qqS8b9shyND9SRNeRORlU9XthyoRpwjohqyD3\n\
// p7bSZ1t9h/kTRSaXpSd6JJrSyCK6qJS3knTsDqHvcjtaTkXMlekKcfzOmyj/GpMJ\n\
// Z6RbFFYFmji7knAP85zDnDwOXZ8hp0Y0xQ5FHScQy64N3Za0DYPTVWApYQgEP3L3\n\
// 0wIDAQAB\n\
// -----END PUBLIC KEY-----"; // 示例公钥
//     std::string tee_id;
//     std::string ceritificate;
//     client.InitializeTEE(public_key, tee_id, ceritificate);
//     std::cout << "收到分配的TEEID为:" << tee_id << std::endl;
//     std::cout << "收到证书为:" << ceritificate << std::endl;

//     // 2. 测试数据胶囊密钥请求与回复
//     std::string dc_key;
//     client.GetDataCapsuleKey(dc_id, dc_key);
//     std::cout << "收到数据胶囊密钥为: " << dc_key << std::endl;

//     // 3. 测试签名公钥请求与回复
//     std::string producer_public_key;
//     client.GetProducerSignaturePublicKey(tee_id, producer_public_key);
//     std::cout << "收到Producer公钥为: " << producer_public_key << std::endl;
// }

// // ====================================================================================================
