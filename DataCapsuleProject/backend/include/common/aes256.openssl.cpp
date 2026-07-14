#include "aes256.openssl.h"

AES256Encryptor::AES256Encryptor(const std::vector<unsigned char> &key)
{
    // 确保密钥长度是 256 位（32 字节）
    if (key.size() != 32)
    {
        throw std::invalid_argument("Key must be 256 bits (32 bytes).");
    }
    std::memcpy(this->key, key.data(), 32);
}

AES256Encryptor::AES256Encryptor(const std::vector<unsigned char> &key, const std::vector<unsigned char> &iv)
{
    // 确保密钥长度是 256 位（32 字节）
    if (key.size() != 32)
    {
        throw std::invalid_argument("Key must be 256 bits (32 bytes).");
    }
    std::memcpy(this->key, key.data(), 32);

    // 确保 IV 长度是 128 位（16 字节）
    if (iv.size() != AES_BLOCK_SIZE)
    {
        throw std::invalid_argument("IV must be 128 bits (16 bytes).");
    }
    std::memcpy(this->iv, iv.data(), AES_BLOCK_SIZE);
}

// 加密函数
std::vector<unsigned char> AES256Encryptor::encrypt(const std::vector<unsigned char> &plaintext)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr)
    {
        throw std::runtime_error("Failed to create EVP_CIPHER_CTX.");
    }

    int len;
    int ciphertext_len;
    std::vector<unsigned char> ciphertext(plaintext.size() + AES_BLOCK_SIZE);

    // 初始化加密上下文，选择 AES-256 加密算法
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize encryption.");
    }

    // 执行加密
    if (1 != EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size()))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to encrypt data.");
    }
    ciphertext_len = len;

    // 结束加密，获取最终的密文数据
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize encryption.");
    }
    ciphertext_len += len;

    // 释放加密上下文
    EVP_CIPHER_CTX_free(ctx);

    // 返回加密后的数据
    ciphertext.resize(ciphertext_len);
    return ciphertext;
}

// 解密函数
std::vector<unsigned char> AES256Encryptor::decrypt(const std::vector<unsigned char> &ciphertext)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr)
    {
        throw std::runtime_error("Failed to create EVP_CIPHER_CTX.");
    }

    int len;
    int plaintext_len;
    std::vector<unsigned char> plaintext(ciphertext.size());

    // 初始化解密上下文，选择 AES-256 解密算法
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize decryption.");
    }

    // 执行解密
    if (1 != EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size()))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to decrypt data.");
    }
    plaintext_len = len;

    // 结束解密，获取最终的明文数据
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize decryption.");
    }
    plaintext_len += len;

    // 释放解密上下文
    EVP_CIPHER_CTX_free(ctx);

    // 返回解密后的数据
    plaintext.resize(plaintext_len);
    return plaintext;
}

std::string encryptAES256(const std::string &plaintext, const std::string &key)
{
    std::vector<unsigned char> key_data(key.begin(), key.end());
    AES256Encryptor encryptor(key_data);
    std::vector<unsigned char> plaintext_data(plaintext.begin(), plaintext.end());
    std::vector<unsigned char> ciphertext = encryptor.encrypt(plaintext_data);
    return std::string(ciphertext.begin(), ciphertext.end());
}

std::string decryptAES256(const std::string &ciphertext, const std::string &key)
{
    std::vector<unsigned char> key_data(key.begin(), key.end());
    AES256Encryptor encryptor(key_data);
    std::vector<unsigned char> ciphertext_data(ciphertext.begin(), ciphertext.end());
    std::vector<unsigned char> plaintext = encryptor.decrypt(ciphertext_data);
    return std::string(plaintext.begin(), plaintext.end());
}

std::string encryptAES256(const std::string &plaintext, const std::string &key, const std::string &iv)
{
    std::vector<unsigned char> key_data(key.begin(), key.end());
    std::vector<unsigned char> iv_data(iv.begin(), iv.end());
    // std::cout << "[encryptAES256] iv" << iv << std::endl;
    // std::cout << "[encryptAES256] iv size: " << iv.size() << std::endl;
    AES256Encryptor encryptor(key_data, iv_data);
    std::vector<unsigned char> plaintext_data(plaintext.begin(), plaintext.end());
    std::vector<unsigned char> ciphertext = encryptor.encrypt(plaintext_data);
    return std::string(ciphertext.begin(), ciphertext.end());
}

std::string decryptAES256(const std::string &ciphertext, const std::string &key, const std::string &iv)
{
    std::vector<unsigned char> key_data(key.begin(), key.end());
    std::vector<unsigned char> iv_data(iv.begin(), iv.end());
    // std::cout << "[decryptAES256] iv" << iv << std::endl;
    // std::cout << "[decryptAES256] iv size: " << iv.size() << std::endl;
    AES256Encryptor encryptor(key_data, iv_data);
    std::vector<unsigned char> ciphertext_data(ciphertext.begin(), ciphertext.end());
    std::vector<unsigned char> plaintext = encryptor.decrypt(ciphertext_data);
    return std::string(plaintext.begin(), plaintext.end());
}

void testAES256Vector()
{
    std::cout << "AES-256 Vector Test" << std::endl;
    // 示例密钥，32 字节
    std::vector<unsigned char> key(32, 0x01); // 简单的密钥
    // 实例化加密器
    AES256Encryptor encryptor(key);
    // 待加密明文
    std::string plaintext = "This is a test message for AES-256 encryption.";
    std::vector<unsigned char> plaintext_data(plaintext.begin(), plaintext.end());
    // 加密
    std::vector<unsigned char> ciphertext = encryptor.encrypt(plaintext_data);
    std::cout << "Encrypted text: ";
    for (unsigned char c : ciphertext)
    {
        std::cout << std::hex << (int)c << " ";
    }
    std::cout << std::endl;
    // 解密
    std::vector<unsigned char> decrypted = encryptor.decrypt(ciphertext);
    std::string decrypted_text(decrypted.begin(), decrypted.end());
    std::cout << "Decrypted text: " << decrypted_text << std::endl;
}

void testAES256String()
{
    std::cout << "AES-256 String Test" << std::endl;
    // 示例密钥，32 字节
    std::string key = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                       0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                       0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                       0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

    // 加密
    std::string plaintext = "This is a test message for AES-256 encryption.";
    std::string ciphertext = encryptAES256(plaintext, key);
    std::cout << "Encrypted text: " << ciphertext << std::endl;

    // 解密
    std::string decrypted = decryptAES256(ciphertext, key);
    std::cout << "Decrypted text: " << decrypted << std::endl;
}
