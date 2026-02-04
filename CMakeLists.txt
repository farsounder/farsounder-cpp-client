cmake_minimum_required(VERSION 3.16)

project(farsounder_sdk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POLICY_VERSION_MINIMUM 3.16)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(protobuf_MSVC_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  protobuf
  URL https://github.com/protocolbuffers/protobuf/archive/refs/tags/v33.5.zip
)

set(ZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZMQ_BUILD_PERFORMANCE_TESTS OFF CACHE BOOL "" FORCE)
set(ZMQ_BUILD_FRAMEWORK OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  zeromq
  URL https://github.com/zeromq/libzmq/archive/refs/tags/v4.3.5.zip
)

FetchContent_Declare(
  cppzmq
  URL https://github.com/zeromq/cppzmq/archive/refs/tags/v4.11.0.zip
)

set(CPPZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)

set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS_SSL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  cpr
  URL https://github.com/libcpr/cpr/archive/refs/tags/1.11.0.zip
)

FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip
)

FetchContent_MakeAvailable(protobuf zeromq cppzmq cpr nlohmann_json)

include(${protobuf_SOURCE_DIR}/cmake/protobuf-generate.cmake)

file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto")

add_library(farsounder_proto)
target_sources(farsounder_proto PRIVATE ${PROTO_FILES})

protobuf_generate(
  TARGET farsounder_proto
  LANGUAGE cpp
  PROTOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
  IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(farsounder_proto
  PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
    "${protobuf_SOURCE_DIR}/src"
)

target_link_libraries(farsounder_proto
  PUBLIC
    protobuf::libprotobuf
)

add_library(farsounder
  src/config.cpp
  src/requests.cpp
  src/subscriber.cpp
)

target_include_directories(farsounder
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(farsounder
  PUBLIC
    farsounder_proto
    libzmq
    cppzmq
    cpr::cpr
    nlohmann_json::nlohmann_json
)

# Create alias for namespaced usage
add_library(farsounder::farsounder ALIAS farsounder)

# Option to build examples
option(FARSOUNDER_BUILD_EXAMPLES "Build examples" ON)

if(FARSOUNDER_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
