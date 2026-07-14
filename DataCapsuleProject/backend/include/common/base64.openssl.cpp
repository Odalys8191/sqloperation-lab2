#include "base64.openssl.h"
// Base64 编码
std::string base64Encode(const unsigned char *bytes_to_encode, unsigned int in_len)
{
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // 取消换行

    BIO_write(bio, bytes_to_encode, in_len);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);
    BIO_set_close(bio, BIO_NOCLOSE);
    BIO_free_all(bio);

    return std::string(bufferPtr->data, bufferPtr->length);
}

// Base64 解码
std::string base64Decode(std::string const &encoded_string)
{
    BIO *bio, *b64;
    char buffer[512];
    int decoded_size;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf(encoded_string.data(), encoded_string.size());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // 取消换行

    std::string decoded_string;
    while ((decoded_size = BIO_read(bio, buffer, sizeof(buffer))) > 0)
    {
        decoded_string.append(buffer, decoded_size);
    }
    BIO_free_all(bio);

    return decoded_string;
}