#pragma once

#include <grpcpp/grpcpp.h>
#include "../../protos/serverInteraction.pb.h"
#include "../../protos/serverInteraction.grpc.pb.h"
#include <unordered_map>
#include <iostream>
#include <map>

#include "../common/json.hpp"

using json = nlohmann::json;

class CloudStorageServiceImpl final : public serverInteraction::CloudStorageService::Service
{
private:
    // 模拟的数据胶囊
    std::unordered_map<std::string, json> data_capsules;
    std::unordered_map<std::string, std::string> encrypted_passwords;

public:
    CloudStorageServiceImpl() {};
    grpc::Status GetDataCapsule(grpc::ServerContext *context, const serverInteraction::GetDataCapsuleRequest *request,
                                serverInteraction::GetDataCapsuleResponse *response) override;

    grpc::Status GetProgramPolicy(grpc::ServerContext *context,
                                  const serverInteraction::GetProgramPolicyRequest *request,
                                  serverInteraction::GetProgramPolicyResponse *response) override;
    grpc::Status StoreDataCapsule(grpc::ServerContext *context,
                                  const serverInteraction::StoreDataCapsuleRequest *request,
                                  serverInteraction::StoreDataCapsuleResponse *response) override;
    grpc::Status GetExistingDataCapsuleInformation(grpc::ServerContext *context,
                                                   const serverInteraction::GetExistingDataCapsuleInformationRequest *request,
                                                   serverInteraction::GetExistingDataCapsuleInformationResponse *response) override;
    grpc::Status GetExistingDataCapsuleNumber(grpc::ServerContext *context,
                                              const serverInteraction::GetExistingDataCapsuleNumberRequest *request,
                                              serverInteraction::GetExistingDataCapsuleNumberResponse *response) override;
    // grpc::Status GetDataCapsuleKeyAdditionalInformation(grpc::ServerContext *context,
    //                                                     const serverInteraction::GetDataCapsuleKeyAdditionalInformationRequest *request,
    //                                                     serverInteraction::GetDataCapsuleKeyAdditionalInformationResponse *response) override;
    // grpc::Status GetEncryptedDataCapsulePassword(grpc::ServerContext *context,
    //                                              const serverInteraction::GetEncryptedDataCapsulePasswordRequest *request,
    //                                              serverInteraction::GetEncryptedDataCapsulePasswordResponse *response) override;
};

void runCloudServer(const std::string &server_address);