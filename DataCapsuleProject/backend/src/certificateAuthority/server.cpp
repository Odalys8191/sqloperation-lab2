#include "../../include/certificateAuthority/server.h"
#include "../../include/common/aes256.openssl.h"
#include "../../include/common/abe/CPABE.h"
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <iostream>
#include <fstream>

// 从公钥字符串创建EVP_PKEY
EVP_PKEY *createPublicKey(const std::string &pubKeyStr)
{
    BIO *bio = BIO_new_mem_buf(pubKeyStr.data(), pubKeyStr.size());
    EVP_PKEY *pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr); // 读取公钥
    BIO_free(bio);
    return pkey;
}

// 使用CA的私钥生成并签署证书
X509 *generateCertificate(EVP_PKEY *clientPublicKey)
{
    // 创建X509证书
    X509 *cert = X509_new();

    // 设置证书版本和序列号
    X509_set_version(cert, 2);                        // 版本号 (v3)
    ASN1_INTEGER_set(X509_get_serialNumber(cert), 1); // 序列号

    // 设置证书的有效期
    X509_gmtime_adj(X509_get_notBefore(cert), 0);        // 起始时间
    X509_gmtime_adj(X509_get_notAfter(cert), 31536000L); // 有效期1年

    // 设置证书的公钥
    X509_set_pubkey(cert, clientPublicKey);

    // 设置颁发者和主题名称
    X509_NAME *name = X509_get_subject_name(cert);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)"MyCA", -1, -1, 0);
    X509_set_issuer_name(cert, name);

    // 将 CA 私钥字符串定义为常量字符串
    const std::string caPrivateKeyStr = R"(-----BEGIN PRIVATE KEY-----
MIIEugIBADANBgkqhkiG9w0BAQEFAASCBKQwggSgAgEAAoIBAQC+6P5mjUhVV6j6
PTMTNVrUeWyXefxNTIRR5C9p/XOGFvgjAueSdaPDG4CidwyEmtBKvSP7GHCaAMsq
zy1sDh+gBCfx6juIMLd17iPr5hxX2YUhoJme6hcYXE6bhUBF2O6LgbvigCnE1ECA
rvTha00DUmn4T8hzq7kJU2t+1jjOVDIHhOaX+F8sZuph8xPe7jTPPlEZZPeJ1TLg
VaiYGQaLMMgT1s8Fi87VQRRjxK1GuQIfqA9xAwCL9aWL5SxybvrwAZiHyw24+4DQ
zmiYO2eKD4BiGXmhuXDFBunJFE4XK3eM4oqNoddt4eWBAscZZGzr0RHGLbewQUAf
EgZvoZh3AgMBAAECgf8wGYP/Uplm5y8r3T+BF3BLGEzD3Xg0AZNVDqjji3gP0Lm0
GIc/32PjRIqkZFpDDr5sFjt6iC3Wn1yQ7KIFF4ngRyvJR7q4FY4jqFxkkxWRDLz+
O/h4kYJAIQGtLWWEM6PmgklUR/Y+ITwfL/WC1gYRUDJi3bcjbaCxONNcoZyuXxjV
P6jF5I0g01sC7FbSeqj+2YbNnOaRmlxU2ZbfQmSM2LKCoEf+mHOmbXH8rBdj547W
r4T+I2mzp92qNYAi7Q2pXw8xoHISIdYFK0am9VPEYaQ0WhjBQ1PQVeqiUmMd4YE6
9w0L7BtV42G1bbEPgMpRcf2MA0HBWrgnrb3G4o0CgYEA3e2S2ZuADss/ELZ1KTEw
XdiuVakFrWSKUGY81u7GOpA4VOaQejEgrlbfWrjWmByMY+I3MtPlG5U5cK3dPf5E
W7u7EyXCle2Zc1f6SGwOhNQ743/cRhq/gZJqBAjWZCR6vUkVJU9jIV0Oo+AgM8PN
bwjgtShEmidB/7Vczq9GAnMCgYEA3DhTUluE6xY7woC2E2ERQjn/Br02ymq8R/JQ
swM57nkHV/Zo5Tgh6e/6Jyd8p8T7NhTIviNvoiAjIbv/VkLYgRbOkwQF7hSAMJaT
cj8XI03OBbiXSurCCq0BMUg53Ezpy68VtJOvOATwKW4KqTUQo08ms/czUg2EUEBw
LAVQXO0CgYBljUHma56UpnKMTkGcgNK2DuHvdrGPX3NfSPlMDOUOhVIRn5O7JmHj
BQOduvkPj/i5+Bj0HDw5e/ZV1mln5cMYCvaW02+QNbJDMp3np/KY25kwCCEevzyk
qP/0KqsYFg1IMnoHoaUl7jQ1zuhNZ6rIxVI182A4/f/YaAA438F0ewKBgEs6opwb
z4hPjBKhzZFJM5AgJ9KxiKoVVERvrsm4bG3ewnd2IddMGanE5PvDLX5SeE69QuZR
TK1htR6tDkj1hOh0VhU65puyPShcB/qbOthAmAS0Tlq1X5wuBZ5EQh/T1npSUWPi
n5TPewtsIBA1pH1laJ9J7yo0m+KeYLNpDU7xAoGAJdBB6e6Q0kl5Qyalr80EdmWZ
+inzX17BWRtj0uQLwG/fecmkBdh4Jyvd6lMBlQXxOIRTWhwzY0IpANEgomhmVaYG
+JCTjYBxhNzaMTMRLaKGkVRLP11xFSx6RoKP+3JmpyV/wrP6k8ns7oIvcjhwCKX4
9cF70kDQ5PRq7xl+GUY=
-----END PRIVATE KEY-----)";

    // 从字符串中加载 CA 私钥
    BIO *caKeyBio = BIO_new_mem_buf(caPrivateKeyStr.data(), caPrivateKeyStr.size());
    EVP_PKEY *caPrivateKey = PEM_read_bio_PrivateKey(caKeyBio, nullptr, nullptr, nullptr);
    BIO_free(caKeyBio);

    if (!caPrivateKey)
    {
        std::cerr << "Failed to load CA private key from string." << std::endl;
        X509_free(cert);
        return nullptr;
    }

    // 用 CA 私钥签署证书
    if (!X509_sign(cert, caPrivateKey, EVP_sha256()))
    {
        std::cerr << "Error signing certificate." << std::endl;
        EVP_PKEY_free(caPrivateKey);
        X509_free(cert);
        return nullptr;
    }

    EVP_PKEY_free(caPrivateKey);
    return cert;
}

CertificateAuthorityServiceImpl::CertificateAuthorityServiceImpl(const std::string &user_management_address)
{
    user_management_stub = serverInteraction::UserManagementService::NewStub(
        grpc::CreateChannel(user_management_address, grpc::InsecureChannelCredentials()));
    // 初始化一些测试数据
    std::string dc_key = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
                          0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};
    dc_map["00000001"] = dc_key;
    dc_map["00000002"] = dc_key;
    tee_map["00000001"] = "example_producer_public_key";
    tee_map["00000002"] = "example_producer_public_key";
}

std::string CertificateAuthorityServiceImpl::generateRandomKey()
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

grpc::Status CertificateAuthorityServiceImpl::InitializeTEE(grpc::ServerContext *context,
                                                            const serverInteraction::InitializeTEERequest *request,
                                                            serverInteraction::InitializeTEEResponse *response)
{
    std::lock_guard<std::mutex> lock(mutex_);
    boost::uuids::uuid uuid = uuid_gen();
    std::string tee_id = to_string(uuid);
    tee_map[tee_id] = request->public_key();
    std::cout << "[InitializeTEE] Initialized TEE ID: " << tee_id << ", Public Key: " << request->public_key() << std::endl;
    response->set_tee_id(tee_id);
    // 从客户端请求中获取公钥
    std::string pubKeyStr = request->public_key();
    EVP_PKEY *clientPublicKey = createPublicKey(pubKeyStr);

    if (!clientPublicKey)
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid public key format.");
    }

    // 根据公钥生成证书
    X509 *cert = generateCertificate(clientPublicKey);
    EVP_PKEY_free(clientPublicKey);

    if (!cert)
    {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to generate certificate.");
    }

    // 将证书转化为PEM格式字符串
    BIO *certBio = BIO_new(BIO_s_mem());
    PEM_write_bio_X509(certBio, cert);
    char *certData;
    long certLen = BIO_get_mem_data(certBio, &certData);
    std::string certStr(certData, certLen);

    // 返回证书给客户端
    response->set_sign_certificate(certStr);

    // 清理资源
    X509_free(cert);
    BIO_free(certBio);
    return grpc::Status::OK;
}

grpc::Status CertificateAuthorityServiceImpl::GetDataCapsuleInitialInformation(
    grpc::ServerContext *context,
    const serverInteraction::DataCapsuleInitialInformationRequest *request,
    serverInteraction::DataCapsuleInitialInformationResponse *response)
{
    // 生成 DCID
    boost::uuids::uuid uuid = uuid_gen();
    std::string dc_id = to_string(uuid);
    // 设置响应中的 DCID
    response->set_dc_id(dc_id);

    return grpc::Status::OK;
}

grpc::Status CertificateAuthorityServiceImpl::GetProducerSignaturePublicKey(grpc::ServerContext *context,
                                                                            const serverInteraction::GetProducerSignaturePublicKeyRequest *request,
                                                                            serverInteraction::GetProducerSignaturePublicKeyResponse *response)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::string tee_id = request->tee_id();

    if (tee_map.find(tee_id) == tee_map.end())
    {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "TEEID 未找到");
    }
    response->set_public_key(tee_map[tee_id]);
    return grpc::Status::OK;
}

// ------------------------------------- useless functions -------------------------------------

// grpc::Status CertificateAuthorityServiceImpl::GetDataCapsuleKey(grpc::ServerContext *context,
//                                                                 const serverInteraction::GetDataCapsuleKeyRequest *request,
//                                                                 serverInteraction::GetDataCapsuleKeyResponse *response)
// {
//     std::lock_guard<std::mutex> lock(mutex_);
//     std::string dc_id = request->dc_id();

//     auto it = dc_map.find(dc_id);
//     if (it == dc_map.end())
//     {
//         return grpc::Status(grpc::StatusCode::NOT_FOUND, "DCID 未找到");
//     }

//     response->set_dc_key(it->second);
// #ifdef DEBUG
//     std::cout << "[GetDataCapsuleKey] DCID: " << dc_id << ", DC Key: " << std::endl;
//     for (const auto &c : it->second)
//     {
//         std::cout << (int)c << " ";
//     }
//     std::cout << std::endl;
// #endif
//     return grpc::Status::OK;
// }

// grpc::Status CertificateAuthorityServiceImpl::GetABEPrivateKey(grpc::ServerContext *context,
//                                                                const serverInteraction::GetABEPrivateKeyRequest *request,
//                                                                serverInteraction::GetABEPrivateKeyResponse *response)
// {
//     std::string access_structure = request->access_structure();
//     int Access[MAX_ACCESS_SIZE];
//     parseAccessString(access_structure, Access);
//     int U = numsOfAttributes(Access); // 所有属性的个数，取决于access_structure
//     CP_ABE_PK pk;
//     CP_ABE_SK sk;
//     init_CP_ABE_SK(sk, U);
//     init_CP_ABE_PK(pk, U);
//     pairing_t p;
//     init_pairing(p, PUBLIC_PARAM_PATH);
//     CP_ABE_MSK msk;
//     deSerializeFromFile(msk, MASTER_KEY_PATH, p);
//     deSerializeFromFile(pk, PUBLIC_KEY_PATH, p);
//     int auth_size = request->auth_size();
//     int *auth = new int[auth_size];
//     for (int i = 0; i < auth_size; i++)
//     {
//         auth[i] = request->auth(i);
//     }
//     cpabe_Keygen(sk, auth, pk, msk, p);
//     json sk_json = cpabe_sk_to_json(sk);
//     std::string sk_str = sk_json.dump(4);
//     response->set_user_sk(sk_str);
//     delete[] auth;
//     return grpc::Status::OK;
// }

// grpc::Status CertificateAuthorityServiceImpl::GetDataCapsuleInitialInformation(
//     grpc::ServerContext *context,
//     const serverInteraction::DataCapsuleInitialInformationRequest *request,
//     serverInteraction::DataCapsuleInitialInformationResponse *response)
// {

//     // 生成 DCID 和 加密密钥
//     boost::uuids::uuid uuid = uuid_gen();
//     std::string dc_id = to_string(uuid);
//     std::string encryption_key = generateRandomKey();
//     dc_map[dc_id] = encryption_key; // 保存到数据胶囊的密钥映射

//     std::cout << "[GetDataCapsuleInitialInformation] DCID: " << dc_id << ", Encryption Key: " << encryption_key << std::endl;

//     // 设置响应中的 DCID 和加密密钥
//     response->set_dc_id(dc_id);

//     // response->set_encryption_key(encryption_key);

//     std::string user_id = request->user_id();

//     // 构造请求
//     serverInteraction::GetUserInfoRequest user_info_request;
//     user_info_request.set_user_id(user_id);

//     // 调用 UserManager 服务
//     serverInteraction::GetUserInfoResponse user_info_response;
//     grpc::ClientContext user_context;
//     grpc::Status status = user_management_stub->GetUserInfo(&user_context, user_info_request, &user_info_response);

//     // 检查服务调用是否成功以及用户信息是否存在
//     if (!status.ok() || !user_info_response.found())
//     {
//         return grpc::Status(grpc::StatusCode::NOT_FOUND, "用户信息未找到: " + user_id);
//     }

//     // 解密用户密码
//     std::string encrypted_password = user_info_response.encrypted_password();
//     std::string decrypted_password = decryptAES256(encrypted_password, aes_key);
//     std::string password_hash = user_info_response.password_hash();
//     // 使用数据胶囊的密钥加密用户密码
//     std::string Ek_password = encryptAES256(password_hash, encryption_key);

//     // 设置响应中的加密密码
//     response->set_encrypted_password(Ek_password);

//     return grpc::Status::OK;
// }
