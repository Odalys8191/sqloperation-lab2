#pragma once

#include <iostream>
#include <string>
#include "../common/computeType.h"

#include "../common/json.hpp"
using json = nlohmann::json;

#include "computeAPI/dataStatistics.h"
#include "computeAPI/sqlOperation.h"
#include "computeAPI/databaseOperation.h"
#include "computeAPI/modelFinetune.h"
#include "computeAPI/officeOperation.h"
#include "computeAPI/sql_policy_executor.h"

bool computeDataStatistics(const json &access_policy, const std::string &content, std::string &result);

bool computeSQLOperation(const json &access_policy, const std::string &content, std::string &result);

bool computecodeSQLOperation(const json &access_policy, const std::string &content, std::string &result, const json &program_policy);

bool computeDatabasedOperation(const json &access_policy, const std::string &content, std::string &result);

bool computeDatabasedOperation(const std::string &dc_id, const json &access_policy, const std::string &content, std::string &result);

bool computeDataAnalysis(const std::string &content, std::string &result);

bool computeMachineLearning(const std::string &content, std::string &result);

bool computeModelFinetune(const std::string &dc_id, const std::string &content, std::string &result);

bool computeModelFinetune(const std::string &dc_id, const json &access_policy, const std::string &content, std::string &result);

bool computeOfficeOperation(const std::string &dc_id, const json &access_policy, const std::string &content, const json &data_capsule_info, std::string &result);

// ------------------------- [For Test] -------------------------

void testDataStatisticsString(const std::string &csvData);
