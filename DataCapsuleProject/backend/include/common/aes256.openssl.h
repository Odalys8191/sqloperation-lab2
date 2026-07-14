#pragma once

#include <iostream>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <vector>
#include <cstring>

class AES256Encryptor
{
public:
    AES256Encryptor(const std::vector<unsigned char> &key);
    AES256Encryptor(const std::vector<unsigned char> &key, const std::vector<unsigned char> &iv);
    std::vector<unsigned char> encrypt(const std::vector<unsigned char> &plaintext);  // 加密
    std::vector<unsigned char> decrypt(const std::vector<unsigned char> &ciphertext); // 解密

private:
    unsigned char key[32] = {0};            // AES-256 密钥 32 字节, 初始为全零
    unsigned char iv[AES_BLOCK_SIZE] = {0}; // AES CBC 模式 IV，初始为全零
};

std::string encryptAES256(const std::string &plaintext, const std::string &key);
std::string encryptAES256(const std::string &plaintext, const std::string &key, const std::string &iv);

std::string decryptAES256(const std::string &ciphertext, const std::string &key);
std::string decryptAES256(const std::string &ciphertext, const std::string &key, const std::string &iv);

void testAES256Vector();
void testAES256String();