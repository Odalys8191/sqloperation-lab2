#pragma once

#include <iostream>
#include <map>
#include <grpcpp/grpcpp.h>
#include "../../protos/serverInteraction.pb.h"
#include "../../protos/serverInteraction.grpc.pb.h"
#include "../../include/common/json.hpp"

using json = nlohmann::json;

class Client
{
public:
    Client(std::shared_ptr<grpc::Channel> channel)
        : cloud_stub_(serverInteraction::CloudStorageService::NewStub(channel)),
          ca_stub_(serverInteraction::CertificateAuthorityService::NewStub(channel)),
          etcd_stub_(serverInteraction::EtcdService::NewStub(channel)) {}

    // 调用 CloudStorageService 的方法
    bool GetDataCapsule(const std::string &dc_id, json &data_capsule);
    bool GetProgramPolicy(const std::string &dc_id, json &access_policy);
    // bool GetEncryptedDataCapsulePassword(const std::string &dc_id, const std::string &user_id, std::string &encrypted_dc_password);
    // bool GetEncryptedDataCapsulePassword(const std::string &dc_id, std::string &encrypted_dc_password);
    // bool GetDataCapsuleKeyAdditionalInformation(const std::string &dc_id, json &dc_key_additional_info);

    // 调用 EtcdService 的方法
    bool GetDataCapsuleState(const std::string &dc_id, json &state);
    bool UpdateDataCapsuleState(const std::string &dc_id, const json &state);

    // 调用 CertificateAuthorityService 的方法
    bool InitializeTEE(const std::string &public_key, std::string &tee_id, std::string &ceritificate);
    bool GetProducerSignaturePublicKey(const std::string &tee_id, std::string &producer_signature_public_key);

    // bool GetDataCapsuleKey(const std::string &dc_id, std::string &data_capsule_key);
    // void TestTransfer(const std::string &data);

    // user client中需要的方法，consumer不需要和user server交互
    // bool RegisterUser(const std::string &username, const std::string &password, std::string &user_id, bool &success, std::string &message);

    // producer client中打包数据胶囊时需要的方法
    // bool GetDataCapsuleInitialInformation(const std::string &user_id, std::string &dc_id, std::string &encrypted_password, std::string &encryption_key);

private:
    std::unique_ptr<serverInteraction::CloudStorageService::Stub> cloud_stub_;
    std::unique_ptr<serverInteraction::CertificateAuthorityService::Stub> ca_stub_;
    std::unique_ptr<serverInteraction::EtcdService::Stub> etcd_stub_;
};

// -------------------------------- useless functions --------------------------------

// void testServerInteraction(const std::string &ca_server_ip, const std::string &ca_server_port,
//                            const std::string &etcd_server_ip, const std::string &etcd_server_port,
//                            const std::string &cloud_server_ip, const std::string &cloud_server_port,
//                            const std::string &dc_id);

// void testCertificateAuthorityInteraction(const std::string &ca_server_address, const std::string &dc_id);