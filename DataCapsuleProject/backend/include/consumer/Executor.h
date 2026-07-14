#pragma once

#include <iostream>
#include <vector>
#include "../common/json.hpp"

using json = nlohmann::json;

class Executor
{
public:
    Executor(const std::string &dc_key) : dc_key_(dc_key) {}                                        // initialize with dc_key and iv
    ~Executor() {}                                                                                  // destructor
    void setDCKey(const std::string &dc_key);                                                       // set dc_key_
    bool decryptDataCapsuleDataAES(const json &dc, const std::string &iv, std::string &plain_text); // decrypt the dc's data
    // bool computeResult(const std::string &dc_id, const std::string &plain_text, const json &access_policy, std::string &compute_result);
    bool computeResult(const std::string &dc_id, const std::string &plain_text, const json &data_capsule_info, const json &access_policy, std::string &compute_result, const json &program_policy);

private:
    std::string dc_key_; // from CertificateAuthority
};