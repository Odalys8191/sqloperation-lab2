#pragma once
#include <string>
#include <cstdint>

// 生成随机 salt（内部调用）
void generate_random_salt(uint8_t* salt, size_t len);

// 对密码进行 Argon2id 加盐哈希
std::string hash_password_argon2id(const std::string& password);

// 验证密码与编码哈希是否匹配
bool verify_password_argon2id(const std::string& encoded_hash, const std::string& password);
