cmake_minimum_required(VERSION 3.20)
project(simple_mapreduce CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# ---------------------------------------------------------------------------
# Common library (api + common + rpc + runtime)
# ---------------------------------------------------------------------------
file(GLOB_RECURSE MR_COMMON_SRCS
    src/api/*.cpp
    src/common/*.cpp
    src/rpc/*.cpp
    src/runtime/*.cpp
    src/runtime/*.cc
)

add_library(mr_common STATIC ${MR_COMMON_SRCS})

target_include_directories(mr_common PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(mr_common PUBLIC
    Threads::Threads
    protobuf::libprotobuf
    gRPC::grpc++
)

# ---------------------------------------------------------------------------
# Client library
# ---------------------------------------------------------------------------
file(GLOB_RECURSE MR_CLIENT_SRCS src/client/*.cpp)

add_library(mr_client STATIC ${MR_CLIENT_SRCS})

target_include_directories(mr_client PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(mr_client PUBLIC mr_common)

# ---------------------------------------------------------------------------
# Executables
# ---------------------------------------------------------------------------
add_executable(master src/main.cpp)
target_link_libraries(master PRIVATE mr_common)

add_executable(client src/client.cpp)
target_link_libraries(client PRIVATE mr_client)
