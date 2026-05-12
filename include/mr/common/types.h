#pragma once

#include <cstdint>
#include <string>

#include "mr/common/ids.h"

struct InputSplit {
    std::string input_path;
    uint64_t    offset       = 0;
    uint64_t    length       = 0;
    int32_t     split_number = 0;
};

// Location of one partition file produced by a completed map task.
// Reduce task N fetches every MapOutputLocation where reduce_partition == N.
struct MapOutputLocation {
    WorkerId      worker_id;
    std::string   worker_address;
    TaskAttemptId map_attempt_id;
    int32_t       reduce_partition = 0;
    std::string   file_path;
    uint64_t      size_bytes       = 0;
};

struct WorkerSlots {
    int32_t free_map_slots    = 0;
    int32_t free_reduce_slots = 0;
};

struct WorkerInfo {
    WorkerId    worker_id;
    std::string address;
    int32_t     map_slots    = 0;
    int32_t     reduce_slots = 0;
    std::string work_dir;
};
