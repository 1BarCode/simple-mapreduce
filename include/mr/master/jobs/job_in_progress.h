#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "mr/common/config.h"
#include "mr/common/ids.h"
#include "mr/master/jobs/task_in_progress.h"

enum class JobState {};

class JobInProgress {
public:
    const JobId   id;
    const JobConf conf;

    float MapProgress() const;
    float ReduceProgress() const;
    bool AllMapsComplete() const;
    bool IsTerminal() const;

private:
    JobState                                    state_;
    std::vector<TaskInProgress>                 map_tasks_;
    std::vector<TaskInProgress>                 reduce_tasks_;
    std::unordered_map<std::string, int64_t>    counters_;
    std::mutex                                  mtx_;
};
