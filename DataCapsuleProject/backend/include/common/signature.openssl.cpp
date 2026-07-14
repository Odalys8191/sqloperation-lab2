#include "signature.openssl.h"

RSASignature::~RSASignature()
{
    if (rsa_key)
    {
        EVP_PKEY_free(rsa_key);
    }
}

bool RSASignature::generateKeyPair(int bits)
{
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    EVP_PKEY_keygen_init(ctx);
    EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits);
    EVP_PKEY_keygen(ctx, &rsa_key);
    EVP_PKEY_CTX_free(ctx);
    return rsa_key != nullptr;
}

std::string RSASignature::sign(const std::string &data)
{
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    unsigned char *signature = new unsigned char[EVP_PKEY_size(rsa_key)];
    unsigned int signature_len;

    EVP_SignInit(mdctx, EVP_sha256());
    EVP_SignUpdate(mdctx, data.c_str(), data.length());
    if (EVP_SignFinal(mdctx, signature, &signature_len, rsa_key) != 1)
    {
        delete[] signature;
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Error signing data: " + getLastError());
    }

    std::string signed_data(reinterpret_cast<char *>(signature), signature_len);
    delete[] signature;
    EVP_MD_CTX_free(mdctx);
    return signed_data;
}

bool RSASignature::verify(const std::string &data, const std::string &signature)
{
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    EVP_SignInit(mdctx, EVP_sha256());
    EVP_SignUpdate(mdctx, data.c_str(), data.length());

    int result = EVP_VerifyFinal(mdctx,
                                 reinterpret_cast<const unsigned char *>(signature.c_str()),
                                 signature.length(),
                                 rsa_key);
    EVP_MD_CTX_free(mdctx);
    return result == 1;
}

bool RSASignature::loadPublicKey(const std::string &publicKey)
{
    BIO *bio = BIO_new_mem_buf(publicKey.data(), publicKey.size());
    rsa_key = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    return rsa_key != nullptr;
}

std::string RSASignature::getPublicKey()
{
    BIO *bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PUBKEY(bio, rsa_key);
    char *public_key = nullptr;
    long public_key_len = BIO_get_mem_data(bio, &public_key);
    std::string result(public_key, public_key_len);
    BIO_free(bio);
    return result;
}

std::string RSASignature::getLastError()
{
    char buffer[120];
    ERR_error_string_n(ERR_get_error(), buffer, sizeof(buffer));
    return std::string(buffer);
}

std::string createRSASignature(const std::string &data, std::string &public_key)
{
    RSASignature rsa;
    rsa.generateKeyPair();
    public_key = rsa.getPublicKey();
    return rsa.sign(data);
}

bool verifyRSASignature(const std::string &data, const std::string &signature, const std::string &public_key)
{
    RSASignature rsa;
    rsa.loadPublicKey(public_key);
    return rsa.verify(data, signature);
}

bool testRSASignature()
{
    try
    {
        RSASignature rsa;
        rsa.generateKeyPair();

        std::string data = "Hello, this is a test message.";
        std::string signature = rsa.sign(data);

        std::cout << "Public Key:\n"
                  << rsa.getPublicKey() << std::endl;

        rsa.loadPublicKey(rsa.getPublicKey());

        if (rsa.verify(data, signature))
        {
            std::cout << "Signature is valid!" << std::endl;
            return true;
        }
        else
        {
            std::cout << "Signature is invalid!" << std::endl;
            return false;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return false;
}
