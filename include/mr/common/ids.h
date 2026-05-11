#pragma once

#include <string>
#include <functional>
#include <cstdint>

struct JobId {
    int64_t epoch   = 0;
    int32_t seq     = 0;

    bool operator==(const JobId& other) const {
        return epoch == other.epoch && seq == other.seq;
    }

    bool operator!=(const JobId& other) const {
        return !(*this == other);
    }

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

    // e.g. "worker-0", "worker-1"
    // or   "localhost:50051"
    const std::string& ToString() const { return value; }
};

struct TaskId {
    JobId job_id;
    bool is_map = true;
    int32_t task_number = 0;

    bool operator==(const TaskId& other) const {
        return (job_id == other.job_id) && 
               (is_map == other.is_map) && 
               (task_number == other.task_number);
    }

    bool operator!=(const TaskId& other) const {
        return !(*this == other);
    }

    // e.g. "1716000000000/42/map/7"
    //      "1716000000000/42/reduce/7"
    std::string ToString() const {
        return  job_id.ToString() + "/" +
                (is_map ? "map" : "reduce") + "/" +
                std::to_string(task_number);
    }
};