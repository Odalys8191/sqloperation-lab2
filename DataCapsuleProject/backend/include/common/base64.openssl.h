#ifndef BASE64_OPENSSL_H
#define BASE64_OPENSSL_H
#include <iostream>
#include <cstring>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

std::string base64Encode(const unsigned char *bytes_to_encode, unsigned int in_len);
std::string base64Decode(std::string const &encoded_string);
#endif