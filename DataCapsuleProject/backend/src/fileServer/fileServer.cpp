#include "httplib.h"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

const std::string BASE_FOLDER = "modelFinetune/finetunedModels";

bool is_valid_folder(const std::string &folder)
{
    // 防止路径穿越，比如 "../../etc"
    std::string full_path = BASE_FOLDER + "/" + folder;
    fs::path real_path = fs::weakly_canonical(full_path);
    fs::path base_path = fs::weakly_canonical(BASE_FOLDER);
    return real_path.string().find(base_path.string()) == 0 && fs::is_directory(real_path);
}

bool create_zip(const std::string &folder_name, const std::string &zip_name)
{
    std::string cmd = "cd " + BASE_FOLDER + " && zip -r ../../" + zip_name + " " + folder_name + " > /dev/null";
    return std::system(cmd.c_str()) == 0;
}

int main()
{
    httplib::Server svr;

    svr.Get("/download", [](const httplib::Request &req, httplib::Response &res)
            {
                auto folder = req.get_param_value("folder");

                if (folder.empty() || !is_valid_folder(folder))
                {
                    res.status = 400;
                    res.set_content("Invalid or missing folder parameter.", "text/plain");
                    return;
                }
                std::cout << "Valid Folder!" << std::endl;

                std::string zip_name = folder + ".zip";
                std::cout << "Creating zip file: " << zip_name << std::endl;
                if (!create_zip(folder, zip_name))
                {
                    res.status = 500;
                    res.set_content("Failed to create zip file.", "text/plain");
                    return;
                }
                std::cout << "Zip file created successfully." << std::endl;
                std::cout << "Opening zip file: " << zip_name << std::endl;

                std::ifstream file(zip_name, std::ios::binary);
                if (!file)
                {
                    res.status = 500;
                    res.set_content("Failed to open zip file.", "text/plain");
                    return;
                }
                std::cout << "Reading zip file content." << std::endl;

                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                res.set_content(content, "application/zip");
                res.set_header("Content-Disposition", "attachment; filename=\"" + zip_name + "\"");

                fs::remove(zip_name); // 清理临时文件
                std::cout << "Successfully sent zip file!" << std::endl; });

    std::cout << "Server running at http://0.0.0.0:1234" << std::endl;
    svr.listen("0.0.0.0", 1234);
}
