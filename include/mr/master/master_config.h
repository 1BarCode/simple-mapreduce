#pragma once

#include <string>
// #include "absl/time/time.h"
#include <chrono>

using namespace std::chrono_literals;

struct MasterConfig {
    std::string             listen_address;
    std::string             worker_binary_path;
    std::string             work_dir;
    // absl::Duration  heartbeat_interval = absl::Seconds(3);
    // absl::Duration  worker_timeout = absl::Seconds(3);
    std::chrono::seconds    heartbeat_interval = 3s;
    std::chrono::seconds    worker_timeout = 3s;
    int                     num_workers = 4;
    int                     map_slots_per_worker = 2;
    int                     reduce_slots_per_worker = 1;
};