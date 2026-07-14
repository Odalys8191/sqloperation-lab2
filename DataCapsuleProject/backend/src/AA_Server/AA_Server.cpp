#include "../../include/AA_Server/AAServiceImpl.h"
#include <iostream>

AAServiceImpl::AAServiceImpl()
    : converter1({
        {"role:admin", 1},
        {"clearance:high", 2},
        {"dept:finance", 3},
        {"role:user", 4},
        {"clearance:low", 5},
        {"dept:hr", 6},
        {"team:alpha", 7},
        {"team:beta", 8},
        {"project:X", 9},
        {"project:Y", 10}
      }),
      converter2({
        {"role:admin", 1},
        {"clearance:high", 2},
        {"dept:finance", 3},
        {"role:user", 4},
        {"clearance:low", 5},
        {"dept:hr", 6},
        {"team:alpha", 7},
        {"team:beta", 8},
        {"project:X", 9},
        {"project:Y", 10}
      })
{
    attr_map1 = {
        {"role:admin", 1},
        {"clearance:high", 2},
        {"dept:finance", 3},
        {"role:user", 4},
        {"clearance:low", 5},
        {"dept:hr", 6},
        {"team:alpha", 7},
        {"team:beta", 8},
        {"project:X", 9},
        {"project:Y", 10}
    };
    attr_map2 = {
        {"role:admin", 1},
        {"clearance:high", 2},
        {"dept:finance", 3},
        {"role:user", 4},
        {"clearance:low", 5},
        {"dept:hr", 6},
        {"team:alpha", 7},
        {"team:beta", 8},
        {"project:X", 9},
        {"project:Y", 10}
    };
    init_CP_ABE_PK(mpk1, 10);
    init_CP_ABE_PK(mpk2, 10);
    std::cout << "[AA] 初始化 pairing..." << std::endl;
    init_pairing(pairing, PUBLIC_PARAM_PATH);
    std::cout << "[AA] 生成第一层公私钥..." << std::endl;
    cpabe_Setup(mpk1, msk1, pairing);
    std::cout << "[AA] 生成第二层公私钥..." << std::endl;
    cpabe_Setup(mpk2, msk2, pairing);
}

grpc::Status AAServiceImpl::GetABEPublicKey(
    grpc::ServerContext* context,
    const serverInteraction::GetABEPublicKeyRequest* request,
    serverInteraction::GetABEPublicKeyResponse* response) {
    int key_id = request->key_id();
    std::cout << "[AA] GetABEPublicKey called, key_id = " << key_id << std::endl;
    std::string pk_bytes;
    if (key_id == 1) {
        pk_bytes = cpabe_pk_to_json(mpk1).dump();
    } else if (key_id == 2) {
        pk_bytes = cpabe_pk_to_json(mpk2).dump();
    } else {
        pk_bytes = "unknown_key_id";
    }
    response->set_abe_public_key(pk_bytes); // 字段名与proto一致
    return grpc::Status::OK;
}

grpc::Status AAServiceImpl::GetABEPrivateKey(
    grpc::ServerContext* context,
    const serverInteraction::GetABEPrivateKeyRequest* request,
    serverInteraction::GetABEPrivateKeyResponse* response) {
    CP_ABE_SK sk;
    int layer = request->layer();
    std::cout << "layer = " << layer << std::endl;
    if (layer == 1){
        std::vector<int> attrs;
        for (const auto& attr : request->attrs()) {
            if (!attr.empty()) attrs.push_back(attr_map1[attr]);
        }
        attrs.push_back(0);
        init_CP_ABE_SK(sk, 10);
        cpabe_Keygen(sk, attrs.data(), mpk1, msk1, pairing);
    }else{
        std::vector<int> attrs;
        for (const auto& attr : request->attrs()) {
            if (!attr.empty()) attrs.push_back(attr_map2[attr]);
        }
        attrs.push_back(0);
        init_CP_ABE_SK(sk, 10);
        cpabe_Keygen(sk, attrs.data(), mpk2, msk2, pairing);
    }
    std::string sk_bytes = cpabe_sk_to_json(sk).dump();
    response->set_abe_secret_key(sk_bytes); // 按proto字段
    std::cout << "[AA] 发布第" << layer << "层私钥..." << std::endl;
    return grpc::Status::OK;
}

grpc::Status AAServiceImpl::ConvertPolicy(
    grpc::ServerContext* context,
    const serverInteraction::ConvertPolicyRequest* request,
    serverInteraction::ConvertPolicyResponse* response) {
    std::cout << "[AA] ConvertPolicy called, policy = " << request->policy() << std::endl;
    AST_Node* root = parser.parse(request->policy());
    if(request->layer() == 1) {
        auto access_list = converter1.convert(root);
        std::string result = access_to_string(access_list);
        std::cout << "[AA] ConvertedPolicy (Layer 1): " << result << std::endl;
        response->set_converted_policy(result); // 按proto字段
    } else if (request->layer() == 2) {
        auto access_list = converter2.convert(root);
        std::string result = access_to_string(access_list);
        std::cout << "[AA] ConvertedPolicy (Layer 2): " << result << std::endl;
        response->set_converted_policy(result); // 按proto字段
    } else {
        return grpc::Status(grpc::INVALID_ARGUMENT, "Invalid layer specified");
    }
    //response->set_converted_policy(result); // 按proto字段
    return grpc::Status::OK;
}

