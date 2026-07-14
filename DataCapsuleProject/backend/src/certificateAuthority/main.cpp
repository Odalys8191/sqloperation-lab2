#include "../../include/certificateAuthority/server.h"
#include "../../include/common/serviceAddress.h"

void RunCertificateAuthorityServer(const std::string &user_management_address)
{
    std::string server_address(CERTIFICATE_AURTHORITY_ADDRESS);
    CertificateAuthorityServiceImpl service(user_management_address);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "CertificateAuthority listening on " << server_address << std::endl;

    server->Wait();
}

int main(int argc, char **argv)
{
    std::string user_management_address = USER_SERVICE_ADDRESS;
    RunCertificateAuthorityServer(user_management_address);
    return 0;
}