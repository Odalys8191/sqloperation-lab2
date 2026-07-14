#include "../../include/userServer/server.h"
#include "../../include/common/serviceAddress.h"
#include <grpcpp/grpcpp.h>
#include <iostream>

void RunUserManagementServer()
{
    std::string server_address(USER_SERVICE_ADDRESS);
    UserManagementServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "User Management Server listening on " << server_address << std::endl;

    server->Wait();
}

int main(int argc, char **argv)
{
    RunUserManagementServer();
    return 0;
}
