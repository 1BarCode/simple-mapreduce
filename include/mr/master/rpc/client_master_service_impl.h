#pragma once

#include <memory>

#include <grpcpp/grpcpp.h>

#include "mr/master/jobs/job_manager.h"
#include "mr/master/cluster/cluster_manager.h"

// inherit from ClientMasterService::Service
class ClientMasterServiceImpl 
// : public ClientMasterService::Service 
{
public:
    ClientMasterServiceImpl(
        std::shared_ptr<JobManager>,
        std::shared_ptr<ClusterManager>
    );

    grpc::Status SubmitJob(...)     override;
    grpc::Status GetJobStatus(...)  override;
    grpc::Status GetCounters(...)   override;
    // grpc::Status GetClusterStatus(...) override;
    // grpc::Status KillJob(...)       override;

private:
    std::shared_ptr<JobManager>     jobs_;
    std::shared_ptr<ClusterManager> cluster_;
};