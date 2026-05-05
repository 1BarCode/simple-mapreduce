#pragma once

#include <memory>

#include <grpcpp/grpcpp.h>

#include "mr/master/jobs/job_manager.h"
#include "mr/master/cluster/cluster_manager.h"
#include "mr/master/scheduler/scheduler.h"
#include "mr/master/shuffle/intermediate_index.h"

// inherit from JobTrackerService::Service
class JobTrackerServiceImpl 
// : JobTrackerService::Service
{
public:

    grpc::Status RegisterWorker(...)        override;
    grpc::Status Heartbeat(...)              override;
    grpc::Status ReportTaskCompletion(...)   override;
    grpc::Status ReportTaskFailure(...)      override;

private:
    std::shared_ptr<JobManager>         jobs_;
    std::shared_ptr<ClusterManager>     cluster_;
    std::shared_ptr<Scheduler>          scheduler_;
    std::shared_ptr<IntermediateIndex>  shuffle_index_;
};