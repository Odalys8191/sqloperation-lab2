#include "../../../include/consumer/computeAPI/modelFinetune.h"
#include "../../../include/common/tools.h"
#include <thread>

using json = nlohmann::json;

std::string ModelFineTune::getTemplateName(ModelFinetuneType type)
{
    switch (type)
    {
    case ModelFinetuneType::DeepSeek_R1_Distill_Qwen_1_5B:
        return "deepseek3";
    case ModelFinetuneType::DeepSeek_R1_Distill_Qwen_7B:
        return "deepseek3";
    case ModelFinetuneType::DeepSeek_R1_Distill_Qwen_14B:
        return "deepseek3";
    case ModelFinetuneType::DeepSeek_R1_Distill_Qwen_32B:
        return "deepseek3";
    case ModelFinetuneType::DeepSeek_R1:
        return "deepseek3";
    default:
        return "unknown";
    }
}

std::string ModelFineTune::getModelPath(ModelFinetuneType type)
{
    switch (type)
    {
    case ModelFinetuneType::DeepSeek_R1_Distill_Qwen_1_5B:
        return "pretrainedModels/DeepSeek-R1-Distill-Qwen-1.5B";
    case ModelFinetuneType::DeepSeek_R1_Distill_Qwen_7B:
        return "pretrainedModels/DeepSeek-R1-Distill-Qwen-7B";
    case ModelFinetuneType::DeepSeek_R1_Distill_Qwen_14B:
        return "pretrainedModels/DeepSeek-R1-Distill-Qwen-14B";
    case ModelFinetuneType::DeepSeek_R1_Distill_Qwen_32B:
        return "pretrainedModels/DeepSeek-R1-Distill-Qwen-32B";
    case ModelFinetuneType::DeepSeek_R1:
        return "pretrainedModels/DeepSeek-R1";
    default:
        return "unknown";
    }
}

// bool ModelFineTune::createDataset(const std::string &dc_id, const std::string &data)
// {
//     // 将胶囊中的数据转换为微调数据集，当前：保存为json文件
//     std::string dataset_path = "model_finetune_" + dc_id + ".json";
//     // 存储路径: modelFinetune/modelFinetuneDatasets/
//     std::string path = "modelFinetune/modelFinetuneDatasets/";
//     std::string filename = path + dataset_path;

//     std::ofstream file(filename, std::ios::binary | std::ios::out);
//     if (file.is_open())
//     {
//         file << data;
//         file.close();
//         std::cout << "[ModelFineTune]: Dataset successfully created at " << filename << std::endl;
//     }
//     else
//     {
//         std::cerr << "[ModelFineTune]: Error opening file." << std::endl;
//         return false;
//     }
//     return true;
// }

bool ModelFineTune::createDataset(const std::string &dc_id, const std::string &data)
{
    // Step 1: 保存新数据集文件
    std::string dataset_path = "model_finetune_" + dc_id + ".json";
    std::string path = "modelFinetune/data/";
    std::string filename = path + dataset_path;

    std::ofstream file(filename, std::ios::binary | std::ios::out);
    if (file.is_open())
    {
        file << data;
        file.close();
        std::cout << "[ModelFineTune]: Dataset successfully created at " << filename << std::endl;
    }
    else
    {
        std::cerr << "[ModelFineTune]: Error opening dataset file." << std::endl;
        return false;
    }

    // Step 2: 更新 data/dataset_info.json 文件
    std::string info_path = "modelFinetune/data/dataset_info.json";
    json dataset_info;

    // 读取原始 dataset_info.json
    std::ifstream info_file_in(info_path);
    if (info_file_in.is_open())
    {
        try
        {
            info_file_in >> dataset_info;
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ModelFineTune]: Failed to parse dataset_info.json: " << e.what() << std::endl;
            return false;
        }
        info_file_in.close();
    }
    else
    {
        std::cerr << "[ModelFineTune]: Cannot open dataset_info.json for reading." << std::endl;
        return false;
    }

    // Step 3: 添加新数据集条目
    dataset_info[dc_id] = {
        {"file_name", dataset_path}};

    // Step 4: 写回更新后的 JSON
    std::ofstream info_file_out(info_path, std::ios::out | std::ios::trunc);
    if (info_file_out.is_open())
    {
        info_file_out << dataset_info.dump(4); // 缩进为4，便于可读性
        info_file_out.close();
        std::cout << "[ModelFineTune]: dataset_info.json updated successfully." << std::endl;
    }
    else
    {
        std::cerr << "[ModelFineTune]: Cannot open dataset_info.json for writing." << std::endl;
        return false;
    }

    return true;
}

bool ModelFineTune::executeFineTune(const ModelFinetuneType &model_type,
                                    const std::string &dataset,
                                    const std::string &output_model_path)
{
    std::ifstream templateFile("modelFinetune/trainYaml/template.yaml");
    if (!templateFile)
    {
        std::cerr << "Failed to open YAML template file." << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << templateFile.rdbuf();
    std::string content = buffer.str();

    // 替换字段
    std::string model_path = getModelPath(model_type);
    std::string template_name = getTemplateName(model_type);

    auto replace_all = [](std::string &s, const std::string &from, const std::string &to)
    {
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos)
        {
            s.replace(pos, from.length(), to);
            pos += to.length();
        }
    };

    replace_all(content, "model_path_to_replace", model_path);
    replace_all(content, "dataset_to_replace", dataset);
    replace_all(content, "template_name_to_replace", template_name);
    replace_all(content, "output_path_to_replace", output_model_path);

    // 写入新的 YAML 文件
    std::string prefix_path = "modelFinetune/";
    std::string config_path = "trainYaml/" + dataset + ".yaml";
    std::ofstream configFile(prefix_path + config_path);
    if (!configFile)
    {
        std::cerr << "Failed to create temp config file." << std::endl;
        return false;
    }
    configFile << content;
    configFile.close();

    // 执行命令
    std::string command = "cd modelFinetune && llamafactory-cli train " + config_path;
    std::cout << "Executing command: " << command << std::endl;
    // runCommandInThread(command);

    return true;
}

void ModelFineTune::runCommandInThread(const std::string &cmd)
{
    std::thread([cmd]()
                {
        std::cout << "[训练线程] 开始执行命令: " << cmd << std::endl;
        int ret = system(cmd.c_str());
        if (ret == 0)
            std::cout << "[训练线程] 命令执行成功。" << std::endl;
        else
            std::cerr << "[训练线程] 命令执行失败，错误码: " << ret << std::endl; })
        .detach(); // 主线程不等待
}