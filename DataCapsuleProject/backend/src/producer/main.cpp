#include "../../include/producer/producer.h"

// 假设 data_capsule 是你的胶囊 JSON 对象
void decryptCapsule(const json &data_capsule, CP_ABE_SK &abe_sk1, CP_ABE_SK &abe_sk2, pairing_t &pairing)
{
    // 解析访问策略
    // 与AA_Server的ConvertPolicy交互，这里假设你已经有了Access1和Access2
    int Access1[MAX_ACCESS_SIZE] = {2, 1, 1, 2, 2, 2, -1, -2, 2, 2, -3, -4, 0};      // 根据实际情况设置Access1
    int Access2[MAX_ACCESS_SIZE] = {3, 1, 1, 2, -6, 2, 2, -7, -9, 2, 2, -8, -10, 0}; // 根据实际情况设置Access2
    // 1. 解密第二层ABE密文，获得对称密钥key2
    json ct2_json = data_capsule["MetaData"]["ABEEncryption"];
    CT ct2 = jsonToCT(ct2_json, pairing);
    unsigned char *key2 = new unsigned char[32];
    cpabe_Decrypt(Access2, pairing, ct2, abe_sk2, key2); // 用第二层ABE私钥解密得到key2

    // 2. 用key2和IV解密ct1（AES密文）
    std::string ciphertext_ct1_base64 = data_capsule["Ciphertext_ct1"];
    std::string iv_ct1_base64 = data_capsule["MetaData"]["SymmetricEncryption"]["AESIV_ct1"];
    std::string ciphertext_ct1 = base64Decode(ciphertext_ct1_base64);
    std::string iv_ct1 = base64Decode(iv_ct1_base64);
    std::string ct1_serialized = decryptAES256(ciphertext_ct1, std::string(reinterpret_cast<char *>(key2), 32), iv_ct1); // 你需要实现decryptAES256

    // 3. 反序列化ct1并用ABE私钥解密数据
    CT ct1;
    cpabe_DeserializeCT(reinterpret_cast<const unsigned char *>(ct1_serialized.data()), ct1_serialized.size(), ct1, pairing);
    unsigned char *key1 = new unsigned char[32];
    cpabe_Decrypt(Access1, pairing, ct1, abe_sk1, key1); // 用第一层ABE私钥解密得到key1
    // 4. 解密数据
    std::string ciphertext_data_base64 = data_capsule["Data"];
    std::string iv_data_base64 = data_capsule["MetaData"]["SymmetricEncryption"]["AESIV_data"];
    std::string ciphertext_data = base64Decode(ciphertext_data_base64);
    std::string iv_data = base64Decode(iv_data_base64);
    std::string data_out = decryptAES256(ciphertext_data, std::string(reinterpret_cast<char *>(key1), 32), iv_data); // 你需要实现decryptAES256
    // 5. 输出解密后的数据
    std::cout << "解密得到对称密钥key2: " << std::string(reinterpret_cast<char *>(key2), 32) << std::endl;
    std::cout << "解密得到对称密钥key1: " << std::string(reinterpret_cast<char *>(key1), 32) << std::endl;
    // std::cout << "解密得到ct1的明文: " << ct1_serialized << std::endl;
    std::cout << "解密得到数据的明文: " << data_out << std::endl;
    delete[] key2; // 释放key2内存
    delete[] key1; // 释放key1内存
}

void test_genCapsule()
{
    // 构造测试参数
    std::string dc_id = "DC_TEST_001";
    std::string producer_tee_id = "TEE_TEST_001";
    // 假设你有一个访问策略,这里用AccessStructure作为示例
    json policy = {
        {"alg", "cpabe"},
        {"AccessStructure", {{"1", "(role:admin AND clearance:high) OR (dept:finance AND role:user)"}, {"2", "((team:alpha AND project:X) OR (team:beta AND project:Y) OR dept:hr)"}}}};
    EncryptAlgType crypto_alg = EncryptAlgType::Encrypt_AES256_CBC;
    std::string plain_data = "This is the secret data for capsule.";
    unsigned long long data_len = plain_data.size();
    std::string encrypt_key = "abcdefghijklmnopqrstuvwxyzabcdef"; // 32字节对称密钥
    json attributes = {
        {"AccessStructure", {{"1", "(role:admin AND clearance:high) OR (dept:finance AND role:user)"}, {"2", "((team:alpha AND project:X) OR (team:beta AND project:Y) OR dept:hr)"}}},
        {"DataDescription", "测试数据胶囊"},
        {"Columns", {"col1", "col2", "col3"}}};
    // 调用生成胶囊函数
    std::string converted_policy1 = "2,1,1,2,2,2,-1,-2,2,2,-3,-4,0";
    std::string converted_policy2 = "3,1,1,2,-6,2,2,-7,-9,2,2,-8,-10,0";
    json capsule = genDataCapsule(dc_id, producer_tee_id, policy, crypto_alg, plain_data, data_len, encrypt_key, attributes, converted_policy1, converted_policy2);

    // 输出主要字段
    std::cout << "=== genCapsule 测试结果 ===" << std::endl;
    std::cout << "DCID: " << capsule["MetaData"]["DCID"] << std::endl;
    std::cout << "TEEID: " << capsule["MetaData"]["PTEEID"] << std::endl;
    std::cout << "Policy: " << capsule["MetaData"]["Policy"].dump() << std::endl;
    std::cout << "ABEEncryption: " << capsule["MetaData"]["ABEEncryption"].dump() << std::endl;
    std::cout << "SymmetricEncryption: " << capsule["MetaData"]["SymmetricEncryption"].dump() << std::endl;
    std::cout << "DataIntro: " << capsule["MetaData"]["DataIntro"].dump() << std::endl;
    std::cout << "Ciphertext_ct1: " << capsule["Ciphertext_ct1"].get<std::string>().substr(0, 32) << "..." << std::endl;
    std::cout << "Data: " << capsule["Data"].get<std::string>().substr(0, 32) << "..." << std::endl;
    std::cout << "Signature: " << capsule["Signature"].get<std::string>().substr(0, 32) << "..." << std::endl;
    std::cout << "=== 完整胶囊JSON ===" << std::endl;
    std::cout << capsule.dump(2) << std::endl;
    // 测试解密
    pairing_t pairing;
    init_pairing(pairing, PUBLIC_PARAM_PATH); // 假设你有一个pairing参数文件
    CP_ABE_SK abe_sk1, abe_sk2;
    init_CP_ABE_SK(abe_sk1, 10);                          // 第一层私钥
    init_CP_ABE_SK(abe_sk2, 10);                          // 第二层私钥
    deSerializeFromFile(abe_sk1, USER_SK1_PATH, pairing); // 第一层私钥
    deSerializeFromFile(abe_sk2, USER_SK2_PATH, pairing); // 第二层私钥
    decryptCapsule(capsule, abe_sk1, abe_sk2, pairing);
}

int main(int argc, char **argv)
{
    // test_genCapsule();
    runProducerServer();

    return 0;
}