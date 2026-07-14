#include "../../include/consumer/api.h"
#include "../../include/consumer/Verifier.h"
#include "../../include/consumer/consumerServer.h"
#include "../../include/common/signature.openssl.h"
#include "../../include/common/aes256.openssl.h"
#include "../../include/consumer/compute.h"
#include "../../include/common/tools.h"
#include "../../include/common/serviceAddress.h"
#include <pybind11/embed.h>

#include <thread>

namespace py = pybind11;

int main(int argc, char *argv[])
{
#ifdef DEBUG
    // test for openssl signature
    std::cout << "==================== RSA Signature Test ====================" << std::endl;
    testRSASignature();

    // test for openssl aes256
    std::cout << "==================== AES256 Test ====================" << std::endl;
    testAES256Vector();
    testAES256String();

    // test for server interaction
    std::cout << "==================== Server Interaction Test ====================" << std::endl;
    std::string dc_id_for_server_interaction_test = "00000001";
    // testServerInteraction(ca_server_ip, ca_server_port, etcd_server_ip, etcd_server_port, cloud_server_ip, cloud_server_port, dc_id_for_server_interaction_test);
    // test for real ca
    // testCertificateAuthorityInteraction(CERTIFICATE_AURTHORITY_ADDRESS, dc_id_for_server_interaction_test);

    std::string filename = "employee.csv";
    std::string csvData = readFromFile(filename);

    // test for string data statistics
    std::cout << "==================== String Data Statistics Test ====================" << std::endl;
    testDataStatisticsString(csvData);

    // test for computeDataStatics
    std::cout << "==================== Compute Data Statistics Test ====================" << std::endl;
    std::string computeDataStatistics_result{};
    computeDataStatistics(access_policy, csvData, computeDataStatistics_result);
    std::cout << "\nCompute Data Statistics Result:\n"
              << computeDataStatistics_result << std::endl;

    // test for office operation
    std::cout << "==================== Office Operation Test ====================" << std::endl;
    std::string dc_id = "00000001";
    json access_policy;
    access_policy["ComputeType"] = {ComputeType::OFFICE_OPERATION};
    access_policy["OfficeOperation"]["Type"] = OfficeOperationType::DOC;

    std::string file_path = "/home/ftre/Desktop/datacapsule.doc";
    std::string content = readFromFile(file_path);

    std::string office_operation_result;
    if (!computeOfficeOperation(dc_id, access_policy, content, office_operation_result))
    {
        std::cerr << "[main] Failed to execute Office Operation!" << std::endl;
        return -1;
    }

    std::cout << "Office Operation Result:\n"
              << office_operation_result << std::endl;
    std::cout << "==================== Office Operation Test ====================" << std::endl;
#endif

    // // ---------------------------------------------------------------------------------------------------------------------
    // // ------------------------------------------- Consume Data Capsule Test -----------------------------------------------
    // // ---------------------------------------------------------------------------------------------------------------------
    // std::set<ComputeType> compute_types = {ComputeType::DATA_STATISTICS};
    // std::set<DataStatisticsType> data_statistics_types = {DataStatisticsType::COUNT, DataStatisticsType::MIN, DataStatisticsType::MAX, DataStatisticsType::AVG};
    // std::set<std::string> max_min_avg_target_data_columns = {"age", "salary"};
    // std::set<std::string> count_target_data_columns = {"sex", "depid"};
    // json access_policy;
    // access_policy["ComputeType"] = compute_types;
    // access_policy["DataStatistics"]["Type"] = data_statistics_types;
    // access_policy["DataStatistics"]["Count"]["Column"] = count_target_data_columns;
    // access_policy["DataStatistics"]["Min"]["Column"] = max_min_avg_target_data_columns;
    // access_policy["DataStatistics"]["Max"]["Column"] = max_min_avg_target_data_columns;
    // access_policy["DataStatistics"]["Avg"]["Column"] = max_min_avg_target_data_columns;

    // std::string user_id_input = "00000001";
    // std::string dc_id_input = "00000001";
    // std::string password_input = "password123";
    // std::string compute_result;
    // consumeDataCapsule(user_id_input, dc_id_input, password_input, access_policy, compute_result, cloud_server_ip, cloud_server_port, ca_server_ip, ca_server_port, etcd_server_ip, etcd_server_port);
    // std::cout << "Compute Result Test on Company Data: \n"
    //           << compute_result << std::endl;
    // exportToJsonFile(access_policy, "user_request.json");

    // // ---------------------------------------------------------------------------------------------------------------------
    // // ----------------------------------- Consume Data Capsule Test for Ministry of Health --------------------------------
    // // ---------------------------------------------------------------------------------------------------------------------
    // // access policy for ministry of health
    // std::string user_id_ministry_of_health = "00000001";
    // std::string password_ministry_of_health = "password123";
    // std::set<ComputeType> compute_types_hospital_ministry_of_health = {ComputeType::SQL_OPERATION, ComputeType::DATA_STATISTICS};
    // json access_policy_hospital_ministry_of_health;
    // access_policy_hospital_ministry_of_health["ComputeType"] = compute_types_hospital_ministry_of_health;
    // access_policy_hospital_ministry_of_health["SQLOperation"]["Type"] = {SQLOperationType::SELECT};
    // access_policy_hospital_ministry_of_health["SQLOperation"]["Select"]["SelectColumns"] = {"id", "name", "cost"};
    // access_policy_hospital_ministry_of_health["SQLOperation"]["Select"]["FilterColumn"] = "id";
    // access_policy_hospital_ministry_of_health["SQLOperation"]["Select"]["FilterValue"] = "10";
    // access_policy_hospital_ministry_of_health["DataStatistics"]["Type"] = {DataStatisticsType::COUNT, DataStatisticsType::MIN, DataStatisticsType::MAX, DataStatisticsType::AVG};
    // access_policy_hospital_ministry_of_health["DataStatistics"]["Count"]["Column"] = {"diseaseid"};
    // access_policy_hospital_ministry_of_health["DataStatistics"]["Min"]["Column"] = {"cost"};
    // access_policy_hospital_ministry_of_health["DataStatistics"]["Max"]["Column"] = {"cost"};
    // access_policy_hospital_ministry_of_health["DataStatistics"]["Avg"]["Column"] = {"cost"};

    // std::string dc_id_input_hospital = "00000002";
    // std::string compute_result_hospital_ministry_of_health;
    // consumeDataCapsule(user_id_ministry_of_health, dc_id_input_hospital, password_ministry_of_health, access_policy_hospital_ministry_of_health, compute_result_hospital_ministry_of_health, cloud_server_ip, cloud_server_port, ca_server_ip, ca_server_port, etcd_server_ip, etcd_server_port);
    // std::cout << "Compute Result for Ministry of Health on Hospital Data: \n"
    //           << compute_result_hospital_ministry_of_health << std::endl;
    // exportToJsonFile(access_policy_hospital_ministry_of_health, "user_request_hospital_ministry_of_health.json");

    // // ---------------------------------------------------------------------------------------------------------------------
    // // ----------------------------------- Consume Data Capsule Test for Insurance Company ---------------------------------
    // // ---------------------------------------------------------------------------------------------------------------------
    // // access policy for insurance company
    // std::string user_id_insurance_company = "00000002";
    // std::string password_insurance_company = "password456";
    // std::set<ComputeType> compute_types_hospital_insurance_company = {ComputeType::SQL_OPERATION};
    // json access_policy_hospital_insurance_company;
    // access_policy_hospital_insurance_company["ComputeType"] = compute_types_hospital_insurance_company;
    // access_policy_hospital_insurance_company["SQLOperation"]["Type"] = {SQLOperationType::SELECT};
    // access_policy_hospital_insurance_company["SQLOperation"]["Select"]["SelectColumns"] = {"id", "name", "cost"};
    // access_policy_hospital_insurance_company["SQLOperation"]["Select"]["FilterColumn"] = "id";
    // access_policy_hospital_insurance_company["SQLOperation"]["Select"]["FilterValue"] = "10";

    // std::string compute_result_hospital_insurance_company;
    // consumeDataCapsule(user_id_insurance_company, dc_id_input_hospital, password_insurance_company, access_policy_hospital_insurance_company, compute_result_hospital_insurance_company, cloud_server_ip, cloud_server_port, ca_server_ip, ca_server_port, etcd_server_ip, etcd_server_port);
    // std::cout << "Compute Result for Insurance Company on Hospital Data: \n"
    //           << compute_result_hospital_insurance_company << std::endl;
    // exportToJsonFile(access_policy_hospital_insurance_company, "user_request_hospital_insurance_company.json");

    // ---------------------------------------------------------------------------------------------------------------------
    // --------------------------------------- Run Consumer Server for User's Request --------------------------------------
    // ---------------------------------------------------------------------------------------------------------------------

    // testDatabaseOperation("localhost", 33060, "root", "root", "8a759bd10021acd3");
    
    // python编辑器只初始化一次
    py::scoped_interpreter guard{};
    // 释放GIL线程锁
    py::gil_scoped_release release;
    
    std::string server_address = CONSUMER_SERVICE_ADDRESS;
    std::thread thread_consumer_server(runConsumerTEEServer, server_address);
    thread_consumer_server.join();

    return 0;
}

// Bug: when compile with grpc, openssl crash, get error:
// Exception: Error signing data: error:00000000:invalid library (0):OPENSSL_internal:invalid library (0)
// Fix Bug: rebuild grpc with openssl(grpc default use boringssl) -DgRPC_SSL_PROVIDER=package
// cmake -DgRPC_INSTALL=ON   -DgRPC_BUILD_TESTS=OFF   -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR  -DgRPC_SSL_PROVIDER=package ../..

// ====================================================================================================

// Usage1 : argv[0] <dc_id_input> <password_input> <access_policy> <cloud_server_ip> <cloud_server_port> <ca_server_ip> <ca_server_port> <etcd_server_ip> <etcd_server_port>
// Usage2: argv[0] <dc_file> <password_input> <access_policy> <ca_server_ip> <ca_server_port> <etcd_server_ip> <etcd_server_port>

// std::string dc_id_input;
// std::string password_input;
// std::string dc_file;
// std::string access_policy;
// std::string cloud_server_ip;
// std::string cloud_server_port;
// std::string ca_server_ip;
// std::string ca_server_port;
// std::string etcd_server_ip;
// std::string etcd_server_port;

// // 1.user request the CertificateAuthority for dc_id_input
// // 2.user input the dc_id_input and password_input
// if (argc == 10)
// {
//     dc_id_input = argv[1];
//     password_input = argv[2];
//     access_policy = argv[3];
//     cloud_server_ip = argv[4];
//     cloud_server_port = argv[5];
//     ca_server_ip = argv[6];
//     ca_server_port = argv[7];
//     etcd_server_ip = argv[8];
//     etcd_server_port = argv[9];

//     std::cout << "Arguments provided: " << '\n'
//               << "Data Capsule Id: " << dc_id_input << '\n'
//               << "Password: " << password_input << '\n'
//               << "Access Policy: " << access_policy << '\n'
//               << "Cloud Server IP: " << cloud_server_ip << '\n'
//               << "Cloud Server Port: " << cloud_server_port << '\n'
//               << "CertificateAuthority IP: " << ca_server_ip << '\n'
//               << "CertificateAuthority Port: " << ca_server_port << '\n'
//               << "Etcd Server IP: " << etcd_server_ip << '\n'
//               << "Etcd Server Port: " << etcd_server_port << std::endl;
// }
// else if (argc == 8)
// {
//     dc_file = argv[1];
//     password_input = argv[2];
//     access_policy = argv[3];
//     ca_server_ip = argv[4];
//     ca_server_port = argv[5];
//     etcd_server_ip = argv[6];
//     etcd_server_port = argv[7];

//     std::cout << "Arguments provided: " << '\n'
//               << "Data Capsule File: " << dc_file << '\n'
//               << "Password: " << password_input << '\n'
//               << "Access Policy: " << access_policy << '\n'
//               << "CertificateAuthority IP: " << ca_server_ip << '\n'
//               << "CertificateAuthority Port: " << ca_server_port << '\n'
//               << "Etcd Server IP: " << etcd_server_ip << '\n'
//               << "Etcd Server Port: " << etcd_server_port << std::endl;
// }
// else
// {
//     std::cerr << "Usage1: " << argv[0] << " <password_input> <dc_id_input> <access_policy> <cloud_server_ip> <cloud_server_port> <ca_server_ip> <ca_server_port> <etcd_server_ip> <etcd_server_port>" << std::endl;
//     std::cerr << "Usage2: " << argv[0] << " <password_input> <dc_file> <access_policy> <ca_server_ip> <ca_server_port> <etcd_server_ip> <etcd_server_port>" << std::endl;
//     return 1;
// }