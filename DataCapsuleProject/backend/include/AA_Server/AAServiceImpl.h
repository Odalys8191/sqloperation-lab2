#ifndef AA_SERVICE_IMPL_H
#define AA_SERVICE_IMPL_H
#include <grpcpp/grpcpp.h>
#include <unordered_map>
#include <string>
#include "../../protos/serverInteraction.grpc.pb.h"
#include "../common/abe/CPABE.h"
#include "../common/abeKeys.h"

class AAServiceImpl final : public serverInteraction::AttributeAuthorityService::Service {
public:
    AAServiceImpl();

    grpc::Status GetABEPublicKey(grpc::ServerContext* context, const serverInteraction::GetABEPublicKeyRequest* request, serverInteraction::GetABEPublicKeyResponse* response) override;
    grpc::Status GetABEPrivateKey(grpc::ServerContext* context, const serverInteraction::GetABEPrivateKeyRequest* request, serverInteraction::GetABEPrivateKeyResponse* response) override;
    grpc::Status ConvertPolicy(grpc::ServerContext* context, const serverInteraction::ConvertPolicyRequest* request, serverInteraction::ConvertPolicyResponse* response) override;

private:
    pairing_t pairing;
    CP_ABE_PK mpk1, mpk2;
    CP_ABE_MSK msk1, msk2;
    ExpressionParser parser;
    AccessConverter converter1; // 第一层属性转换器
    AccessConverter converter2; // 第二层属性转换器
    std::unordered_map<std::string, int> attr_map1; // 第一层属性映射
    std::unordered_map<std::string, int> attr_map2; // 第二层属性映射
};

#endif // AA_SERVICE_IMPL_H
