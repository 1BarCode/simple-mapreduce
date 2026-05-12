#pragma once

#include <memory>

#include <grpcpp/grpcpp.h>

#include "master/job_tracker.grpc.pb.h"
#include "mr/master/jobs/job_manager.h"
#include "mr/master/cluster/worker_registry.h"
#include "mr/master/scheduler/scheduler.h"
#include "mr/master/shuffle/intermediate_index.h"

class JobTrackerServiceImpl : public mr::JobTrackerService::Service {
public:
    JobTrackerServiceImpl(
        std::shared_ptr<JobManager>        jobs,
        std::shared_ptr<WorkerRegistry>    registry,
        std::shared_ptr<Scheduler>         scheduler,
        std::shared_ptr<IntermediateIndex> shuffle_index,
        int32_t                            heartbeat_interval_ms
    );

    grpc::Status RegisterWorker(
        grpc::ServerContext*              ctx,
        const mr::RegisterWorkerRequest*  req,
        mr::RegisterWorkerReply*          reply) override;

    grpc::Status Heartbeat(
        grpc::ServerContext*        ctx,
        const mr::HeartbeatRequest* req,
        mr::HeartbeatReply*         reply) override;

    grpc::Status ReportTaskCompletion(
        grpc::ServerContext*                    ctx,
        const mr::ReportTaskCompletionRequest*  req,
        mr::ReportTaskCompletionReply*          reply) override;

    grpc::Status ReportTaskFailure(
        grpc::ServerContext*                ctx,
        const mr::ReportTaskFailureRequest* req,
        mr::ReportTaskFailureReply*         reply) override;

private:
    std::shared_ptr<JobManager>         jobs_;
    std::shared_ptr<WorkerRegistry>     registry_;
    std::shared_ptr<Scheduler>          scheduler_;
    std::shared_ptr<IntermediateIndex>  shuffle_index_;
    int32_t                             heartbeat_interval_ms_;
};
