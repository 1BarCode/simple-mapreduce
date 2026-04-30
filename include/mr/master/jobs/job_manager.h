#pragma once

#include "mr/master/jobs/job_in_progress.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>

class JobManager {
public:
    JobId CreateJob(SubmitJobRequest req);

    std::shared_ptr<JobInProgress> FindJob(JobId id) const;

    std::vector<JobInProgress> ListActiveJobs() const;

private:
    std::unordered_map<JobId,
        std::shared_ptr<JobInProgress>> jobs_;
        
    std::mutex                          mtx_;
};