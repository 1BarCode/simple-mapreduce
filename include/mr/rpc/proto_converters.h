#pragma once

#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "client/job_client.pb.h"
#include "common/ids.pb.h"
#include "common/types.pb.h"
#include "mr/common/config.h"
#include "mr/common/ids.h"
#include "mr/common/types.h"

// ---- FromProto: proto -> domain ----

inline JobId FromProto(const mr::JobId& p) {
    return JobId{p.epoch(), p.seq()};
}

inline WorkerId FromProto(const mr::WorkerId& p) {
    return WorkerId{p.value()};
}

inline TaskId FromProto(const mr::TaskId& p) {
    return TaskId{FromProto(p.job_id()), p.is_map(), p.task_number()};
}

inline TaskAttemptId FromProto(const mr::TaskAttemptId& p) {
    return TaskAttemptId{FromProto(p.task_id()), p.attempt_number()};
}

inline InputSplit FromProto(const mr::InputSplit& p) {
    return InputSplit{p.input_path(), p.offset(), p.length(), p.split_number()};
}

inline MapOutputLocation FromProto(const mr::MapOutputLocation& p) {
    return MapOutputLocation{
        FromProto(p.worker_id()),
        p.worker_address(),
        FromProto(p.map_attempt_id()),
        p.reduce_partition(),
        p.file_path(),
        p.size_bytes(),
    };
}

inline WorkerInfo FromProto(const mr::WorkerInfo& p) {
    return WorkerInfo{
        FromProto(p.worker_id()),
        p.address(),
        p.map_slots(),
        p.reduce_slots(),
        p.work_dir(),
    };
}

inline absl::StatusOr<JobConf> FromProto(const mr::SubmitJobRequest& p) {
    if (p.num_reduce_tasks() < 0) {
        return absl::InvalidArgumentError(
            absl::StrCat("num_reduce_tasks must be >= 0, got ", p.num_reduce_tasks()));
    }
    JobConf c;
    c.job_name           = p.job_name();
    c.input_path         = p.input_path();
    c.output_path        = p.output_path();
    c.mapper_name        = p.mapper_name();
    c.reducer_name       = p.reducer_name();
    c.combiner_name      = p.combiner_name();
    c.num_reduce_tasks   = p.num_reduce_tasks();
    for (const auto& [k, v] : p.properties()) {
        c.properties[k] = v;
    }
    return c;
}

// ---- ToProto: domain -> proto ----

inline mr::JobId ToProto(const JobId& id) {
    mr::JobId p;
    p.set_epoch(id.epoch);
    p.set_seq(id.seq);
    return p;
}

inline mr::WorkerId ToProto(const WorkerId& id) {
    mr::WorkerId p;
    p.set_value(id.value);
    return p;
}

inline mr::TaskId ToProto(const TaskId& id) {
    mr::TaskId p;
    *p.mutable_job_id() = ToProto(id.job_id);
    p.set_is_map(id.is_map);
    p.set_task_number(id.task_number);
    return p;
}

inline mr::TaskAttemptId ToProto(const TaskAttemptId& id) {
    mr::TaskAttemptId p;
    *p.mutable_task_id() = ToProto(id.task_id);
    p.set_attempt_number(id.attempt_number);
    return p;
}

inline mr::MapOutputLocation ToProto(const MapOutputLocation& loc) {
    mr::MapOutputLocation p;
    *p.mutable_worker_id()      = ToProto(loc.worker_id);
    p.set_worker_address(loc.worker_address);
    *p.mutable_map_attempt_id() = ToProto(loc.map_attempt_id);
    p.set_reduce_partition(loc.reduce_partition);
    p.set_file_path(loc.file_path);
    p.set_size_bytes(loc.size_bytes);
    return p;
}
