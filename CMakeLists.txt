cmake_minimum_required(VERSION 3.0.2)

add_compile_options(-std=c++11)

include_directories(${CMAKE_SOURCE_DIR})

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

# FindProtobuf is integrated into CMake >= 3.0.2.
find_package(Protobuf REQUIRED)
find_package(GRPC REQUIRED)
include_directories(${GRPC_INCLUDE_DIR})

set(PROTO_SRCS
    # the auth API is currently unsupported, but is a dependency of rpc.pb.cc
    ${CMAKE_SOURCE_DIR}/proto-src/auth.pb.cc
    ${CMAKE_SOURCE_DIR}/proto-src/auth.grpc.pb.cc
    ${CMAKE_SOURCE_DIR}/proto-src/kv.pb.cc
    ${CMAKE_SOURCE_DIR}/proto-src/kv.grpc.pb.cc
    ${CMAKE_SOURCE_DIR}/proto-src/v3lock.pb.cc
    ${CMAKE_SOURCE_DIR}/proto-src/v3lock.grpc.pb.cc
    ${CMAKE_SOURCE_DIR}/proto-src/rpc.pb.cc
    ${CMAKE_SOURCE_DIR}/proto-src/rpc.grpc.pb.cc
)

add_library(etcd3 SHARED ${CMAKE_SOURCE_DIR}/src/etcd3.cc ${PROTO_SRCS})
target_link_libraries(etcd3 gRPC::grpc++)
target_link_libraries(etcd3 ${PROTOBUF_LIBRARIES})
