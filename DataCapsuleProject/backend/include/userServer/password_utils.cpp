#include "password_utils.h"
#include <random>
#include <stdexcept>
#include <argon2.h>

#define HASHLEN 32
#define SALTLEN 16
#define ENCODEDLEN 128

void generate_random_salt(uint8_t* salt, size_t len) {
    std::random_device rd;
    std::uniform_int_distribution<int> dist(0, 255);
    for (size_t i = 0; i < len; ++i)
        salt[i] = static_cast<uint8_t>(dist(rd));
}

std::string hash_password_argon2id(const std::string& password) {
    uint8_t salt[SALTLEN];
    generate_random_salt(salt, SALTLEN);

    char encoded[ENCODEDLEN];

    int ret = argon2id_hash_encoded(
        2, 1 << 16, 1,                    // time_cost, memory_cost, parallelism
        password.data(), password.size(),
        salt, SALTLEN,
        HASHLEN,
        encoded, ENCODEDLEN
    );

    if (ret != ARGON2_OK) {
        throw std::runtime_error(argon2_error_message(ret));
    }

    return std::string(encoded);
}

bool verify_password_argon2id(const std::string& encoded_hash, const std::string& password) {
    int ret = argon2id_verify(encoded_hash.c_str(), password.data(), password.size());
    return ret == ARGON2_OK;
}
