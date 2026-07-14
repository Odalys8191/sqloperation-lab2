#include "../../include/cloudServer/server.h"
#include "../../include/common/serviceAddress.h"

int main(int argc, char **argv)
{
    runCloudServer(CLOUD_SERVICE_ADDRESS);
    
    return 0;
}