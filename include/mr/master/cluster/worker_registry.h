#pragma once

#include <mutex>
#include <unordered_map>
#include <cstdint>
#include <vector>
#include <optional>

struct WorkerId;
struct WorkerInfo;
struct WorkerSlots;

enum class WorkerState {};

// thread-safe map of live workers
class WorkerRegistry {
public:
    // Status Register(WorkerInfo info);
    void UpdateHeartbeat(WorkerId id, int64_t ts_ms);
    void MarkDead(WorkerId id);

    std::vector<WorkerInfo> GetLiveWorkers() const;
    WorkerSlots GetFreeSlots(WorkerId) const;
    std::optional<WorkerInfo> Find(WorkerId) const;

private:
    std::unordered_map<WorkerId, WorkerState>   workers_;
    std::mutex                                  mtx_;
};