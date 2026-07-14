#pragma once

#include <iostream>
#include <unordered_map>
#include <grpcpp/grpcpp.h>
#include "../../protos/serverInteraction.pb.h"
#include "../../protos/serverInteraction.grpc.pb.h"
#include "../common/json.hpp"

using json = nlohmann::json;

class EtcdServiceImpl final : public serverInteraction::EtcdService::Service
{
private:
    // 模拟的元数据
    std::unordered_map<std::string, json> state_storage;

public:
    EtcdServiceImpl();
    grpc::Status GetDataCapsuleState(grpc::ServerContext *context, const serverInteraction::GetDataCapsuleStateRequest *request,
                                     serverInteraction::GetDataCapsuleStateResponse *response) override;
    grpc::Status UpdateDataCapsuleState(grpc::ServerContext *context, const serverInteraction::UpdateDataCapsuleStateRequest *request,
                                        serverInteraction::UpdateDataCapsuleStateResponse *response) override;
    grpc::Status SendDataCapsuleState(grpc::ServerContext *context, const serverInteraction::SendDataCapsuleStateRequest *request,
                                      serverInteraction::SendDataCapsuleStateResponse *response) override;
};

void runEtcdServer(const std::string &server_address);