#pragma once

#include "./client.h"
#include "../common/json.hpp"

using json = nlohmann::json;

/* APIs for cloud storage interaction */

// get the dc from cloud storage
bool getDataCapsule(const std::string &dc_id,
                    const std::string &cloud_server_ip,
                    const std::string &cloud_server_port,
                    json &data_capsule);

// get the dc's encrypted password from cloud storage
bool getEncryptedDataCapsulePasswordProgramPolicy(const std::string &dc_id,
                                                  const std::string &cloud_server_ip,
                                                  const std::string &cloud_server_port,
                                                  std::string &encrypted_password,
                                                  json &program_policy);

/* APIs for CertificateAuthority interaction */

// initialize TEE
bool initializeTEE(const std::string &signature_public_key,
                   const std::string &ca_server_ip,
                   const std::string &ca_server_port, std::string &tee_id, std::string &certificate);

// get the dc's data key (password key) from the CertificateAuthority
bool getDataCapsuleKeyWithAdditionalInformation(const std::string &dc_id,
                                                const std::string &ca_server_ip,
                                                const std::string &ca_server_port,
                                                const std::string &cloud_server_ip,
                                                const std::string &cloud_server_port,
                                                json &dc_key_info);

// get the Producer(who produces the dc)'s signature public key from the CertificateAuthority
bool getProducerSignaturePublicKey(const std::string &tee_id,
                                   const std::string &ca_server_ip,
                                   const std::string &ca_server_port, std::string &producer_signature_public_key);

/* APIs for etcd interaction */

// get the dc's state from etcd key-value storage
bool getDataCapsuleState(const std::string &dc_id,
                         const std::string &etcd_server_ip,
                         const std::string &etcd_server_port, json &state);

// update the dc's state to etcd key-value storage
bool UpdateDataCapsuleState(const std::string &dc_id,
                            const json &state,
                            const std::string &etcd_server_ip,
                            const std::string &etcd_server_port);

/* APIs for Intergrated operations */

bool consumeDataCapsule(const std::string &user_id_input,
                        const std::string &dc_id_input,
                        const std::string &dc_password_input,
                        const json &access_policy,
                        std::string &compute_result,
                        const std::string &cloud_server_ip,
                        const std::string &cloud_server_port,
                        const std::string &ca_server_ip,
                        const std::string &ca_server_port,
                        const std::string &etcd_server_ip,
                        const std::string &etcd_server_port,
                        std::string *error_message = nullptr);
