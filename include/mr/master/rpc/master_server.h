#pragma once

#include <memory>

#include <grpcpp/grpcpp.h>

#include "mr/master/rpc/client_master_service_impl.h"
#include "mr/master/rpc/job_tracker_service_impl.h"


class MasterServer {
public:
    MasterServer(
        std::string address,
        std::shared_ptr<ClientMasterServiceImpl> client_service,
        std::shared_ptr<JobTrackerServiceImpl> job_tracker_service
    );

    void Start();
    void Shutdown();

private:
    std::shared_ptr<ClientMasterServiceImpl>    client_service_;
    std::shared_ptr<JobTrackerServiceImpl>      job_tracker_service_;
    std::unique_ptr<grpc::Server>               grpc_server_;
};