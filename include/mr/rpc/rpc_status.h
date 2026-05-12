#pragma once

#include <grpcpp/grpcpp.h>
#include <absl/status/status.h>

inline grpc::Status ToGrpcStatus(const absl::Status& s) {
    if (s.ok()) return grpc::Status::OK;
    return grpc::Status(grpc::StatusCode::INTERNAL, std::string(s.message()));
}
