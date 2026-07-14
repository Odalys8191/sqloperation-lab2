#pragma once

#include <iostream>
#include "json.hpp"

using json = nlohmann::json;

std::string genSha256Sum(const std::string &data);

void saveToFile(const std::string &content, const std::string filename, const std::string path);

std::string readFromFile(const std::string filename, const std::string path);

std::string readFromFile(const std::string file_path);

void exportToJsonFile(const json &json_data, const std::string &output_file_name);

json stringToJson(const std::string &str);

std::string jsonToString(const json &j);

bool checkPolicyStructure(const json &reference_policy, const json &check_policy);