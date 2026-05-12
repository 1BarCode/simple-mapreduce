#pragma once

#include <absl/status/status.h>

#include <string>

// spawns worker processes locally
class WorkerProvisioner {
public:
    absl::Status LaunchWorkers(int n, const std::string& master_addr);

    // blocks until all workers registered (or timeout)
    absl::Status WaitUntilRegistered(absl::Duration deadline);

    void ShutDownWorkers();

private:
    struct WorkerProc {
        pid_t       pid;
        // std::string work_dir;
        int         port;
    };

    std::vector<WorkerProc> procs_;
};