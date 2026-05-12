#include "mr/master/rpc/job_tracker_service_impl.h"

#include "mr/rpc/proto_converters.h"
#include "mr/rpc/rpc_status.h"

JobTrackerServiceImpl::JobTrackerServiceImpl(
    std::shared_ptr<JobManager>        jobs,
    std::shared_ptr<WorkerRegistry>    registry,
    std::shared_ptr<Scheduler>         scheduler,
    std::shared_ptr<IntermediateIndex> shuffle_index,
    int32_t                            heartbeat_interval_ms)
    : jobs_(std::move(jobs)),
      registry_(std::move(registry)),
      scheduler_(std::move(scheduler)),
      shuffle_index_(std::move(shuffle_index)),
      heartbeat_interval_ms_(heartbeat_interval_ms) {}

grpc::Status JobTrackerServiceImpl::RegisterWorker(
    grpc::ServerContext*             ctx,
    const mr::RegisterWorkerRequest* req,
    mr::RegisterWorkerReply*         reply)
{
    WorkerInfo info = FromProto(req->worker());
    absl::Status status = registry_->Register(info);
    if (!status.ok()) {
        reply->set_accepted(false);
        return ToGrpcStatus(status);
    }
    reply->set_accepted(true);
    reply->set_heartbeat_interval_ms(heartbeat_interval_ms_);
    return grpc::Status::OK;
}

grpc::Status JobTrackerServiceImpl::Heartbeat(
    grpc::ServerContext*        ctx,
    const mr::HeartbeatRequest* req,
    mr::HeartbeatReply*         reply)
{
    WorkerId worker_id = FromProto(req->worker_id());
    registry_->UpdateHeartbeat(worker_id, req->timestamp_ms());

    // TODO(scheduler): query scheduler for new task assignments to launch
    // TODO(job): update progress for each req->running_tasks() entry
    // TODO(job): mark req->just_finished() attempt IDs complete

    reply->set_next_heartbeat_ms(heartbeat_interval_ms_);
    return grpc::Status::OK;
}

grpc::Status JobTrackerServiceImpl::ReportTaskCompletion(
    grpc::ServerContext*                   ctx,
    const mr::ReportTaskCompletionRequest* req,
    mr::ReportTaskCompletionReply*         reply)
{
    TaskAttemptId attempt_id = FromProto(req->attempt_id());

    auto job = jobs_->FindJob(attempt_id.task_id.job_id);
    if (!job) {
        reply->set_accepted(false);
        return grpc::Status::OK;
    }

    // TODO(job): validate attempt is still active (stale-attempt check via
    //            TaskAttemptManager), then call task.MarkSucceeded()
    // TODO(shuffle): if req->type() == MAP, record each output location via
    //                shuffle_index_->Add(FromProto(out))

    reply->set_accepted(true);
    return grpc::Status::OK;
}

grpc::Status JobTrackerServiceImpl::ReportTaskFailure(
    grpc::ServerContext*               ctx,
    const mr::ReportTaskFailureRequest* req,
    mr::ReportTaskFailureReply*         reply)
{
    TaskAttemptId attempt_id = FromProto(req->attempt_id());

    auto job = jobs_->FindJob(attempt_id.task_id.job_id);
    if (!job) {
        reply->set_accepted(false);
        return grpc::Status::OK;
    }

    // TODO(job): validate attempt is still active, then call task.MarkFailed()
    //            and reschedule if retry budget allows

    reply->set_accepted(true);
    return grpc::Status::OK;
}
