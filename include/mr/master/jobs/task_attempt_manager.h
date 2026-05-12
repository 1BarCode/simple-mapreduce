#pragma once

#include <chrono>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include "mr/common/ids.h"

struct AttemptRecord {
    TaskAttemptId                           id;
    WorkerId                                worker;
    std::chrono::steady_clock::time_point   started_at;
};

// Tracks which attempt is currently active for each task and enforces retry limits.
// Lives on the master. All methods are thread-safe.
//
// Responsibilities:
//   - Allocate attempt numbers so every execution has a unique TaskAttemptId.
//   - Validate incoming completion/failure reports (stale attempts are ignored).
//   - Expire all attempts on a dead worker so the scheduler can reassign them.
//
// TaskAttemptManager does NOT mutate TaskInProgress directly. Callers use the
// returned values to drive state transitions on the task.
class TaskAttemptManager {
public:
    explicit TaskAttemptManager(int32_t max_attempts);

    // Allocates the next attempt number for task_id, records it as active,
    // and returns the TaskAttemptId to embed in the LaunchTaskAction sent to worker.
    // Replaces any previously active attempt for the task (e.g. during speculative execution).
    TaskAttemptId NewAttempt(const TaskId& task_id, const WorkerId& worker);

    // Returns true if attempt_id is the current active attempt for its task.
    // False means the report is stale and should be discarded.
    bool IsActiveAttempt(const TaskAttemptId& attempt_id) const;

    // Marks the active attempt complete and removes it from the active set.
    // Returns false if attempt_id is stale.
    bool CompleteAttempt(const TaskAttemptId& attempt_id);

    // Marks the active attempt failed and removes it from the active set.
    // Returns true if the task is eligible for another attempt (failure count < max_attempts).
    // Returns false if stale or if the task has exhausted its retry budget.
    bool FailAttempt(const TaskAttemptId& attempt_id);

    // Called when a worker's heartbeat times out. Removes all active attempts
    // on that worker and returns their TaskIds so the scheduler can reschedule them.
    std::vector<TaskId> ExpireWorker(const WorkerId& worker);

    // Returns the active attempt for a task, or nullopt if none is running.
    std::optional<AttemptRecord> GetActiveAttempt(const TaskId& task_id) const;

    int32_t max_attempts() const { return max_attempts_; }

private:
    int32_t max_attempts_;

    // task_id -> currently active attempt
    std::unordered_map<TaskId, AttemptRecord>   active_attempts_;

    // task_id -> number of failed attempts so far (for retry budget enforcement)
    std::unordered_map<TaskId, int32_t>         failure_counts_;

    mutable std::mutex mtx_;
};
