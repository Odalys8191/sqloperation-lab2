#pragma once
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <cstdlib>
#include "../../common/computeType.h"

class ModelFineTune
{
public:
    ModelFineTune() {}
    bool createDataset(const std::string &dc_id, const std::string &data); // 将胶囊中的数据转换为微调数据集，当前：保存为json文件
    bool executeFineTune(const ModelFinetuneType &model_type,
                         const std::string &dataset,
                         const std::string &output_model_path);

private:
    std::string getTemplateName(ModelFinetuneType type);
    std::string getModelPath(ModelFinetuneType type);
    void runCommandInThread(const std::string &cmd);
};
