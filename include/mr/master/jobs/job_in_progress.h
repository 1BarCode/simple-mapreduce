#pragma once

#include "mr/common/Config.h"
#include "mr/master/jobs/task_in_progress.h"

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <string>
#include <mutex>


struct JobId {};

enum class JobState {

};

class JobInProgress {
public:
    const JobId                                 id;
    const JobConf                               conf;

    float MapProgress() const;
    float ReduceProgress() const;
    bool AllMapsComplete() const;
    bool IsTerminal() const;

private:
    JobState                                    state_;
    std::vector<TaskInProgress>                 map_tasks;
    std::vector<TaskInProgress>                 reduce_tasks;
    std::unordered_map<std::string, int64_t>    counters;
    std::mutex                                  mtx_;
};