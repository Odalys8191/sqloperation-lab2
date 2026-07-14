#include "../../include/etcdServer/server.h"

EtcdServiceImpl::EtcdServiceImpl()
{
    // 初始化一些模拟状态信息
    json state_example;
    state_example["OwnerID"] = "00000001";
    state_example["UsableTimes"] = 100;
    json state_example2;
    state_example2["OwnerID"] = "00000001";
    state_example2["UsableTimes"] = 100;

    // 将模拟数据存储到 state_storage 中
    state_storage["00000001"] = state_example;
    state_storage["00000002"] = state_example2;
}

grpc::Status EtcdServiceImpl::GetDataCapsuleState(grpc::ServerContext *context, const serverInteraction::GetDataCapsuleStateRequest *request,
                                                  serverInteraction::GetDataCapsuleStateResponse *response)
{
    const std::string &dc_id = request->dc_id();
    if (state_storage.find(dc_id) != state_storage.end())
    {
        response->set_state(state_storage[dc_id].dump());
        return grpc::Status::OK;
    }
    return grpc::Status(grpc::NOT_FOUND, "State not found");
}

grpc::Status EtcdServiceImpl::UpdateDataCapsuleState(grpc::ServerContext *context, const serverInteraction::UpdateDataCapsuleStateRequest *request,
                                                     serverInteraction::UpdateDataCapsuleStateResponse *response)
{
    const std::string &dc_id = request->dc_id();
    // 更新状态数据
    if (state_storage.find(dc_id) != state_storage.end())
    {
        json new_state = json::parse(request->state());
        state_storage[dc_id]["OwnerID"] = new_state["OwnerID"];
        state_storage[dc_id]["UsableTimes"] = new_state["UsableTimes"];

        response->set_success(true);
        return grpc::Status::OK;
    }
    response->set_success(false);
    return grpc::Status(grpc::NOT_FOUND, "State not found");
}

grpc::Status EtcdServiceImpl::SendDataCapsuleState(grpc::ServerContext *context, const serverInteraction::SendDataCapsuleStateRequest *request,
                                                   serverInteraction::SendDataCapsuleStateResponse *response)
{
    const std::string &dc_id = request->dc_id();
    json dc_state = json::parse(request->state());
    state_storage[dc_id] = dc_state;
    response->set_result("store state successfully!");
    return grpc::Status::OK;
}
void runEtcdServer(const std::string &server_address)
{
    EtcdServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Etcd Server listening on " << server_address << std::endl;
    server->Wait();
}