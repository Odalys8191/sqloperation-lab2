cmake_minimum_required(VERSION 3.8)

project(DATACAPSULE C CXX)

if(MSVC)
  add_definitions(-D_WIN32_WINNT=0x600)
endif()

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 REQUIRED)

include_directories(${Python3_INCLUDE_DIRS})

set(CMAKE_BUILD_TYPE Debug) # For gdb Debug

# This branch assumes that gRPC and all its dependencies are already installed
# on this system, so they can be located by find_package().

# Find Protobuf installation
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
option(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
if(CMAKE_CROSSCOMPILING)
    find_program(_PROTOBUF_PROTOC protoc)
else()
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP gRPC::grpc++)
if(CMAKE_CROSSCOMPILING)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

# Proto file
get_filename_component(server_interaction_proto "${PROJECT_SOURCE_DIR}/protos/serverInteraction.proto" ABSOLUTE)
get_filename_component(server_interaction_proto_path "${server_interaction_proto}" PATH)

# Google APIs directory
set(googleapis_path "${PROJECT_SOURCE_DIR}/include/googleapis")

# Generated sources
set(server_interaction_proto_srcs "${PROJECT_SOURCE_DIR}/protos/serverInteraction.pb.cc")
set(server_interaction_proto_hdrs "${PROJECT_SOURCE_DIR}/protos/serverInteraction.pb.h")
set(server_interaction_grpc_srcs "${PROJECT_SOURCE_DIR}/protos/serverInteraction.grpc.pb.cc")
set(server_interaction_grpc_hdrs "${PROJECT_SOURCE_DIR}/protos/serverInteraction.grpc.pb.h")
set(google_api_annotations_pb_srcs "${PROJECT_SOURCE_DIR}/protos/google/api/annotations.pb.cc")
set(google_api_annotations_pb_hdrs "${PROJECT_SOURCE_DIR}/protos/google/api/annotations.pb.h")
set(google_api_http_pb_srcs "${PROJECT_SOURCE_DIR}/protos/google/api/http.pb.cc")
set(google_api_http_pb_hdrs "${PROJECT_SOURCE_DIR}/protos/google/api/http.pb.h")

# Generate protobuf and gRPC code
add_custom_command(
    OUTPUT "${server_interaction_proto_srcs}" "${server_interaction_proto_hdrs}"
           "${server_interaction_grpc_srcs}" "${server_interaction_grpc_hdrs}"
           "${google_api_annotations_pb_srcs}" "${google_api_annotations_pb_hdrs}"
           "${google_api_http_pb_srcs}" "${google_api_http_pb_hdrs}"
    COMMAND ${_PROTOBUF_PROTOC}
    ARGS --grpc_out="${PROJECT_SOURCE_DIR}/protos"
         --cpp_out="${PROJECT_SOURCE_DIR}/protos"
         -I "${server_interaction_proto_path}"
         -I "${googleapis_path}"
         -I "${PROTOBUF_WKT_INCLUDE_DIR}"
         --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
         "${server_interaction_proto}" 
         "${googleapis_path}/google/api/annotations.proto"
         "${googleapis_path}/google/api/http.proto"
    DEPENDS "${server_interaction_proto}"
            "${googleapis_path}/google/api/annotations.proto"
            "${googleapis_path}/google/api/http.proto"
)

# Include directories
include_directories("${PROJECT_SOURCE_DIR}/protos")
include_directories("${googleapis_path}")

# pbc库，头文件，库文件目录
include_directories(usr/local/include/pbc)
link_directories(usr/local/bin)
link_directories(usr/local/lib)
#target_link_directories(consumer PRIVATE usr/local/bin)

# server_interaction_grpc_proto
add_library(server_interaction_grpc_proto
  ${server_interaction_grpc_srcs}
  ${server_interaction_grpc_hdrs}
  ${server_interaction_proto_srcs}
  ${server_interaction_proto_hdrs}
  ${google_api_annotations_pb_srcs}
  ${google_api_annotations_pb_hdrs}
  ${google_api_http_pb_srcs}
  ${google_api_http_pb_hdrs})
target_link_libraries(server_interaction_grpc_proto
  absl::check
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# 编译ABE库abe_core，将include/common/abe目录下所有cpp文件编译成静态库
file(GLOB AA_SRC_FILES "${PROJECT_SOURCE_DIR}/include/common/abe/*.cpp")
add_library(abe_core STATIC ${AA_SRC_FILES})

# Cloud Server
file(GLOB CLOUD_SERVER_SRC_FILES
"${PROJECT_SOURCE_DIR}/include/common/*.hpp"
"${PROJECT_SOURCE_DIR}/include/common/*.h"
"${PROJECT_SOURCE_DIR}/include/common/*.cpp"
"${PROJECT_SOURCE_DIR}/include/cloudServer/*.h"
"${PROJECT_SOURCE_DIR}/src/cloudServer/*.cpp"
)

add_executable(cloud ${CLOUD_SERVER_SRC_FILES})

target_link_libraries(cloud
    OpenSSL::SSL
    OpenSSL::Crypto
    server_interaction_grpc_proto
    absl::check
    absl::flags
    absl::flags_parse
    absl::log
    argon2
    mysqlclient
    pbc
    gmp
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})

# Etcd Server
file(GLOB ETCD_SERVER_SRC_FILES
"${PROJECT_SOURCE_DIR}/include/common/*.hpp"
"${PROJECT_SOURCE_DIR}/include/common/*.h"
"${PROJECT_SOURCE_DIR}/include/common/*.cpp"
"${PROJECT_SOURCE_DIR}/include/etcdServer/*.h"
"${PROJECT_SOURCE_DIR}/src/etcdServer/*.cpp"
)

add_executable(etcd ${ETCD_SERVER_SRC_FILES})

target_link_libraries(etcd
    OpenSSL::SSL
    OpenSSL::Crypto
    server_interaction_grpc_proto
    absl::check
    absl::flags
    absl::flags_parse
    absl::log
    argon2
    mysqlclient
    pbc
    gmp
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})

# 编译 CA 服务器可执行文件
add_executable(ca src/certificateAuthority/main.cpp src/certificateAuthority/server.cpp include/common/aes256.openssl.cpp)

# 链接可执行文件的依赖库
target_link_libraries(ca
    server_interaction_grpc_proto
    absl::flags
    absl::flags_parse
    abe_core
    pbc
    gmp
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})

# 编译AA_Server

add_executable(AA_server src/AA_Server/AA_Server.cpp src/AA_Server/main.cpp)


target_link_libraries(AA_server
    server_interaction_grpc_proto
    abe_core
    absl::check
    pbc
    gmp
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
)

# 编译 usermanager 服务器可执行文件
add_executable(userServer src/userServer/main.cpp src/userServer/server.cpp include/userServer/userdb.cpp include/userServer/password_utils.cpp include/common/aes256.openssl.cpp)

target_link_libraries(userServer
    server_interaction_grpc_proto
    absl::flags
    absl::flags_parse
    argon2
    mysqlclient
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})

# Consumer
file(GLOB CONSUMER_SRC_FILES
"${PROJECT_SOURCE_DIR}/include/common/*.hpp"
"${PROJECT_SOURCE_DIR}/include/common/*.h"
"${PROJECT_SOURCE_DIR}/include/common/*.cpp"
"${PROJECT_SOURCE_DIR}/include/consumer/*.h"
"${PROJECT_SOURCE_DIR}/include/consumer/computeAPI/*.h"
"${PROJECT_SOURCE_DIR}/src/consumer/*.cpp"
"${PROJECT_SOURCE_DIR}/src/consumer/computeAPI/*.cpp"
"${PROJECT_SOURCE_DIR}/include/userServer/password_utils.cpp"
"${PROJECT_SOURCE_DIR}/include/common/abe/*.cpp"
)

add_executable(consumer ${CONSUMER_SRC_FILES})

target_link_libraries(consumer
    mysqlcppconnx
    OpenSSL::SSL
    OpenSSL::Crypto
    server_interaction_grpc_proto
    abe_core
    absl::check
    absl::flags
    absl::flags_parse
    absl::log
    argon2
    mysqlclient
    pbc
    gmp
    pybind11::embed
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})

# Producer
file(GLOB CONSUMER_SRC_FILES
"${PROJECT_SOURCE_DIR}/include/common/*.hpp"
"${PROJECT_SOURCE_DIR}/include/common/*.h"
"${PROJECT_SOURCE_DIR}/include/common/*.cpp"
"${PROJECT_SOURCE_DIR}/include/producer/*.h"
"${PROJECT_SOURCE_DIR}/src/producer/*.cpp"
)

add_executable(producer ${CONSUMER_SRC_FILES})

target_link_libraries(producer
    OpenSSL::SSL
    OpenSSL::Crypto
    abe_core
    server_interaction_grpc_proto
    absl::check
    absl::flags
    absl::flags_parse
    absl::log
    argon2
    mysqlclient
    pbc
    gmp
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})

# gateway
file(GLOB GATEWAY_SRC_FILES
"${PROJECT_SOURCE_DIR}/include/common/base64.openssl.h"
"${PROJECT_SOURCE_DIR}/include/common/base64.openssl.cpp"
"${PROJECT_SOURCE_DIR}/include/gateway/*.h"
"${PROJECT_SOURCE_DIR}/src/gateway/*.cpp"
)

add_executable(gateway ${GATEWAY_SRC_FILES})

target_link_libraries(gateway
    OpenSSL::SSL
    OpenSSL::Crypto
    abe_core
    pbc
    gmp
    server_interaction_grpc_proto
    absl::check
    absl::flags
    absl::flags_parse
    absl::log
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})

# fileServer
file(GLOB FILESERVER_SRC_FILES
"${PROJECT_SOURCE_DIR}/src/fileServer/*.cpp"
)

add_executable(fileServer ${FILESERVER_SRC_FILES})
target_link_libraries(fileServer PRIVATE Threads::Threads)
