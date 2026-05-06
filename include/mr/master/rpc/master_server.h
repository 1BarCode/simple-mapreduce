#pragma once

#include <memory>

#include <grpcpp/grpcpp.h>

#include "mr/master/rpc/client_master_service_impl.h"
#include "mr/master/rpc/job_tracker_service_impl.h"


class MasterServer {
public:
    MasterServer(
        std::string address,
        std::shared_ptr<ClientMasterServiceImpl>,
        std::shared_ptr<JobTrackerServiceImpl>
    );

    void Start();
    void Shutdown();

private:
    std::unique_ptr<grpc::Server> grpc_server_;
};