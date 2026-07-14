#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <openssl/sha.h>

#include "./json.hpp"
using json = nlohmann::json;

#include "./tools.h"

// Compute the sha256sum
std::string genSha256Sum(const std::string &data)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(data.c_str()), data.size(), hash);

    std::string output;
    for (const auto &byte : hash)
    {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", byte);
        output.append(buf);
    }
    return output;
}

void saveToFile(const std::string &content, const std::string filename, const std::string path)
{
    std::ofstream file(path + filename, std::ios::binary | std::ios::out);
    if (file.is_open())
    {
        file << content;
        file.close();
        std::cout << "[Storage]: File successfully saved to " << path + filename << std::endl;
    }
    else
    {
        std::cerr << "[Error]: Error opening file.\n";
    }
}

std::string readFromFile(const std::string filename, const std::string path)
{
    std::ifstream file(path + filename, std::ios::binary);
    if (file.is_open())
    {
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
    else
    {
        std::cerr << "无法打开文件：" << path + filename << std::endl;
        return "";
    }
}

std::string readFromFile(const std::string file_path)
{
    std::ifstream file(file_path, std::ios::binary);
    if (file.is_open())
    {
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
    else
    {
        std::cerr << "无法打开文件：" << file_path << std::endl;
        return "";
    }
}

/*exportToJsonFile
 * 功能：将json数据导出到文件
 * 输入：
 * json_data -- json数据
 * output_file_name -- 文件名
 */
void exportToJsonFile(const json &json_data, const std::string &output_file_name)
{
    // Create and open a json file
    // string full_path = "../storage/dataUser/accessPolicy/" + output_file_name;
    std::ofstream file(output_file_name, std::ios::binary | std::ios::out);

    if (file.is_open())
    {
        // Write the json data to the file with indentation (4 spaces)
        file << json_data.dump(4);
        file.close();
#ifdef DEBUG
        std::cout << "[Storage]: JSON data successfully exported to " << output_file_name << std::endl;
#endif
    }
    else
    {
        std::cerr << "Error: opening file for writing result." << std::endl;
    }
}

json stringToJson(const std::string &str)
{
    return json::parse(str);
}

std::string jsonToString(const json &j)
{
    return j.dump();
}

bool checkPolicyStructure(const json &reference_policy, const json &check_policy)
{
    if (reference_policy.type() != check_policy.type())
    {
        // 类型不一致
        return false;
    }

    if (reference_policy.is_object())
    {
        // 如果是对象，检查每个键
        for (auto it = reference_policy.begin(); it != reference_policy.end(); ++it)
        {
            const std::string &key = it.key();
            if (!check_policy.contains(key))
            {
                // 缺少键
                return false;
            }
            if (!checkPolicyStructure(it.value(), check_policy[key]))
            {
                // 递归检查值的结构
                return false;
            }
        }
    }
    else if (reference_policy.is_array())
    {
        // 如果是数组，检查元素类型是否一致
        if (check_policy.size() < reference_policy.size())
        {
            return false;
        }
        for (size_t i = 0; i < reference_policy.size(); ++i)
        {
            if (!checkPolicyStructure(reference_policy[i], check_policy[i]))
            {
                return false;
            }
        }
    }
    // 如果是基本类型，类型一致即可
    return true;
}