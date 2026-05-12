#pragma once

#include <cstdint>
#include <functional>
#include <string>

struct JobId {
    int64_t epoch = 0;
    int32_t seq   = 0;

    bool operator==(const JobId& other) const {
        return epoch == other.epoch && seq == other.seq;
    }
    bool operator!=(const JobId& other) const { return !(*this == other); }
    bool operator<(const JobId& other) const {
        if (epoch != other.epoch) return epoch < other.epoch;
        return seq < other.seq;
    }

    // e.g. "1716000000000/42"
    std::string ToString() const {
        return std::to_string(epoch) + "/" + std::to_string(seq);
    }
};

struct WorkerId {
    std::string value;

    bool operator==(const WorkerId& other) const { return value == other.value; }
    bool operator!=(const WorkerId& other) const { return !(*this == other); }
    bool operator<(const WorkerId& other) const { return value < other.value; }

    bool empty() const { return value.empty(); }

    // e.g. "worker-0", "worker-1" or "localhost:50051"
    const std::string& ToString() const { return value; }
};

// task_number is the input split index for map tasks,
// and the reduce partition index for reduce tasks.
struct TaskId {
    JobId   job_id;
    bool    is_map      = true;
    int32_t task_number = 0;

    bool operator==(const TaskId& other) const {
        return job_id == other.job_id &&
               is_map == other.is_map &&
               task_number == other.task_number;
    }
    bool operator!=(const TaskId& other) const { return !(*this == other); }
    bool operator<(const TaskId& other) const {
        if (job_id != other.job_id) return job_id < other.job_id;
        if (is_map != other.is_map) return is_map > other.is_map; // map sorts before reduce
        return task_number < other.task_number;
    }

    // e.g. "1716000000000/42/map/7"
    //      "1716000000000/42/reduce/3"
    std::string ToString() const {
        return job_id.ToString() + "/" +
               (is_map ? "map" : "reduce") + "/" +
               std::to_string(task_number);
    }
};

struct TaskAttemptId {
    TaskId  task_id;
    int32_t attempt_number = 0;

    bool operator==(const TaskAttemptId& other) const {
        return task_id == other.task_id && attempt_number == other.attempt_number;
    }
    bool operator!=(const TaskAttemptId& other) const { return !(*this == other); }
    bool operator<(const TaskAttemptId& other) const {
        if (task_id != other.task_id) return task_id < other.task_id;
        return attempt_number < other.attempt_number;
    }

    // e.g. "1716000000000/42/map/7/attempt/1"
    std::string ToString() const {
        return task_id.ToString() + "/attempt/" + std::to_string(attempt_number);
    }
};

// ---------------------------------------------------------------------------
// std::hash specializations so all ID types can be used as unordered_map keys
// ---------------------------------------------------------------------------
namespace std {

template <> struct hash<JobId> {
    size_t operator()(const JobId& id) const noexcept {
        size_t h = hash<int64_t>{}(id.epoch);
        h ^= hash<int32_t>{}(id.seq) + 0x9e3779b9u + (h << 6) + (h >> 2);
        return h;
    }
};

template <> struct hash<WorkerId> {
    size_t operator()(const WorkerId& id) const noexcept {
        return hash<string>{}(id.value);
    }
};

template <> struct hash<TaskId> {
    size_t operator()(const TaskId& id) const noexcept {
        size_t h = hash<JobId>{}(id.job_id);
        h ^= hash<bool>{}(id.is_map)            + 0x9e3779b9u + (h << 6) + (h >> 2);
        h ^= hash<int32_t>{}(id.task_number)    + 0x9e3779b9u + (h << 6) + (h >> 2);
        return h;
    }
};

template <> struct hash<TaskAttemptId> {
    size_t operator()(const TaskAttemptId& id) const noexcept {
        size_t h = hash<TaskId>{}(id.task_id);
        h ^= hash<int32_t>{}(id.attempt_number) + 0x9e3779b9u + (h << 6) + (h >> 2);
        return h;
    }
};

} // namespace std
