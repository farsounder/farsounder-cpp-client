cmake_minimum_required(VERSION 3.16)

project(farsounder_sdk VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POLICY_VERSION_MINIMUM 3.16)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =============================================================================
# Options
# =============================================================================
option(FARSOUNDER_BUILD_EXAMPLES "Build examples" ON)
option(FARSOUNDER_BUILD_SHARED "Build shared library (DLL)" OFF)

if(FARSOUNDER_BUILD_SHARED)
  set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
endif()

include(FetchContent)
include(GNUInstallDirs)

# =============================================================================
# Dependencies (all PRIVATE - not exposed to consumers)
# =============================================================================

# Protobuf
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(protobuf_MSVC_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)
set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  protobuf
  URL https://github.com/protocolbuffers/protobuf/archive/refs/tags/v33.5.zip
)

# ZeroMQ
set(ZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZMQ_BUILD_PERFORMANCE_TESTS OFF CACHE BOOL "" FORCE)
set(ZMQ_BUILD_FRAMEWORK OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  zeromq
  URL https://github.com/zeromq/libzmq/archive/refs/tags/v4.3.5.zip
)

# cppzmq (header-only)
set(CPPZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  cppzmq
  URL https://github.com/zeromq/cppzmq/archive/refs/tags/v4.11.0.zip
)

# CPR (HTTP client)
set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS_SSL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  cpr
  URL https://github.com/libcpr/cpr/archive/refs/tags/1.11.0.zip
)

# nlohmann_json (header-only)
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip
)

FetchContent_MakeAvailable(protobuf zeromq cppzmq cpr nlohmann_json)

include(${protobuf_SOURCE_DIR}/cmake/protobuf-generate.cmake)

# =============================================================================
# Proto library (internal, compiled into farsounder)
# =============================================================================
file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto")

add_library(farsounder_proto OBJECT)
target_sources(farsounder_proto PRIVATE ${PROTO_FILES})

protobuf_generate(
  TARGET farsounder_proto
  LANGUAGE cpp
  PROTOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
  IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(farsounder_proto
  PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
    "${protobuf_SOURCE_DIR}/src"
)

target_link_libraries(farsounder_proto
  PRIVATE
    protobuf::libprotobuf
)

# =============================================================================
# Main SDK library
# =============================================================================
add_library(farsounder
  src/config.cpp
  src/requests.cpp
  src/subscriber.cpp
  $<TARGET_OBJECTS:farsounder_proto>
)

# Public include directories (only our headers, no proto)
target_include_directories(farsounder
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
    "${protobuf_SOURCE_DIR}/src"
)

# All dependencies are PRIVATE - not exposed to consumers
target_link_libraries(farsounder
  PRIVATE
    protobuf::libprotobuf
    libzmq
    cppzmq
    cpr::cpr
    nlohmann_json::nlohmann_json
)

# Create alias for namespaced usage
add_library(farsounder::farsounder ALIAS farsounder)

# =============================================================================
# Examples
# =============================================================================
if(FARSOUNDER_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# =============================================================================
# Install rules (manual IMPORTED target - no install(EXPORT) to avoid dep issues)
# =============================================================================
include(CMakePackageConfigHelpers)

# Install the library files directly (no EXPORT)
install(TARGETS farsounder
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install public headers (only our wrapper types, no proto)
install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Generate version file
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/farsounderConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

# Configure the package config (creates IMPORTED target manually)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/farsounderConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/farsounderConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/farsounder
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/farsounderConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/farsounderConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/farsounder
)
