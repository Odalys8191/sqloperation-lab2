#include "../../include/consumer/consumerServer.h"

grpc::Status ConsumerTEEServiceImpl::ConsumeDataCapsule(grpc::ServerContext *context,
                                                        const serverInteraction::ConsumeDataCapsuleRequest *request,
                                                        serverInteraction::ConsumeDataCapsuleResponse *response)
{
    std::cout << "\n-------------------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "---------------------- [ConsumerTEEServiceImpl] Consume Data Capsule Request Recieved! -------------- -------" << std::endl;
    std::cout << "-------------------------------------------------------------------------------------------------------------\n";
    user_id_ = request->user_id();
    dc_id_ = request->dc_id();
    // password_ = request->password();
    password_ = "test_password"; // For testing, replace with actual password retrieval logic
    access_policy_ = json::parse(request->user_request());
    std::cout << "[ConsumerTEEServiceImpl] User ID: " << user_id_ << std::endl;
    std::cout << "[ConsumerTEEServiceImpl] DC ID: " << dc_id_ << std::endl;
    std::cout << "[ConsumerTEEServiceImpl] Password: " << password_ << std::endl;
    std::cout << "[ConsumerTEEServiceImpl] Access Policy: " << access_policy_.dump() << std::endl;

    std::string compute_result = "";
    std::string error_message;
    if (!consumeDataCapsule(user_id_, dc_id_, password_, access_policy_, compute_result, CLOUD_SERVICE_IP, CLOUD_SERVICE_PORT, CERTIFICATE_AURTHORITY_IP, CERTIFICATE_AURTHORITY_PORT, ETCD_SERVICE_IP, ETCD_SERVICE_PORT, &error_message))
    {
        std::cout << "-------------------------------------------------------------------------------------------------------------" << std::endl;
        std::cout << "----------------------- [ConsumerTEEServiceImpl] Consume Data Capsule Request Failed! -----------------------" << std::endl;
        std::cout << "-------------------------------------------------------------------------------------------------------------\n"
                  << std::endl;
        response->set_compute_result("");
        if (error_message.empty())
        {
            error_message = "Unprivileged Access to Data Capsule!";
        }
        if (error_message.find("ABE attributes insufficient") != std::string::npos)
        {
            return grpc::Status(grpc::StatusCode::PERMISSION_DENIED, error_message);
        }
        return grpc::Status(grpc::StatusCode::CANCELLED, error_message);
    }
    std::cout << "-------------------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "---------------------- [ConsumerTEEServiceImpl] Consume Data Capsule Request Succeess! ----------------------" << std::endl;
    std::cout << "-------------------------------------------------------------------------------------------------------------\n"
              << std::endl;
    response->set_compute_result(compute_result);
    return grpc::Status::OK;
}

void runConsumerTEEServer(std::string server_address)
{
    ConsumerTEEServiceImpl service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Consumer Server listening on " << server_address << std::endl;
    server->Wait();
}