# Concurrency Model

## Overview

This document summarizes the concurrency model for the local MapReduce system. The system uses three process roles: client, master, and worker. The client is lightweight and mostly synchronous, the master is a concurrent control plane, and each worker is a bounded task executor with its own control and data-plane activity [1][2].

The design follows a Hadoop-like execution style in which the master coordinates jobs and workers, while workers execute tasks in parallel and report status through periodic communication. gRPC C++ guidance also recommends treating callbacks as potentially concurrent and keeping handlers lightweight, which shapes the internal threading model of the master and worker processes [1][2][3].

## Client

The client has the simplest concurrency model. In the common case, it can run as a mostly single-threaded CLI process that builds a job configuration, submits it to the master, and then either exits or waits for completion by repeatedly querying job status [1].

A practical client design uses one main thread for argument parsing, job submission, and output, plus an optional polling loop or background thread for `waitForCompletion(true)`. The client should not perform task execution, scheduling, or heavy parallel work locally because those responsibilities belong to the master and workers [1][3].

### Client responsibilities by thread

- Main thread: parse CLI input, build `JobConf`, submit job, print progress, return exit status [1].
- Optional polling thread: periodically call `GetJobStatus` and update progress display while the main thread waits or handles UI logic [3].

## Master

The master is the most concurrent component because it accepts client RPCs, receives worker heartbeats, updates job and task state, assigns work, records intermediate output metadata, and handles worker failure detection [1][4]. It acts as a control plane only; it does not execute mapper or reducer user code itself [1][4].

A good master design combines gRPC server threads with a small number of internal coordination threads. RPC handlers should do short state updates and signal scheduling activity, while longer scheduling and recovery logic should run outside the hot RPC path because gRPC callback reactions may run in parallel and should not block unnecessarily [2][3].

### Master threads

- gRPC server threads: handle client requests and worker RPCs such as registration, heartbeat, and task completion [2].
- Scheduler thread: examines runnable jobs and available worker slots, then prepares task assignments [1][4].
- Heartbeat monitor thread: checks last-seen heartbeat timestamps and marks workers dead after timeout [4].
- Optional cleanup/history thread: removes old temporary state and archives completed job metadata.

### Master shared state

The master typically owns these shared structures:

- `JobRegistry`
- `WorkerRegistry`
- `JobInProgress` / `TaskInProgress`
- `IntermediateIndex`

These structures must be synchronized because multiple RPC callbacks may update them concurrently. A simple design can start with coarse-grained locking and later move to per-job or per-registry locks if contention becomes a problem [2].

### Master concurrency style

The master should be thought of as an event-driven scheduler. Important events include job submission, worker registration, heartbeat arrival, task completion, task failure, and worker timeout [1][4].

A clean model is: RPC thread receives an event, updates state, signals the scheduler, and returns quickly. The scheduler thread then decides which tasks become assignable and which worker should receive them on the next heartbeat reply [1][2].

## Worker

Each worker is its own process and mixes lightweight control-plane concurrency with bounded execution concurrency. Workers register with the master, send heartbeats, execute map/reduce tasks up to configured slot limits, and serve shuffle data to reducers when needed [1][4].

The worker therefore has two distinct types of activity. The control plane covers registration, heartbeat, completion, and failure reporting, while the data plane covers task execution, local file I/O, and shuffle serving/fetching [4][3].

### Worker threads

- gRPC server threads: serve shuffle-data requests and any local control RPCs [2][3].
- Heartbeat thread: periodically reports status and free slot counts to the master [1][4].
- Task execution pool: runs map and reduce task attempts up to the configured capacity [1].
- Optional shuffle-fetch helper threads: used by reduce tasks to fetch map outputs with bounded parallelism [3].

### Worker slot model

Workers should use explicit slot-based concurrency, similar to Hadoop-style map and reduce slots. For example, a worker may be configured with `mapSlots = 2` and `reduceSlots = 1`, meaning it can run at most two map attempts and one reduce attempt concurrently [1].

This keeps execution bounded and visible to the master. The worker reports free slots during heartbeat, and the master only assigns work that fits within those limits [1][4].

### Worker synchronization

Worker synchronization is simpler than master synchronization because each task attempt should own its own execution context and files. The worker mainly needs synchronized access to slot accounting, task tables, and any shared local metadata [2].

## Recommended default model

| Process | Threads / Concurrency                                                 | Main purpose                                                    |
| ------- | --------------------------------------------------------------------- | --------------------------------------------------------------- |
| Client  | 1 main thread, optional polling thread                                | Submit jobs and observe progress [1]                            |
| Master  | gRPC server threads + 1 scheduler thread + 1 heartbeat monitor thread | Coordinate jobs, workers, retries, and job completion [1][4][2] |
| Worker  | gRPC server threads + 1 heartbeat thread + bounded task pool          | Execute tasks and serve shuffle data [1][4][3]                  |

## Why this model works

This model keeps the client simple, which matches its narrow role as a submission and monitoring endpoint. It keeps the master focused on control-plane concurrency rather than computation, and it lets the workers carry the actual parallel execution load across multiple processes [1][4].

It also matches gRPC C++ best practices: callback handlers should remain lightweight, deadlines should be used, and shared structures must assume concurrent access. That makes the design realistic without becoming unnecessarily complex for a local multi-process MapReduce implementation [2][3].
