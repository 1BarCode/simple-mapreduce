#pragma once

#include <functional>
#include <memory>
#include <chrono>
#include <atomic>
#include <thread>

#include "mr/master/cluster/worker_registry.h"

// using namespace std::chrono_literals;

class HeartbeatMonitor {
public:
    using DeadCallback = std::function<void(WorkerId)>;

    HeartbeatMonitor(
        std::shared_ptr<WorkerRegistry>, 
        std::chrono::milliseconds timeout,
        DeadCallback on_dead_cb
    );

    void Start();
    void Stop();

private:
    void RunLoop(); // run in background thread

    std::shared_ptr<WorkerRegistry> registry_;
    std::chrono::milliseconds       timeout_;
    DeadCallback                    on_dead_cb_;
    std::thread                     background_thread_;
    std::atomic<bool>               stop_{false};
};