#pragma once

#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <vector>
#include "../../include/common/json.hpp"
#include "../../include/common/encryptAlgType.h"
#include "../../include/common/signature.openssl.h"
#include "../../include/common/aes256.openssl.h"
#include "../../include/common/base64.openssl.h"
#include "../../include/common/tools.h"
#include "../../include/common/abe/CPABE.h"
#include "../../protos/serverInteraction.pb.h"
#include "../../protos/serverInteraction.grpc.pb.h"
#include "../../include/common/serviceAddress.h"
#include "../../include/common/abeKeys.h"
#include <grpcpp/grpcpp.h>
using json = nlohmann::json;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientWriter;

using namespace serverInteraction;

class ProducerServiceImpl final : public ProducerService::Service
{
public:
    explicit ProducerServiceImpl(std::string tee_id) : pro_tee_id(tee_id) {};

    grpc::Status GenerateDataCapsule(::grpc::ServerContext *context, const serverInteraction::GenerateDataCapsuleRequest *dc_input,
                                     serverInteraction::GenerateDataCapsuleResponse *reply) override;

private:
    std::string pro_tee_id;
};

std::string initialTEE();

void runProducerServer();

json genDataCapsule(
    const std::string &dc_id,
    const std::string &producer_tee_id,
    const json &policy,
    EncryptAlgType crypto_alg,
    const std::string &plain_data,
    unsigned long long data_len,
    const std::string &encrypt_key,
    const json &attributes, 
    const std::string &converted_policy1,  // 第一层ABE加密的访问策略
    const std::string &converted_policy2   // 第二层ABE加密的访问策略
);