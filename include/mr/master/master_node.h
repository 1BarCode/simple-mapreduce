#pragma once

#include <memory>

#include "absl/status/status.h"

#include "mr/common/clock.h"
#include "mr/common/logger.h"
#include "mr/master/master_config.h"
#include "mr/master/rpc/master_server.h"
#include "mr/master/cluster/cluster_manager.h"
#include "mr/master/jobs/job_manager.h"
#include "mr/master/scheduler/scheduler.h"
#include "mr/master/shuffle/intermediate_index.h"
#include "mr/runtime/threadpool.h"


class MasterNode {
public:
    explicit MasterNode(MasterConfig cfg);
    ~MasterNode();

    // Start RPC server, provision workers, enter run loop
    absl::Status Start();
    void Shutdown();

private:
    MasterConfig                        config_;

    // rpc services
    std::unique_ptr<MasterServer>       master_server_;

    // cluster
    std::shared_ptr<ClusterManager>     cluster_;
    
    // job tracking + scheduling
    std::shared_ptr<JobManager>         jobs_;

    // based on Hadoop MRv1 model: tasks are subdivision of a job
    // a job with 100 input splits has 100 map tasks + N reduce tasks
    // client submit "jobs"
    // master create "jobs" and split them into "tasks" and distributed to workers
    // workers work on "tasks", not "jobs"
    std::shared_ptr<Scheduler>          scheduler_;

    // shuffle metadata - map output locations
    std::shared_ptr<IntermediateIndex>  shuffle_index_;

    // Support
    std::shared_ptr<Clock>              clock_;
    std::shared_ptr<Logger>             logger_;
    ThreadPool                          workers_;

};