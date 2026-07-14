#include "../../include/userServer/server.h"
#include "../../include/common/aes256.openssl.h"
// 构造函数：初始化测试数据
UserManagementServiceImpl::UserManagementServiceImpl():db(Database::getInstance()) {

    if (!db.connect("../../include/userServer/userdb.conf")) {
        throw std::runtime_error("连接数据库失败");
    }
    // 添加测试用户
    std::string test_username = "testuser1";
    std::string test_password = "password1";
    std::string test_user_id = "user1";

    std::string encrypted_password = encryptAES256(test_password, aes_key);
    user_data[test_user_id] = std::make_tuple(test_username, test_user_id, encrypted_password);

    std::string test_username2 = "testuser2";
    std::string test_password2 = "password2";
    std::string test_user_id2 = "user2";

    std::string encrypted_password2 = encryptAES256(test_password2, aes_key);
    user_data[test_user_id2] = std::make_tuple(test_username2, test_user_id2, encrypted_password2);
    
    std::string test_username3 = "ministry of health";
    std::string test_password3 = "password123";
    std::string test_user_id3 = "00000001";
    user_data[test_user_id3] = std::make_tuple(test_username3, test_user_id3, encryptAES256(test_password3, aes_key));

    std::string test_username4 = "insurance company";
    std::string test_password4 = "password456";
    std::string test_user_id4 = "00000002";
    user_data[test_user_id4] = std::make_tuple(test_username4, test_user_id4, encryptAES256(test_password4, aes_key));

    std::string test_username5 = "hospital";
    std::string test_password5 = "password789";
    std::string test_user_id5 = "00000003";
    user_data[test_user_id5] = std::make_tuple(test_username5, test_user_id5, encryptAES256(test_password5, aes_key));
}

// 注册用户
grpc::Status UserManagementServiceImpl::RegisterUser(grpc::ServerContext* context,
                                                     const serverInteraction::UserRegistrationRequest* request,
                                                     serverInteraction::UserRegistrationResponse* response) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 生成唯一的 UserID
    boost::uuids::uuid uuid = uuid_gen();
    std::string user_id = to_string(uuid);

    //数据库相关
    std::string user_name = request->user_name();
    std::string password_hash = hash_password_argon2id(request->password());
    if (!db.insertUser(user_id, user_name, password_hash)) {
        response->set_success(false);
        response->set_message("可能是用户名已存在");
        return grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "可能是用户名已存在");
    }

    // 加密密码
    //std::string encrypted_password = encryptAES256(request->password(), aes_key);

    // 创建三元组 (username, user_id, encrypted_password)
    /*std::tuple<std::string, std::string, std::string> user_entry =
        std::make_tuple(request->username(), user_id, encrypted_password);

    // 存储到 user_data
    user_data[user_id] = user_entry;*/
    

    // 返回注册结果
    //response->set_user_id(user_id);
    response->set_user_name(user_name);
    response->set_success(true);
    response->set_message("注册成功");
    std::cout<<"用户"<<user_id<<"已注册"<<std::endl;
    return grpc::Status::OK;
}

// 用户登录
grpc::Status UserManagementServiceImpl::LoginUser(grpc::ServerContext* context,
                                                  const serverInteraction::UserLoginRequest* request,
                                                  serverInteraction::UserLoginResponse* response) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 仅支持通过 UserID 登录
    /*auto it = user_data.find(request->user_id());
    if (it == user_data.end()) {
        response->set_success(false);
        response->set_message("UserID 不存在");
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "UserID 不存在");
    }

    // 验证密码
    const auto& [username, user_id, encrypted_password] = it->second;
    std::string decrypted_password = decryptAES256(encrypted_password, aes_key);
    if (decrypted_password != request->password()) {
        response->set_success(false);
        response->set_message("密码错误");
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "密码错误");
    }*/

    // 数据库相关
    std::string user_name = request->user_name();
    if (user_name.empty() || request->password().empty()) {
        response->set_success(false);
        response->set_message("UserName 或密码不能为空");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "UserName 或密码不能为空");
    }
    std::string out_hash;
    if (!db.getPasswordHash(user_name, out_hash)) {
        response->set_success(false);
        response->set_message("UserName 不存在");
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "该用户不存在");
    }
    if (!verify_password_argon2id(out_hash, request->password())) {
        response->set_success(false);
        response->set_message("密码错误");
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "密码错误");
    }

    // 登录成功
    response->set_success(true);
    response->set_message("登录成功");
    response->set_user_name(user_name);
    std::cout<<"用户"<<user_name<<"已登录"<<std::endl;
    return grpc::Status::OK;
}

// 获取用户信息
grpc::Status UserManagementServiceImpl::GetUserInfo(grpc::ServerContext* context,
                                                    const serverInteraction::GetUserInfoRequest* request,
                                                    serverInteraction::GetUserInfoResponse* response) {
    std::lock_guard<std::mutex> lock(mutex_);

    //数据库相关
    std::string stored_hash;
    if (db.getPasswordHash(request->user_id(), stored_hash)) {
        response->set_password_hash(stored_hash);
        response->set_found(true);
        return grpc::Status::OK;
    }
    else {
        response->set_found(false);
        response->set_message("UserID 未找到");
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "UserID 未找到");
    }

    /*auto it = user_data.find(request->user_id());
    if (it == user_data.end()) {
        response->set_found(false);
        response->set_message("UserID 未找到");
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "UserID 未找到");
    }

    // 仅返回加密后的密码
    const auto& [username, user_id, encrypted_password] = it->second;
    response->set_encrypted_password(encrypted_password);
    response->set_found(true);
    return grpc::Status::OK;*/
}
