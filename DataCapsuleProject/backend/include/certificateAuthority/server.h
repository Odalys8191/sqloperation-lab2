#pragma once

#include "../../protos/serverInteraction.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <unordered_map>
#include <string>
#include <mutex>
#include <random>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

class CertificateAuthorityServiceImpl final : public serverInteraction::CertificateAuthorityService::Service
{
private:
    std::unordered_map<std::string, std::string> tee_map; // <TEEID, public_key>
    std::unordered_map<std::string, std::string> dc_map;  // <DCID, DCK>
    std::mutex mutex_;
    boost::uuids::random_generator uuid_gen; // UUID生成器
    std::string aes_key = "20021212YukiSiHwonnatseesoyoanon";
    std::unique_ptr<serverInteraction::UserManagementService::Stub> user_management_stub; // 用户管理服务的客户端

    // 生成随机密钥函数
    std::string generateRandomKey();

public:
    // 构造函数，初始化与用户管理服务的连接
    CertificateAuthorityServiceImpl(const std::string &user_management_address);

    // TEE公钥提交和TEEID分配
    grpc::Status InitializeTEE(grpc::ServerContext *context,
                               const serverInteraction::InitializeTEERequest *request,
                               serverInteraction::InitializeTEEResponse *response) override;

    // 胶囊初始信息请求与回复
    grpc::Status GetDataCapsuleInitialInformation(grpc::ServerContext *context,
                                                  const serverInteraction::DataCapsuleInitialInformationRequest *request,
                                                  serverInteraction::DataCapsuleInitialInformationResponse *response) override;

    // 签名公钥请求与回复
    grpc::Status GetProducerSignaturePublicKey(grpc::ServerContext *context,
                                               const serverInteraction::GetProducerSignaturePublicKeyRequest *request,
                                               serverInteraction::GetProducerSignaturePublicKeyResponse *response) override;

    // // 数据胶囊密钥请求与回复
    // grpc::Status GetDataCapsuleKey(grpc::ServerContext *context,
    //                                const serverInteraction::GetDataCapsuleKeyRequest *request,
    //                                serverInteraction::GetDataCapsuleKeyResponse *response) override;

    // // 用户Cpabe私钥请求与回复
    // grpc::Status GetABEPrivateKey(grpc::ServerContext* context,
    //                                            const serverInteraction::GetABEPrivateKeyRequest* request,
    //                                            serverInteraction::GetABEPrivateKeyResponse* response) override;
};
