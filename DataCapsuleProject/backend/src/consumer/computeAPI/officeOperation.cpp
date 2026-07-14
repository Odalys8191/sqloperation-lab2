#include "../../../include/consumer/computeAPI/officeOperation.h"
#include "../../../include/common/tools.h"
#include "../../../include/common/computeType.h"

#include <random>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static bool produceDataCapsule(const json &data_capsule_info, const std::string &office_file_path, std::string &new_dc_id)
{
    (void)data_capsule_info;
    (void)office_file_path;
    (void)new_dc_id;
    return true;
}

bool OfficeOperation::createOfficeFile(const std::string &dc_id, const std::string &file_extension, const std::string &data)
{
    (void)dc_id;
    (void)file_extension;
    (void)data;
    return true;
}

bool OfficeOperation::executeOfficeOperation(const json &data_capsule_info, std::string &result)
{
    (void)data_capsule_info;
    (void)result;
    return true;
}

// 辅助函数：检查端口是否可用
static bool isPortAvailable(int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)))
    {
        close(sock);
        return false;
    }

    close(sock);
    return true;
}

// 辅助函数：生成随机字符串
static std::string generateRandomString(size_t length)
{
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::string result;
    result.reserve(length);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);

    for (size_t i = 0; i < length; ++i)
    {
        result += alphanum[dis(gen)];
    }

    return result;
}

// 辅助函数：复制文件
static bool copyFile(const std::string &src, const std::string &dest)
{
    std::ifstream in(src, std::ios::binary);
    if (!in)
        return false;

    std::ofstream out(dest, std::ios::binary);
    if (!out)
        return false;

    out << in.rdbuf();
    return out.good();
}

// 静态函数：监控容器退出并处理文件
static void handleContainerExit(const std::string &container_id,
                                const std::string &office_file_path,
                                const json &data_capsule_info)
{
    // 等待容器退出
    std::string wait_cmd = "docker wait " + container_id;
    FILE *pipe = popen(wait_cmd.c_str(), "r");
    if (!pipe)
    {
        std::cerr << "[OfficeOperation-handleContainerExit]: Failed to monitor container exit." << std::endl;
        return;
    }

    char buffer[128];
    if (!fgets(buffer, sizeof(buffer), pipe))
    {
        pclose(pipe);
        return;
    }
    pclose(pipe);

    // 容器已退出，处理更新后的文件
    // --------------------------------- 生成新胶囊 ---------------------------------
    std::string new_dc_id = "";
    if (produceDataCapsule(data_capsule_info, office_file_path, new_dc_id))
    {
        if (new_dc_id.empty())
        {
            std::cout << "[OfficeOperation-handleContainerExit]: Office policy exhausted; no new Data Capsule generated." << std::endl;
        }
        else
        {
            std::cout << "[OfficeOperation-handleContainerExit]: New Data Capsule generated with ID: " << new_dc_id << std::endl;
        }
    }
    else
    {
        std::cerr << "[OfficeOperation-handleContainerExit]: Failed to generate new Data Capsule." << std::endl;
        return;
    }
}
