#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "mr/common/ids.h"
#include "mr/common/types.h"

enum class TaskState {
    kUnassigned,  // waiting to be scheduled
    kAssigned,    // scheduler picked a worker, assignment not yet acknowledged
    kRunning,     // worker confirmed it started
    kSucceeded,
    kFailed,
    kKilled,
};

// Represents one logical task (one input split or one reduce partition) within a job.
// Owned by JobInProgress. All mutation is driven by the master's RPC handlers via
// TaskAttemptManager; JobInProgress's mutex serializes all access.
class TaskInProgress {
public:
    // Map task: responsible for processing the given input split.
    TaskInProgress(TaskId id, InputSplit split);

    // Reduce task: responsible for collecting and reducing partition `reduce_partition`.
    TaskInProgress(TaskId id, int32_t reduce_partition);

    // --- Accessors ---

    const TaskId&   id()              const { return id_; }
    TaskState       state()           const { return state_; }
    int32_t         attempt_number()  const { return attempt_number_; }
    const WorkerId& assigned_worker() const { return assigned_worker_; }
    float           progress()        const { return progress_; }

    bool IsComplete()  const { return state_ == TaskState::kSucceeded; }
    bool IsPending()   const { return state_ == TaskState::kUnassigned; }
    bool IsRunning()   const {
        return state_ == TaskState::kAssigned || state_ == TaskState::kRunning;
    }
    bool CanRetry(int32_t max_attempts) const {
        return attempt_number_ < max_attempts && !IsComplete();
    }

    // Map task input spec.
    const InputSplit& split() const { return split_; }

    // Reduce task partition index.
    int32_t reduce_partition() const { return reduce_partition_; }

    // Set on map task success; empty until then.
    const std::vector<MapOutputLocation>& map_outputs() const { return map_outputs_; }

    // Set on reduce task success; empty until then.
    const std::string& reduce_output_file() const { return reduce_output_file_; }

    // --- State transitions (called by the master's job-tracking layer) ---

    // Scheduler assigned this task to a worker. attempt_number must equal
    // the value in the TaskAttemptId sent to the worker.
    void Assign(WorkerId worker, int32_t attempt_number);

    // Worker heartbeat confirmed the task is executing.
    void UpdateProgress(float progress);

    // Worker reported success (map task).
    void MarkSucceeded(std::vector<MapOutputLocation> outputs);

    // Worker reported success (reduce task).
    void MarkSucceeded(std::string output_file);

    // Worker reported failure or heartbeat timed out. Does not reset attempt_number;
    // the caller increments it via the next Assign() call.
    void MarkFailed();

    void MarkKilled();

private:
    TaskId      id_;
    TaskState   state_          = TaskState::kUnassigned;
    int32_t     attempt_number_ = 0;
    WorkerId    assigned_worker_;
    float       progress_       = 0.0f;

    // Map task fields
    InputSplit                      split_;
    std::vector<MapOutputLocation>  map_outputs_;

    // Reduce task fields
    int32_t     reduce_partition_    = 0;
    std::string reduce_output_file_;
};
