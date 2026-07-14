#pragma once

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <string>
#include <iostream>

class RSASignature
{
public:
    RSASignature() : rsa_key(nullptr) {}
    ~RSASignature();
    bool generateKeyPair(int bits = 2048);
    std::string sign(const std::string &data);
    bool verify(const std::string &data, const std::string &signature);
    bool loadPublicKey(const std::string &publicKey);
    std::string getPublicKey();

private:
    EVP_PKEY *rsa_key;

    std::string getLastError();
};

std::string createRSASignature(const std::string &data, std::string &public_key);

bool verifyRSASignature(const std::string &data, const std::string &signature, const std::string &public_key);

bool testRSASignature();
