#pragma once

#include <iostream>
#include "../common/json.hpp"

using json = nlohmann::json;

class Verifier
{
public:
    Verifier(const std::string &user_id, const std::string &dc_id, const std::string &dc_password) : user_id_input_(user_id), dc_id_input_(dc_id), dc_password_input_(dc_password) {}
    ~Verifier() {}
    void setDCKey(const std::string &dc_key);
    void setProducerSignaturePublicKey(const std::string &producer_signature_public_key);
    bool verifyDataCapsuleState(const json &state);
    bool verifyDataCapsuleAvailability(const std::string &encrypted_password, const json &dc_key_info);
    bool verifyDataCapsuleSignature(const json &dc, const std::string &producer_signature_public_key);
    bool verifyDataCapsuleSignature(const json &dc);
    bool verifyDataCapsuleProgramPolicy(const json &program_policy, const json &access_policy);

private:
    std::string user_id_input_;                 // from user
    std::string dc_id_input_;                   // from user
    std::string dc_password_input_;             // from user
    std::string dc_key_;                        // from CertificateAuthority
    std::string producer_signature_public_key_; // from CertificateAuthority
};