#include "../../include/producer/producer.h"
#include <random>

static std::string generateRandomKey()
{
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const int length = 32;
    std::string key;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    for (int i = 0; i < length; ++i)
    {
        key += chars[dis(gen)];
    }
    return key;
}

grpc::Status ProducerServiceImpl::GenerateDataCapsule(::grpc::ServerContext *context, const serverInteraction::GenerateDataCapsuleRequest *dc_input,
                                                      serverInteraction::GenerateDataCapsuleResponse *reply)
{
    // policy
    std::string policy_str = dc_input->policy();
    json policy = stringToJson(policy_str);
    // attributes
    json attributes = stringToJson(dc_input->attributes());
    // owner_id
    std::string owner_id = dc_input->owner_id();
    // // user ID
    // std::string user_id = dc_input->user_id();
    std::string abe_policy = dc_input->abe_policy();
    // TODO: 在policy中读取加密算法
    EncryptAlgType cryptoalg = EncryptAlgType::Encrypt_AES256_CBC;
    // data
    std::string data = dc_input->data();
    int data_len = data.size();

    // ----------------------------------------------------------------------------------------
    // ------------------- 发送userID 给CertificateAuthority ,获得dc_id,k,E_k(pwd_userID) -------------------
    // ----------------------------------------------------------------------------------------
    DataCapsuleInitialInformationRequest send_user_id_request;
    send_user_id_request.set_user_id(owner_id); // TODO

    DataCapsuleInitialInformationResponse send_user_id_response;
    std::shared_ptr<Channel> channel = grpc::CreateChannel(CERTIFICATE_AURTHORITY_ADDRESS, grpc::InsecureChannelCredentials());
    std::unique_ptr<CertificateAuthorityService::Stub> ca_stub_ = CertificateAuthorityService::NewStub(channel);
    ClientContext ca_context;
    Status datacapsule_initial_status = ca_stub_->GetDataCapsuleInitialInformation(&ca_context, send_user_id_request, &send_user_id_response);
    if (!datacapsule_initial_status.ok())
    {
        std::cout << datacapsule_initial_status.error_message() << std::endl;
        return grpc::Status(grpc::NOT_FOUND, "[Error] Get Data Capsule InitialInformation Failed!");
    }

    // std::string encrypted_password = send_user_id_response.encrypted_password();

    // encrypted_password = base64Encode(reinterpret_cast<const unsigned char *>(encrypted_password.data()), encrypted_password.size());
    std::string dc_id = send_user_id_response.dc_id();
    // std::string key = send_user_id_response.encryption_key();

    std::cout << "接收到CA发来的数据胶囊ID为 " << dc_id << std::endl;
    // std::cout << "接收到CA发来的数据胶囊密钥为 " << key << std::endl;

    // 生成32位随机密钥
    std::string key = generateRandomKey();

    // ----------------------------------------------------------------------------------------
    // ------------------------------- 发送 dc_id,state 给 Etcd --------------------------------
    // ----------------------------------------------------------------------------------------
    json dc_state = json::parse(dc_input->state());

    json etcd_state;
    etcd_state["OwnerID"] = dc_input->owner_id();
    etcd_state["UsableTimes"] = dc_state["UsableTimes"];

    SendDataCapsuleStateRequest send_datacapsuel_state_request;
    send_datacapsuel_state_request.set_dc_id(dc_id);
    send_datacapsuel_state_request.set_state(etcd_state.dump());

    std::shared_ptr<Channel> etcd_channel = grpc::CreateChannel(ETCD_SERVICE_ADDRESS, grpc::InsecureChannelCredentials());
    std::unique_ptr<EtcdService::Stub> etcd_stub_ = EtcdService::NewStub(etcd_channel);
    ClientContext etcd_context;
    SendDataCapsuleStateResponse send_datacapsuel_state_response;
    Status send_states_status = etcd_stub_->SendDataCapsuleState(&etcd_context, send_datacapsuel_state_request, &send_datacapsuel_state_response);
    if (!send_states_status.ok())
    {
        std::cout << send_states_status.error_message() << std::endl;
        return grpc::Status(grpc::NOT_FOUND, "[Error] Send Data Capsule State Failed!");
    }
#ifdef DEBUG
    std::cout << send_datacapsuel_state_response.result() << std::endl;
#endif
    // ----------------------------------------------------------------------------------------
    // ------------------------------- 策略转换 ------------------------------------------
    // 策略转换
    auto aa_server_channel = grpc::CreateChannel(AA_SERVICE_ADDRESS, grpc::InsecureChannelCredentials());
    auto aa_server_stub = serverInteraction::AttributeAuthorityService::NewStub(aa_server_channel);
    std::string accessStructure1 = attributes["AccessStructure"]["1"];
    serverInteraction::ConvertPolicyRequest policy_req1;
    policy_req1.set_policy(accessStructure1);
    policy_req1.set_layer(1); // 设置层级为1
    // 发送请求到AA Server
    serverInteraction::ConvertPolicyResponse policy_resp1;
    grpc::ClientContext ctx1;
    auto status1 = aa_server_stub->ConvertPolicy(&ctx1, policy_req1, &policy_resp1);
    std::string converted_policy1 = policy_resp1.converted_policy();
    if (status1.ok())
    {
        std::cout << "[TEST] ConvertedPolicy1: " << converted_policy1 << std::endl;
    }
    else
    {
        std::cout << "[TEST] ConvertPolicy1 failed: " << status1.error_message() << std::endl;
    }
    if (!status1.ok() || converted_policy1.empty() || converted_policy1 == "0")
    {
        return grpc::Status(grpc::INVALID_ARGUMENT, "[Error] ConvertPolicy1 failed or invalid access structure");
    }

    std::string accessStructure2 = attributes["AccessStructure"]["2"];
    serverInteraction::ConvertPolicyRequest policy_req2;
    policy_req2.set_policy(accessStructure2);
    policy_req2.set_layer(2); // 设置层级为2
    // 发送请求到AA Server
    serverInteraction::ConvertPolicyResponse policy_resp2;
    grpc::ClientContext ctx2;
    auto status2 = aa_server_stub->ConvertPolicy(&ctx2, policy_req2, &policy_resp2);
    std::string converted_policy2 = policy_resp2.converted_policy();
    if (status2.ok())
    {
        std::cout << "[TEST] ConvertedPolicy2: " << converted_policy2 << std::endl;
    }
    else
    {
        std::cout << "[TEST] ConvertPolicy2 failed: " << status2.error_message() << std::endl;
    }
    if (!status2.ok() || converted_policy2.empty() || converted_policy2 == "0")
    {
        return grpc::Status(grpc::INVALID_ARGUMENT, "[Error] ConvertPolicy2 failed or invalid access structure");
    }
    attributes["AccessStructure"]["1_convert"] = converted_policy1;
    attributes["AccessStructure"]["2_convert"] = converted_policy2;
    // ----------------------------------------------------------------------------------------
    // ------------------------------- gendatacapsule -----------------------------------------
    // ----------------------------------------------------------------------------------------
    json data_capsule = genDataCapsule(dc_id, pro_tee_id, policy, cryptoalg, data, data_len, key, attributes, converted_policy1, converted_policy2);

    // ----------------------------------------------------------------------------------------
    // ------------------------------- 发送 dc 给 Cloud Server --------------------------------
    // ----------------------------------------------------------------------------------------
    StoreDataCapsuleRequest store_datacapsuel_request;
    StoreDataCapsuleResponse store_datacapsuel_response;
    store_datacapsuel_request.set_dc_id(dc_id);
    store_datacapsuel_request.set_data_capsule(data_capsule.dump());
    std::shared_ptr<Channel> cloude_channel = grpc::CreateChannel(CLOUD_SERVICE_ADDRESS, grpc::InsecureChannelCredentials());
    std::unique_ptr<CloudStorageService::Stub> cloud_stub_ = CloudStorageService::NewStub(cloude_channel);
    ClientContext cloudstorage_context;
    Status store_datacapsule_status = cloud_stub_->StoreDataCapsule(&cloudstorage_context, store_datacapsuel_request, &store_datacapsuel_response);
    std::string res = "";
    if (store_datacapsule_status.ok())
    {
        res = "[Success] GenerateDataCapsule success! DCID: " + dc_id;
        reply->set_res(res);
        reply->set_dc_id(dc_id);
        return Status::OK;
    }
    else
    {
        res = "[Error] GenerateDataCapsule Failed!";
        reply->set_res(res);
        std::cout << store_datacapsule_status.error_message() << std::endl;
        return grpc::Status(grpc::NOT_FOUND, "[Error] GenerateDataCapsule Failed!"); // 改
    }
}

std::string initialTEE()
{
    std::shared_ptr<Channel> channel = grpc::CreateChannel(CERTIFICATE_AURTHORITY_ADDRESS, grpc::InsecureChannelCredentials());
    std::unique_ptr<CertificateAuthorityService::Stub> ca_stub_ = CertificateAuthorityService::NewStub(channel);

    //     std::string pubKeyStr =
    //         "-----BEGIN PUBLIC KEY-----\n\
// MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAkJdBZm8ijjlXnCzWTI8m\n\
// j14+S8hqR5AFy4N9WY70rqY1Tr33PvfpQQ68S5U+8HwUKpeaMo8rlYTD4ULeNt4M\n\
// fqS1By11BmWVf94NHSzs86kqKfEtIM4ujF8hc7Asr0AYqUebUB1QccxDCxtN1csz\n\
// kgF5/p9T/ddRE6TfWryjfTESNe5qqS8b9shyND9SRNeRORlU9XthyoRpwjohqyD3\n\
// p7bSZ1t9h/kTRSaXpSd6JJrSyCK6qJS3knTsDqHvcjtaTkXMlekKcfzOmyj/GpMJ\n\
// Z6RbFFYFmji7knAP85zDnDwOXZ8hp0Y0xQ5FHScQy64N3Za0DYPTVWApYQgEP3L3\n\
// 0wIDAQAB\n\
// -----END PUBLIC KEY-----";
    std::string pubKeyStr = readFromFile("../../keys/sign.key");

    InitializeTEERequest request;
    request.set_public_key(pubKeyStr);
    InitializeTEEResponse response;
    grpc::ClientContext context;
    grpc::Status status = ca_stub_->InitializeTEE(&context, request, &response);
    if (status.ok())
    {
        std::cout << "收到分配到TEE ID为: " << response.tee_id() << std::endl;
        std::cout << "收到数字证书: " << response.sign_certificate() << std::endl;
        return response.tee_id();
    }
    else
    {
        std::cerr << "InitializeTEE RPC failed: " << status.error_message() << std::endl;
        return "";
    }
}

void runProducerServer()
{
    std::string server_address(PRODUCER_SERVICE_ADDRESS);
    std::string tee_id = initialTEE();
    ProducerServiceImpl service(tee_id);
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Producer Server listening on " << server_address << std::endl;
    server->Wait();
}

json genDataCapsule(
    const std::string &dc_id,           // 胶囊ID
    const std::string &producer_tee_id, // Producer TEE ID
    const json &policy,                 // 数据胶囊的访问策略
    EncryptAlgType crypto_alg,          // 明文加密算法
    const std::string &plain_data,      // 胶囊明文
    unsigned long long data_len,        // 没用到
    const std::string &encrypt_key,     // 加密胶囊明文的对称密钥（也是加密password的密钥）
    const json &attributes,
    const std::string &converted_policy1, // 第一层ABE加密的访问策略
    const std::string &converted_policy2  // 第二层ABE加密的访问策略
)
{
    // 加密胶囊明文
    std::string ciphertext_base64_data;
    std::string iv_base64_data;
    switch (crypto_alg)
    {
    case EncryptAlgType::Encrypt_AES256_CBC:
    {
        unsigned char *iv = (unsigned char *)malloc(16);
        RAND_bytes(iv, 16);
        std::string iv_str = std::string(reinterpret_cast<char *>(iv), 16);
        std::string ciphertext = encryptAES256(plain_data, encrypt_key, iv_str);

        iv_base64_data = base64Encode(iv, 16);
        ciphertext_base64_data = base64Encode(reinterpret_cast<unsigned char *>(ciphertext.data()), ciphertext.size());
        std::cout << "[TEST] 明文长度: " << plain_data.length() << std::endl;
        std::cout << "[TEST] 对称密钥encrypt_key: " << encrypt_key << std::endl;
        std::cout << "[TEST] AES256_CBC 加密明文完成，IV(base64): " << iv_base64_data << std::endl;
        std::cout << "[TEST] AES256_CBC 密文(base64)前32字节: " << ciphertext_base64_data.substr(0, 32) << "..." << std::endl;
        break;
    }
    default:
        break;
    }

    // 第一层ABE加密 encrypt_key
    pairing_t pairing;
    init_pairing(pairing, PUBLIC_PARAM_PATH);
    std::cout << "[TEST] pairing 初始化完成。" << std::endl;
    int Access1[MAX_ACCESS_SIZE];
    parseAccessString(converted_policy1, Access1);
    std::cout << "[TEST] Access1: ";
    for (int i = 0; i < MAX_ACCESS_SIZE && Access1[i] != 0; i++)
    {
        std::cout << Access1[i] << " ";
    }
    CT ct1;
    CP_ABE_PK pk1;
    int U1 = 10;
    init_CP_ABE_PK(pk1, U1);
    deSerializeFromFile(pk1, PUBLIC_KEY_1_PATH, pairing);
    std::cout << "[TEST] pk1 反序列化完成。" << std::endl;
    cpabe_Encrypt(Access1, reinterpret_cast<const unsigned char *>(encrypt_key.c_str()), ct1, pk1, pairing);

    std::cout << "[TEST] 第一层ABE加密完成，encrypt_key长度: " << encrypt_key.size() << std::endl;

    // 用TEE密钥加密ct1
    unsigned char tee_key[32] = "1234567890abcdef1234567890abcde";
    int out_ct1_len = 0;
    unsigned char out_ct1_buf[1024 * 1024];
    out_ct1_len = cpabe_SerializeCT(ct1, out_ct1_buf);
    std::string ct1_str = std::string(reinterpret_cast<char *>(out_ct1_buf), out_ct1_len);
    std::string iv_base64_ct1;
    std::string ciphertext_base64_ct1;
    {
        unsigned char *iv2 = (unsigned char *)malloc(16);
        RAND_bytes(iv2, 16);
        std::string iv_str2 = std::string(reinterpret_cast<char *>(iv2), 16);
        std::string ciphertext = encryptAES256(ct1_str, std::string(reinterpret_cast<char *>(tee_key), 32), iv_str2);

        iv_base64_ct1 = base64Encode(iv2, 16);
        ciphertext_base64_ct1 = base64Encode(reinterpret_cast<unsigned char *>(ciphertext.data()), ciphertext.size());

        std::cout << "[TEST] TEE密钥加密ct1完成，IV(base64): " << iv_base64_ct1 << std::endl;
        std::cout << "[TEST] ct1密文(base64)前32字节: " << ciphertext_base64_ct1.substr(0, 32) << "..." << std::endl;
    }

    // 第二层ABE加密TEE密钥
    int Access2[MAX_ACCESS_SIZE];
    parseAccessString(converted_policy2, Access2);
    CT ct2;
    CP_ABE_PK pk2;
    int U2 = 10; // 假设U2为10
    init_CP_ABE_PK(pk2, U2);
    deSerializeFromFile(pk2, PUBLIC_KEY_2_PATH, pairing);
    cpabe_Encrypt(Access2, tee_key, ct2, pk2, pairing);

    std::cout << "[TEST] 第二层ABE加密TEE密钥完成。" << std::endl;

    json ct_json = ct_to_json(ct2);

    // 生成数据胶囊
    json data_capsule;
    data_capsule["MetaData"]["DCID"] = dc_id;
    data_capsule["MetaData"]["PTEEID"] = producer_tee_id;
    data_capsule["MetaData"]["Policy"] = policy;
    data_capsule["MetaData"]["ABEEncryption"] = ct_json;
    data_capsule["MetaData"]["ABEEncryption"]["AccessStructure"] = attributes["AccessStructure"];
    data_capsule["MetaData"]["DataIntro"]["DataDescription"] = attributes["DataDescription"];
    data_capsule["MetaData"]["DataIntro"]["Columns"] = attributes["Columns"];
    data_capsule["MetaData"]["DataIntro"]["DataLen"] = plain_data.length();
    data_capsule["MetaData"]["SymmetricEncryption"]["CryptoAlg"] = crypto_alg;
    data_capsule["MetaData"]["SymmetricEncryption"]["AESIV_ct1"] = iv_base64_ct1;
    data_capsule["MetaData"]["SymmetricEncryption"]["AESIV_data"] = iv_base64_data;
    data_capsule["Data"] = ciphertext_base64_data;
    data_capsule["Ciphertext_ct1"] = ciphertext_base64_ct1;

    std::cout << "[TEST] 胶囊生成完成，DCID: " << dc_id << std::endl;
    std::cout << "[TEST] 胶囊JSON预览: " << data_capsule.dump(2).substr(0, 512) << "..." << std::endl;

    // 签名等后续流程...
    std::string message = data_capsule["MetaData"].dump() + data_capsule["Data"].dump();
    std::string public_key;
    std::string signature = createRSASignature(message, public_key);
    std::string encoded_signature = base64Encode(reinterpret_cast<const unsigned char *>(signature.data()), signature.size());
    data_capsule["Signature"] = encoded_signature;

    std::cout << "[TEST] 胶囊签名完成，签名(base64)前32字节: " << encoded_signature.substr(0, 32) << "..." << std::endl;

    return data_capsule;
}