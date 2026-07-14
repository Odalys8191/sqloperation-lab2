#include "../../include/etcdServer/server.h"
#include "../../include/common/serviceAddress.h"

int main(int argc, char **argv)
{
    runEtcdServer(ETCD_SERVICE_ADDRESS);
    
    return 0;
}