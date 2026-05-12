#pragma once

#include <mutex>
#include <unordered_map>
#include <cstdint>
#include <vector>
#include <optional>

#include <absl/status/status.h>

#include "mr/common/types.h"

enum class WorkerState {};

// thread-safe map of live workers
class WorkerRegistry {
public:
    absl::Status Register(WorkerInfo info);
    void UpdateHeartbeat(WorkerId id, int64_t ts_ms);
    void MarkDead(WorkerId id);

    std::vector<WorkerInfo> GetLiveWorkers() const;
    WorkerSlots GetFreeSlots(const WorkerId& id) const;
    std::optional<WorkerInfo> Find(const WorkerId& id) const;

private:
    std::unordered_map<WorkerId, WorkerState>   workers_;
    std::mutex                                  mtx_;
};