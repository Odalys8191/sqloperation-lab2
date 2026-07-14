#pragma once

#include <iostream>
#include <grpcpp/grpcpp.h>
#include "../../protos/serverInteraction.grpc.pb.h"
#include "../../include/common/json.hpp"

#include "./serviceAddress.h"

using nlohmann::json;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
// Cloud
using serverInteraction::CloudStorageService;
using serverInteraction::GetExistingDataCapsuleInformationRequest;
using serverInteraction::GetExistingDataCapsuleInformationResponse;
using serverInteraction::GetExistingDataCapsuleNumberRequest;
using serverInteraction::GetExistingDataCapsuleNumberResponse;
// Producer
using serverInteraction::GenerateDataCapsuleRequest;
using serverInteraction::GenerateDataCapsuleResponse;
using serverInteraction::ProducerService;
// Consumer
using serverInteraction::ConsumeDataCapsuleRequest;
using serverInteraction::ConsumeDataCapsuleResponse;
using serverInteraction::ConsumerService;
// UserServer
using serverInteraction::UserLoginRequest;
using serverInteraction::UserLoginResponse;
using serverInteraction::UserManagementService;
using serverInteraction::UserRegistrationRequest;
using serverInteraction::UserRegistrationResponse;

class CloudClient
{
public:
    CloudClient(std::shared_ptr<Channel> channel)
        : stub_(CloudStorageService::NewStub(channel)) {}

    std::string getExistingDataCapsuleInformation(const std::string &message)
    {
        std::cout << "gRPC Client: getExistingDataCapsuleInformation(" << message << ")" << std::endl;
        // 构造 gRPC 请求
        GetExistingDataCapsuleInformationRequest request;
        request.set_message(message);

        // 构造 gRPC 响应
        GetExistingDataCapsuleInformationResponse response;
        ClientContext context;

        // 调用 gRPC 服务
        Status status = stub_->GetExistingDataCapsuleInformation(&context, request, &response);

        if (status.ok())
        {
            return response.data_capsules();
        }
        else
        {
            std::cerr << "gRPC Error: " << status.error_message() << std::endl;
            return "Error: Failed to call gRPC service.";
        }
    }

    int getExistingDataCapsuleNumber(const std::string &message)
    {
        std::cout << "gRPC Client: getExistingDataCapsuleNumber(" << message << ")" << std::endl;
        // 构造 gRPC 请求
        GetExistingDataCapsuleNumberRequest request;
        request.set_message(message);

        // 构造 gRPC 响应
        GetExistingDataCapsuleNumberResponse response;
        ClientContext context;

        // 调用 gRPC 服务
        Status status = stub_->GetExistingDataCapsuleNumber(&context, request, &response);

        if (status.ok())
        {
            return response.data_capsules_number();
            std::cout << "gRPC Client: getExistingDataCapsuleNumber(" << message << ") = " << response.data_capsules_number() << std::endl;
        }
        else
        {
            std::cerr << "gRPC Error: " << status.error_message() << std::endl;
            return -1; // 返回错误码
        }
        return 0; // 默认返回值
    }

private:
    std::unique_ptr<CloudStorageService::Stub> stub_;
};

class ProducerClient
{
public:
    ProducerClient(std::shared_ptr<Channel> channel)
        : stub_(ProducerService::NewStub(channel)) {}

    std::string generateDataCapsule(const std::string &ownerID, const json &policy, const std::string &abe_policy, int usableTimes,
                                    const std::string &data, const json &attributes)
    {
        GenerateDataCapsuleRequest dc_input;
        json state;
        state["UsableTimes"] = usableTimes;
        dc_input.set_state(state.dump());
        dc_input.set_owner_id(ownerID);
        dc_input.set_policy(abe_policy);
        dc_input.set_attributes(attributes.dump());
        dc_input.set_policy(policy.dump());
        dc_input.set_data(data);

        GenerateDataCapsuleResponse reply;
        grpc::ClientContext context;
        grpc::Status status = stub_->GenerateDataCapsule(&context, dc_input, &reply);

        if (status.ok())
        {
            return reply.dc_id();
        }
        else
        {
            std::cerr << "GenerateDataCapsule Request failed: " << status.error_message() << "\n";
            return "";
        }
    }

private:
    std::unique_ptr<ProducerService::Stub> stub_;
};

class ConsumerClient
{
public:
    ConsumerClient(std::shared_ptr<Channel> channel)
        : stub_(ConsumerService::NewStub(channel)) {}

    std::string consumeDataCapsule(const std::string &user_id,
                                   const std::string &dc_id,
                                   const std::string &user_request)
    {
        ConsumeDataCapsuleRequest request;
        request.set_user_id(user_id);
        request.set_dc_id(dc_id);
        request.set_user_request(user_request);

        ConsumeDataCapsuleResponse response;
        grpc::ClientContext context;

        grpc::Status status = stub_->ConsumeDataCapsule(&context, request, &response);

        if (status.ok())
        {
            return response.compute_result();
        }
        else
        {
            std::cerr << "Consume Data Capsule: " << dc_id << " request failed: " << status.error_message() << "\n";
            return "__ERROR__:" + status.error_message();
        }
    }

private:
    std::unique_ptr<ConsumerService::Stub> stub_;
};

class UserClient
{
public:
    UserClient(std::shared_ptr<Channel> channel)
        : stub_(UserManagementService::NewStub(channel)) {}

    // 用户注册方法
    std::pair<std::string, std::string> registerUser(const std::string &username, const std::string &password)
    {
        // 构造 gRPC 请求
        UserRegistrationRequest request;
        request.set_user_name(username);
        request.set_password(password);

        // 构造 gRPC 响应
        UserRegistrationResponse response;
        ClientContext context;

        // 调用 gRPC 服务
        Status status = stub_->RegisterUser(&context, request, &response);

        if (status.ok() && response.success())
        {
            return {response.user_name(), "Registration successful"}; // 返回 user_id 和消息
        }
        else
        {
            std::cerr << "gRPC Error: " << status.error_message() << std::endl;
            return {"", status.error_message()};
        }
    }

    // 用户登录方法
    std::string loginUser(const std::string &user_name, const std::string &password)
    {
        // 构造 gRPC 请求
        UserLoginRequest request;
        request.set_user_name(user_name);
        request.set_password(password);

        // 构造 gRPC 响应
        UserLoginResponse response;
        ClientContext context;

        // 调用 gRPC 服务
        Status status = stub_->LoginUser(&context, request, &response);

        if (status.ok() && response.success())
        {
            return "success";
        }
        else
        {
            std::cerr << "gRPC Error: " << status.error_message() << std::endl;
            return "Error: Failed to login user.";
        }
    }

private:
    std::unique_ptr<UserManagementService::Stub> stub_;
};