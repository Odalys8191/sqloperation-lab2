#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "../../protos/serverInteraction.pb.h"
#include "../../protos/serverInteraction.grpc.pb.h"
#include "api.h"
#include "../../include/common/json.hpp"
#include "../../include/common/serviceAddress.h"

using json = nlohmann::json;

// 定义服务处理类
class ConsumerTEEServiceImpl final : public serverInteraction::ConsumerService::Service
{
public:
    grpc::Status ConsumeDataCapsule(grpc::ServerContext *context,
                                    const serverInteraction::ConsumeDataCapsuleRequest *request,
                                    serverInteraction::ConsumeDataCapsuleResponse *response) override;

private:
    std::string user_id_;
    std::string dc_id_;
    std::string password_;
    json access_policy_;
};

void runConsumerTEEServer(std::string server_address);