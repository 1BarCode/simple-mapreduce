#pragma once

#include <memory>

#include "mr/master/cluster/worker_provisioner.h"
#include "mr/master/cluster/worker_registry.h"
#include "mr/master/cluster/heartbeat_monitor.h"

// Facade: provision -> register -> monitor
class ClusterManager {
public:
    // Status BootstrapCluster(int num_workers);
    void Shutdown();

private:
    std::shared_ptr<WorkerProvisioner>  provisioner_;
    std::shared_ptr<WorkerRegistry>     registry_;
    std::shared_ptr<HeartbeatMonitor>   hb_monitor_;
};