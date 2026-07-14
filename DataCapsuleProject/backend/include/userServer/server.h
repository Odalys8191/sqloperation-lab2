#pragma once

#include "../../protos/serverInteraction.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <unordered_map>
#include <mutex>
#include <tuple>
#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "userdb.h"
#include "password_utils.h"

class UserManagementServiceImpl final : public serverInteraction::UserManagementService::Service {
public:
    UserManagementServiceImpl(); // 构造函数

    // 注册用户
    grpc::Status RegisterUser(grpc::ServerContext* context,
                              const serverInteraction::UserRegistrationRequest* request,
                              serverInteraction::UserRegistrationResponse* response) override;

    // 用户登录
    grpc::Status LoginUser(grpc::ServerContext* context,
                           const serverInteraction::UserLoginRequest* request,
                           serverInteraction::UserLoginResponse* response) override;

    // 获取用户信息
    grpc::Status GetUserInfo(grpc::ServerContext* context,
                             const serverInteraction::GetUserInfoRequest* request,
                             serverInteraction::GetUserInfoResponse* response) override;

private:
    // 存储用户信息：<UserID, (Username, UserID, EncryptedPassword)>
    std::unordered_map<std::string, std::tuple<std::string, std::string, std::string>> user_data;
    std::mutex mutex_;                      // 线程安全锁
    boost::uuids::random_generator uuid_gen; // UUID生成器
    std::string aes_key = "20021212YukiSiHwonnatseesoyoanon"; // AES密钥
    
    Database& db;
};
