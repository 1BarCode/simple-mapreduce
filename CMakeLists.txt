cmake_minimum_required(VERSION 3.20)
project(simple_mapreduce CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
find_package(Threads REQUIRED)

include(FetchContent)

# Disable tests/examples/install rules in all fetched dependencies
set(gRPC_BUILD_TESTS        OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_CSHARP_EXT   OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_TESTS     OFF CACHE BOOL "" FORCE)
set(protobuf_INSTALL         OFF CACHE BOOL "" FORCE)
set(utf8_range_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(ABSL_BUILD_TESTING       OFF CACHE BOOL "" FORCE)
set(ABSL_ENABLE_INSTALL      OFF CACHE BOOL "" FORCE)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

FetchContent_Declare(
    gRPC
    GIT_REPOSITORY https://github.com/grpc/grpc
    GIT_TAG        v1.66.2
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE
)

# Allow CMake 4.x to configure deps that declare cmake_minimum_required < 3.5
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
# Propagate our C++17 standard into abseil (suppresses its future-default warning)
set(ABSL_PROPAGATE_CXX_STD ON)

# gRPC transitively fetches protobuf and abseil
FetchContent_MakeAvailable(gRPC googletest)

# Normalize targets so the rest of the file works whether deps were fetched or
# found via find_package (namespaced vs non-namespaced targets differ)
if(NOT TARGET gRPC::grpc++)
    add_library(gRPC::grpc++ ALIAS grpc++)
endif()
if(NOT TARGET protobuf::libprotobuf)
    add_library(protobuf::libprotobuf ALIAS libprotobuf)
endif()

# ---------------------------------------------------------------------------
# Common library (api + common + rpc + runtime)
# ---------------------------------------------------------------------------
file(GLOB_RECURSE MR_COMMON_SRCS
    src/api/*.cc
    src/common/*.cc
    src/rpc/*.cc
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
file(GLOB_RECURSE MR_CLIENT_SRCS src/client/*.cc)

add_library(mr_client STATIC ${MR_CLIENT_SRCS})

target_include_directories(mr_client PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(mr_client PUBLIC mr_common)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()
add_subdirectory(test)

# ---------------------------------------------------------------------------
# Executables
# ---------------------------------------------------------------------------
add_executable(master src/master_main.cc)
target_link_libraries(master PRIVATE mr_common)

add_executable(client src/client_main.cc)
target_link_libraries(client PRIVATE mr_client)
