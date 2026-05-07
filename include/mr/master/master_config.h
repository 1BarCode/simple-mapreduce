#pragma once

#include <chrono>
#include <string>

struct MasterConfig {
    std::string             listen_address;
    std::string             worker_binary_path;
    std::string             work_dir;
    std::chrono::seconds    heartbeat_interval = std::chrono::seconds(1);
    std::chrono::seconds    worker_timeout = std::chrono::seconds(1);
    int                     num_workers = 4;
    int                     map_slots_per_worker = 2;
    int                     reduce_slots_per_worker = 1;
};