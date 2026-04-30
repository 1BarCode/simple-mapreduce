# RPC Protocol

## Overview

This document defines the gRPC protocol used between the client, master, and worker processes. The protocol is designed to resemble Hadoop MRv1’s JobClient/JobTracker/TaskTracker interactions while fitting a local multi-process system with explicit gRPC services.

The protocol is divided into three categories:

- **client -> master**: job submission and status.
- **worker -> master**: registration, heartbeat, task completion/failure.
- **worker -> worker**: shuffle data transfer.

Unary RPCs are used for most control-plane actions. Server-streaming is used for shuffle transfer because intermediate partitions may be large and naturally chunked.

## Service summary

### ClientMasterService

Used by the CLI/client to submit jobs and query status.

```proto
service ClientMasterService {
  rpc SubmitJob(SubmitJobRequest) returns (SubmitJobReply);
  rpc GetJobStatus(GetJobStatusRequest) returns (GetJobStatusReply);
  rpc GetCounters(GetCountersRequest) returns (GetCountersReply);
  rpc GetClusterStatus(GetClusterStatusRequest) returns (GetClusterStatusReply);
  rpc KillJob(KillJobRequest) returns (KillJobReply);
}
```

### JobTrackerService

Used by workers to register, heartbeat, and report execution outcomes.

```proto
service JobTrackerService {
  rpc RegisterWorker(RegisterWorkerRequest) returns (RegisterWorkerReply);
  rpc Heartbeat(HeartbeatRequest) returns (HeartbeatReply);
  rpc ReportTaskCompletion(ReportTaskCompletionRequest)
      returns (ReportTaskCompletionReply);
  rpc ReportTaskFailure(ReportTaskFailureRequest)
      returns (ReportTaskFailureReply);
}
```

### ShuffleDataService

Used by reducers to fetch map-output partitions from workers.

```proto
service ShuffleDataService {
  rpc FetchMapOutput(FetchMapOutputRequest) returns (stream ShuffleChunk);
}
```

### Optional ClusterBootstrapService

This service is optional. In the current local design, worker provisioning is typically process spawning performed directly by the master rather than over gRPC. If remote bootstrap is added later, a separate bootstrap service can be introduced.

## Shared identifiers

These messages should live in `proto/common/ids.proto`.

```proto
message JobId {
  int64 epoch = 1;
  int32 seq = 2;
}

message TaskId {
  JobId job_id = 1;
  bool is_map = 2;
  int32 task_number = 3;
}

message TaskAttemptId {
  TaskId task_id = 1;
  int32 attempt_number = 2;
}

message WorkerId {
  string value = 1;
}
```

`JobId` is cluster-unique for the master’s lifetime. `TaskId` identifies a logical map or reduce task. `TaskAttemptId` distinguishes retries of the same task.

## Shared enums

These messages should live in `proto/common/status.proto`.

```proto
enum JobState {
  JOB_STATE_UNSPECIFIED = 0;
  PREP = 1;
  RUNNING_MAP = 2;
  RUNNING_REDUCE = 3;
  SUCCEEDED = 4;
  FAILED = 5;
  KILLED = 6;
}

enum TaskState {
  TASK_STATE_UNSPECIFIED = 0;
  UNASSIGNED = 1;
  ASSIGNED = 2;
  RUNNING = 3;
  SUCCEEDED_TASK = 4;
  FAILED_TASK = 5;
  KILLED_TASK = 6;
}

enum TaskType {
  TASK_TYPE_UNSPECIFIED = 0;
  MAP = 1;
  REDUCE = 2;
}
```

## Shared metadata types

These messages should live in `proto/common/types.proto`.

```proto
message CounterEntry {
  string name = 1;
  int64 value = 2;
}

message InputSplit {
  string input_path = 1;
  uint64 offset = 2;
  uint64 length = 3;
  int32 split_number = 4;
}

message MapOutputLocation {
  WorkerId worker_id = 1;
  string worker_address = 2;
  TaskAttemptId map_attempt_id = 3;
  int32 reduce_partition = 4;
  string file_path = 5;
  uint64 size_bytes = 6;
}

message WorkerInfo {
  WorkerId worker_id = 1;
  string address = 2;
  int32 map_slots = 3;
  int32 reduce_slots = 4;
  string work_dir = 5;
}

message TaskAssignment {
  TaskAttemptId attempt_id = 1;
  TaskType type = 2;

  oneof spec {
    MapTaskSpec map_spec = 10;
    ReduceTaskSpec reduce_spec = 11;
  }
}

message MapTaskSpec {
  InputSplit split = 1;
  string mapper_name = 2;
  string combiner_name = 3;
  string partitioner_name = 4;
  int32 num_reduce_tasks = 5;
  string job_work_dir = 6;
}

message ReduceTaskSpec {
  int32 reduce_partition = 1;
  string reducer_name = 2;
  string output_format_name = 3;
  repeated MapOutputLocation inputs = 4;
  string output_path = 5;
  string job_work_dir = 6;
}
```

`MapOutputLocation` is critical because the master must remember where completed map partitions reside and provide those locations to reducers.

## ClientMasterService messages

These messages should live in `proto/client/job_client.proto`.

### SubmitJob

```proto
message SubmitJobRequest {
  string job_name = 1;
  string input_path = 2;
  string output_path = 3;

  string mapper_name = 4;
  string reducer_name = 5;
  string combiner_name = 6;
  string partitioner_name = 7;
  string input_format_name = 8;
  string output_format_name = 9;

  int32 num_reduce_tasks = 10;
  map<string, string> properties = 11;
}

message SubmitJobReply {
  JobId job_id = 1;
  JobState state = 2;
}
```

The client submits logical job configuration only. It does not send executable code over RPC.

### GetJobStatus

```proto
message GetJobStatusRequest {
  JobId job_id = 1;
}

message JobStatusProto {
  JobId job_id = 1;
  JobState state = 2;
  float map_progress = 3;
  float reduce_progress = 4;
  int32 total_maps = 5;
  int32 completed_maps = 6;
  int32 total_reduces = 7;
  int32 completed_reduces = 8;
  string status_message = 9;
  int64 submit_time_ms = 10;
  int64 start_time_ms = 11;
  int64 finish_time_ms = 12;
}

message GetJobStatusReply {
  JobStatusProto status = 1;
}
```

A client-side `waitForCompletion(true)` call repeatedly invokes `GetJobStatus` and prints progress until the job reaches a terminal state.

### GetCounters

```proto
message GetCountersRequest {
  JobId job_id = 1;
}

message GetCountersReply {
  repeated CounterEntry counters = 1;
}
```

### GetClusterStatus

```proto
message GetClusterStatusRequest {}

message ClusterStatusProto {
  int32 total_workers = 1;
  int32 live_workers = 2;
  int32 dead_workers = 3;
  int32 total_map_slots = 4;
  int32 free_map_slots = 5;
  int32 total_reduce_slots = 6;
  int32 free_reduce_slots = 7;
}

message GetClusterStatusReply {
  ClusterStatusProto status = 1;
}
```

### KillJob

```proto
message KillJobRequest {
  JobId job_id = 1;
}

message KillJobReply {
  bool accepted = 1;
}
```

## JobTrackerService messages

These messages should live in `proto/master/job_tracker.proto`.

### RegisterWorker

```proto
message RegisterWorkerRequest {
  WorkerInfo worker = 1;
  int64 start_time_ms = 2;
  string version = 3;
}

message RegisterWorkerReply {
  bool accepted = 1;
  int32 heartbeat_interval_ms = 2;
}
```

Workers must register before the master considers them schedulable.

### Heartbeat

Heartbeats are the core scheduling primitive. Workers report health and available capacity; the master may respond with task-launch actions. This is intentionally Hadoop-like, where TaskTrackers periodically heartbeat and receive instructions from the JobTracker.

```proto
message RunningTaskStatus {
  TaskAttemptId attempt_id = 1;
  TaskType type = 2;
  TaskState state = 3;
  float progress = 4;
  string status_message = 5;
}

message HeartbeatRequest {
  WorkerId worker_id = 1;
  int32 free_map_slots = 2;
  int32 free_reduce_slots = 3;
  repeated RunningTaskStatus running_tasks = 4;
  repeated TaskAttemptId just_finished = 5;
  int64 timestamp_ms = 6;
}

message LaunchTaskAction {
  TaskAssignment assignment = 1;
}

message KillTaskAction {
  TaskAttemptId attempt_id = 1;
  string reason = 2;
}

message HeartbeatReply {
  repeated LaunchTaskAction launch_tasks = 1;
  repeated KillTaskAction kill_tasks = 2;
  int32 next_heartbeat_ms = 3;
}
```

In the initial implementation, `just_finished` can be optional if completions are always sent through explicit completion RPCs. Keeping it available makes the protocol more extensible.

## Task completion and failure messages

### ReportTaskCompletion

```proto
message ReportTaskCompletionRequest {
  WorkerId worker_id = 1;
  TaskAttemptId attempt_id = 2;
  TaskType type = 3;
  repeated CounterEntry counters = 4;

  oneof result {
    MapTaskResult map_result = 10;
    ReduceTaskResult reduce_result = 11;
  }
}

message MapTaskResult {
  repeated MapOutputLocation outputs = 1;
  uint64 records_emitted = 2;
}

message ReduceTaskResult {
  string output_file = 1;
  uint64 records_emitted = 2;
}

message ReportTaskCompletionReply {
  bool accepted = 1;
}
```

For map completions, the master records the provided partition locations as the authoritative shuffle metadata for that task attempt.

### ReportTaskFailure

```proto
message ReportTaskFailureRequest {
  WorkerId worker_id = 1;
  TaskAttemptId attempt_id = 2;
  TaskType type = 3;
  string error_message = 4;
  bool retryable = 5;
}

message ReportTaskFailureReply {
  bool accepted = 1;
}
```

The master uses retry policy plus attempt count to decide whether to reschedule the task or fail the job.

## ShuffleDataService messages

These messages should live in `proto/worker/shuffle_data.proto`.

```proto
message FetchMapOutputRequest {
  TaskAttemptId map_attempt_id = 1;
  int32 reduce_partition = 2;
}

message ShuffleChunk {
  bytes data = 1;
}
```

The reducer contacts the worker listed in `MapOutputLocation.worker_address` and streams the partition file contents. Server-streaming is preferred here because files may be large and do not fit naturally into one unary response.

## Deadlines and timeouts

Every RPC should use explicit deadlines. gRPC guidance recommends using deadlines so callers do not wait forever on failed or stalled peers .

Recommended defaults:

- `SubmitJob`: 3s
- `GetJobStatus`: 1s
- `GetClusterStatus`: 1s
- `RegisterWorker`: 3s
- `Heartbeat`: 1s
- `ReportTaskCompletion`: 3s
- `ReportTaskFailure`: 3s
- `FetchMapOutput`: 10s to start, longer overall stream timeout if needed

Heartbeat timeout on the master should be a multiple of the worker heartbeat interval, for example 3 missed heartbeats before a worker is declared dead.

## Failure semantics

### Worker timeout

If the master stops receiving heartbeats from a worker within the configured timeout window:

- the worker is marked dead,
- its running attempts are failed,
- its completed map outputs are invalidated,
- affected tasks are re-queued.

This follows the MapReduce assumption that map output stored on a dead worker’s local disk is no longer usable.

### Stale completions

A worker may complete an old attempt after the master has already reissued a retry. The master must reject stale `ReportTaskCompletion` messages if the attempt id is no longer current.

### Shuffle fetch failure

If a reducer cannot fetch a map partition:

- it reports failure or retries locally,
- the master may decide whether to retry the reduce attempt,
- repeated fetch failure from the same worker may trigger worker death detection.

## Recommended implementation notes

- Use unary RPCs for all control-plane operations.
- Use server-streaming for `FetchMapOutput`.
- Keep RPC handlers lightweight; schedule heavy work on internal executors.
- Treat heartbeat as both a liveness signal and scheduling opportunity.
- Never consider a job complete until all reduce tasks have successfully completed.

## Protocol evolution

Future additions may include:

- task completion events stream to clients,
- speculative execution hints,
- master-to-worker kill/cleanup messages,
- job history RPCs,
- authenticated worker registration,
- compression negotiation for shuffle streams.

The current protocol is intentionally minimal but preserves the essential Hadoop-like scheduling and MapReduce dataflow behavior.
