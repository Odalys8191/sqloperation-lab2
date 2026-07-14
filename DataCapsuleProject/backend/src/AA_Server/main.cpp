#include "../../include/AA_Server/AAServiceImpl.h"
#include "../../include/common/serviceAddress.h"
void RunServer(const std::string& server_address) {
    AAServiceImpl service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "AAService listening on " << server_address << std::endl;
    server->Wait();
}

int main() {
    std::string server_address(AA_SERVICE_ADDRESS);
    RunServer(server_address);
    return 0;
}