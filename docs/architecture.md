# Architecture

## Overview

This project is a local, multi-process MapReduce framework implemented in C++ with gRPC-based IPC. It is intentionally modeled after Hadoop MRv1, using a `JobClient -> JobTracker -> TaskTracker` style split, but adapted for local execution where the master process also provisions worker processes on startup [web:111][web:13].

The system has three runtime roles:

- **Client**: submits jobs, polls status, and optionally blocks in `waitForCompletion(true)`.
- **Master**: provisions workers, accepts jobs, splits input, schedules map/reduce task attempts, tracks worker liveness, and decides job completion.
- **Worker**: executes map/reduce task attempts, stores local intermediate files, heartbeats to the master, and serves shuffle data to reducers.

Reducers fetch map outputs using gRPC after the master tells them where each partition is located. This follows the MapReduce design where the master stores intermediate file locations for completed map tasks and informs reducers accordingly [web:13][web:7].

## Design goals

The design aims to:

- Closely resemble Hadoop MRv1 concepts and APIs.
- Preserve the MapReduce execution model: map -> shuffle/sort -> reduce.
- Run entirely on one machine using multiple OS processes.
- Use gRPC for all control-plane IPC and for shuffle-data transfer.
- Make failure and retry behavior explicit and observable.
- Keep the public API simple while separating internal runtime modules.

## Runtime roles

### Client

The client is analogous to Hadoop’s `JobClient`. It is responsible for:

- constructing `JobConf`,
- submitting jobs to the master,
- querying job status and counters,
- optionally calling `waitForCompletion(verbose)`.

The client does not directly communicate with workers.

### Master

The master is analogous to Hadoop’s `JobTracker`, with one additional responsibility: **local cluster bootstrap**. At startup it:

1. reads cluster configuration,
2. launches worker processes,
3. waits for worker registration,
4. marks the cluster ready,
5. begins accepting client jobs.

During execution, the master:

- validates submitted jobs,
- plans input splits,
- creates map/reduce task metadata,
- tracks worker registration and heartbeat state,
- assigns tasks to workers through heartbeat replies,
- records map-output partition locations,
- transitions jobs from map phase to reduce phase,
- retries failed task attempts,
- marks jobs as succeeded or failed.

### Worker

The worker is analogous to Hadoop’s `TaskTracker`. Each worker process:

- starts its own gRPC server,
- registers with the master,
- periodically sends heartbeats,
- executes assigned map/reduce tasks,
- writes intermediate and final output files,
- serves shuffle partitions to reducers.

Workers are generic executors; they are not permanently designated as map-only or reduce-only nodes.

## High-level flow

The end-to-end flow is:

1. Master starts.
2. Master provisions `N` worker processes locally.
3. Workers register with the master.
4. Client submits a job.
5. Master computes input splits and creates map tasks.
6. Workers send heartbeats with free slot information.
7. Master replies to heartbeats with map task launch actions.
8. Map tasks run and write one intermediate partition file per reducer.
9. Workers report map completion and partition metadata.
10. Master records partition locations.
11. When all maps are complete, master begins assigning reduce tasks.
12. Reduce workers fetch their partition from every completed map task via gRPC.
13. Reducers group/sort by key, run the reducer, and write final outputs.
14. Workers report reduce completion.
15. Master marks the job `SUCCEEDED` when all reduce tasks complete successfully.

The job is complete only after the reduce phase is complete, not when maps finish [web:13][web:7].

## Hadoop-style architecture mapping

| This project                 | Hadoop MRv1 analog   | Responsibility                                 |
| ---------------------------- | -------------------- | ---------------------------------------------- |
| `JobClient`                  | `JobClient`          | Job submission and status tracking             |
| `MasterNode` / `JobTracker`  | `JobTracker`         | Scheduling, monitoring, recovery               |
| `WorkerNode` / `TaskTracker` | `TaskTracker`        | Task execution and heartbeats                  |
| `JobConf`                    | `JobConf`            | User job configuration                         |
| `RunningJob`                 | `RunningJob` / `Job` | Client-side handle for progress and completion |

The most important adaptation is that our master also includes `WorkerProvisioner`, because workers are not pre-deployed daemons in a real cluster; they are launched locally by the master.

## Module boundaries

### Public API

The public API is the stable user-facing layer:

- `JobConf`
- `JobClient`
- `RunningJob`
- `Mapper`
- `Reducer`
- `Combiner`
- `Partitioner`
- `InputFormat`
- `OutputFormat`

This is the only layer job authors should typically depend on.

### Master runtime

The master runtime owns cluster control and scheduling:

- `MasterNode`
- `JobTrackerServiceImpl`
- `JobClientServiceImpl`
- `JobTracker`
- `JobInProgress`
- `TaskInProgress`
- `TaskScheduler`
- `WorkerRegistry`
- `WorkerProvisioner`
- `HeartbeatMonitor`
- `IntermediateIndex`
- `InputSplitPlanner`
- `TaskAttemptManager`

### Worker runtime

The worker runtime owns task execution and shuffle serving:

- `WorkerNode`
- `TaskTrackerServiceImpl`
- `ShuffleDataServiceImpl`
- `MasterClient`
- `HeartbeatLoop`
- `TaskRunner`
- `MapTaskRunner`
- `ReduceTaskRunner`
- `LocalDirManager`
- `IntermediateFileWriter`
- `IntermediateFileReader`
- `ShuffleFetcher`

### Shared infrastructure

Shared utilities are grouped into:

- `common/` for ids, config, logging, and errors,
- `runtime/` for threads, processes, retries, temp files,
- `rpc/` for protocol conversion and deadlines,
- `storage/` for atomic writes and file abstractions.

## Job lifecycle

A job moves through the following states:

- `PREP`
- `RUNNING_MAP`
- `RUNNING_REDUCE`
- `SUCCEEDED`
- `FAILED`
- `KILLED`

The master exposes this state to clients through `GetJobStatus`. Clients may either poll for status or call `waitForCompletion(verbose)`, which internally polls and prints progress updates rather than silently blocking.

## Task lifecycle

Each logical task may have one or more attempts. A task attempt moves through these states:

- `UNASSIGNED`
- `ASSIGNED`
- `RUNNING`
- `SUCCEEDED`
- `FAILED`
- `KILLED`

A failed worker may cause all of its running task attempts to be retried. If a worker dies after completing map tasks, the map outputs on its local disk are treated as lost and those map tasks are eligible for re-execution, because the master can no longer direct reducers to valid intermediate data [web:13][web:7].

## Slots and scheduling

Workers advertise capacity using map and reduce slots:

- `mapSlots`
- `reduceSlots`

On each heartbeat, a worker reports its free slots and currently running attempts. The master chooses assignments based on:

- job phase,
- worker free slots,
- unassigned tasks,
- retry priority,
- optional data-locality hints for local input files.

The scheduler is heartbeat-driven to mimic Hadoop MRv1. Rather than a worker separately requesting work, the master piggybacks launch decisions on heartbeat responses [web:111][web:107].

## Input splitting

The master creates one logical map task per input split. Input splitting is performed before any reduce scheduling begins. Built-in formats such as `TextInputFormat` may split by file chunk or by line-oriented regions depending on implementation.

The split planner must generate stable split ids so retries refer to the same logical map task.

## Intermediate data model

Each successful map task writes `R` partition files, one per reducer. For every completed map task, the master stores:

- producing worker id,
- worker address,
- map task id,
- reduce partition id,
- file path,
- byte size.

Reducers cannot start until all map tasks have completed successfully and the master has complete partition-location metadata for all reducers [web:13].

## Shuffle and reduce

For reduce task `r`, the reduce worker fetches partition `r` from every completed map task using `ShuffleDataService`. It then:

1. streams or downloads partition data,
2. merges records locally,
3. groups values by key,
4. sorts keys if deterministic ordering is required,
5. invokes the reducer,
6. writes final output through `OutputCommitter`.

This design is intentionally close to MapReduce’s worker-local intermediate storage model [web:13][web:7].

## Failure handling

### Worker failure

A worker is considered failed if it misses heartbeat deadlines or its RPC endpoint becomes unreachable. When the master declares a worker dead:

- its running map/reduce attempts are marked failed,
- completed map outputs hosted on that worker are invalidated,
- affected map tasks are made runnable again,
- stale completion reports from older attempts are ignored.

### Task failure

A task attempt may fail because of:

- worker crash,
- mapper/reducer exception,
- file I/O failure,
- gRPC shuffle transfer failure,
- deadline exceeded.

The master increments the attempt count and may reschedule the task unless the job exceeds retry policy thresholds.

## Output commit

Reducers write to temporary output files and atomically rename them on success. This prevents partial results from being visible if a reducer crashes mid-write. Atomic commit mirrors the broader MapReduce idea that final task outputs should become visible only after successful completion [web:7].

## Local cluster provisioning

A major customization in this project is local worker provisioning. `WorkerProvisioner` is responsible for:

- choosing ports,
- creating worker work directories,
- spawning worker processes,
- capturing stdout/stderr logs,
- waiting for registration,
- shutting workers down on master exit.

This lets a developer run the whole system locally with a single master command while preserving a distributed-process architecture.

## Concurrency model

### Master

The master uses:

- gRPC server threads for RPC entry points,
- scheduler/heartbeat-monitor threads,
- synchronized job and worker registries.

RPC handlers should validate requests and mutate state quickly, not perform long-running work inline.

### Worker

Workers use:

- gRPC server threads,
- a heartbeat loop thread,
- task execution threads,
- local file I/O paths for shuffle and output.

Unary RPCs are preferred for control-plane operations, while shuffle transfer uses server-streaming because data payloads may be large and naturally chunked [web:27][web:112].

## Extensibility

The framework is designed to support:

- additional input/output formats,
- alternative partitioners,
- optional combiners,
- different scheduling policies,
- speculative execution,
- persistent job history,
- remote cluster deployment in the future.

The local-only provisioning layer is isolated so it can be replaced later by external worker deployment if desired.

## Non-goals

The initial design does not attempt to provide:

- HDFS-compatible storage,
- rack-aware scheduling,
- YARN-style resource management,
- distributed filesystem replication,
- remote code shipping,
- full security / authn / authz.

The goal is a clear, Hadoop-like MapReduce runtime, not a full Hadoop ecosystem clone.

## Summary of key decisions

- Hadoop MRv1-inspired client/master/worker structure.
- gRPC for both control-plane and shuffle IPC.
- Master-provisioned local worker processes.
- Heartbeat-driven task assignment.
- Worker-local intermediate files.
- Reducer fetches partitions from map workers.
- Client completion means **reduce phase complete**, not merely map phase complete.
