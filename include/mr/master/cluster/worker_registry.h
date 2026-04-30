#pragma once

#include <mutex>
#include <unordered_map>
#include <cstdint>

struct WorkerId;

enum class WorkerState {};

// thread-safe map of live workers
class WorkerRegistry {
public:
    
private:
    std::unordered_map<WorkerId, WorkerState>   workers_;
    std::mutex                                  mtx_;
};