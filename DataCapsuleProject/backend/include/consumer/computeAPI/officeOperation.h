#pragma once
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <cstdlib>
#include "../../common/computeType.h"
#include "../../common/json.hpp"
using json = nlohmann::json;

class OfficeOperation
{
public:
    OfficeOperation() {}
    bool executeOfficeOperation(const json &data_capsule_info, std::string &result);
    bool createOfficeFile(const std::string &dc_id, const std::string &file_extension, const std::string &data); // 保存为 dc_id.file_extension 文件

private:
    std::string office_file_path_; // 保存文件的路径
    std::string file_extension_;   // 保存文件的扩展名
};
