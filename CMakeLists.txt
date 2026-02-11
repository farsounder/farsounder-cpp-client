cmake_minimum_required(VERSION 3.16)

# =============================================================================
# MSVC Runtime - MUST be set before project() to affect all targets
# =============================================================================
cmake_policy(SET CMP0091 NEW)
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

project(farsounder_sdk VERSION 0.1.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POLICY_VERSION_MINIMUM 3.16)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT MSVC)
  # needed for ubuntu build of shared library
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  # turn off -Wdeprecated-declarations for protobuf
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
endif()

# =============================================================================
# Options
# =============================================================================
option(FARSOUNDER_BUILD_EXAMPLES "Build examples" ON)
option(FARSOUNDER_BUILD_TESTS "Build tests" ON)

# Always build as SHARED (DLL) - this embeds all dependencies into the DLL
# so end users only need our DLL + headers
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Force all dependencies to build as STATIC so they get baked into our DLL
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(FetchContent)
include(GNUInstallDirs)

# =============================================================================
# Dependencies (all built static, linked into our DLL)
# =============================================================================

# Abseil (protobuf dependency) - must use dynamic runtime
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
set(ABSL_MSVC_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)

# Protobuf
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(protobuf_MSVC_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)
set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)
set(protobuf_ABSL_PROVIDER "module" CACHE STRING "" FORCE)

FetchContent_Declare(
  protobuf
  URL https://github.com/protocolbuffers/protobuf/archive/refs/tags/v33.5.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# ZeroMQ - force static
set(ZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZMQ_BUILD_PERFORMANCE_TESTS OFF CACHE BOOL "" FORCE)
set(ZMQ_BUILD_FRAMEWORK OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC ON CACHE BOOL "" FORCE)
set(BUILD_SHARED OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  zeromq
  URL https://github.com/zeromq/libzmq/archive/refs/tags/v4.3.5.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# cppzmq (header-only)
set(CPPZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  cppzmq
  URL https://github.com/zeromq/cppzmq/archive/refs/tags/v4.11.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# CPR (HTTP client) - force static
set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS_SSL OFF CACHE BOOL "" FORCE)
set(CPR_FORCE_USE_SYSTEM_CURL OFF CACHE BOOL "" FORCE)
set(BUILD_CPR_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  cpr
  URL https://github.com/libcpr/cpr/archive/refs/tags/1.11.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# nlohmann_json (header-only)
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

if(FARSOUNDER_BUILD_TESTS)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
endif()

FetchContent_MakeAvailable(protobuf zeromq cppzmq cpr nlohmann_json)
if(FARSOUNDER_BUILD_TESTS)
  FetchContent_MakeAvailable(googletest)
endif()

include(${protobuf_SOURCE_DIR}/cmake/protobuf-generate.cmake)

# =============================================================================
# Proto library (internal, compiled into farsounder)
# =============================================================================
file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/proto")

add_library(farsounder_proto STATIC)
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
# Main SDK library - SHARED (DLL) with all deps baked in
# =============================================================================
add_library(farsounder SHARED
  src/config.cpp
  src/requests.cpp
  src/subscriber.cpp
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

# All dependencies are private - linked into the DLL, not exposed to consumers
target_link_libraries(farsounder
  PRIVATE
    farsounder_proto
    protobuf::libprotobuf
    libzmq-static
    cpr::cpr
    nlohmann_json::nlohmann_json
)

# cppzmq is header-only - just add include path (don't link, it pulls in libzmq shared)
target_include_directories(farsounder PRIVATE ${cppzmq_SOURCE_DIR})

# Windows-specific: link required system libraries for networking
if(WIN32)
  target_link_libraries(farsounder PRIVATE ws2_32 iphlpapi)
endif()

# Create alias for namespaced usage
add_library(farsounder::farsounder ALIAS farsounder)

# =============================================================================
# Examples + Tests - these can be disabled with options to cmake
# eg:
# cmake -S . -B build -DFARSOUNDER_BUILD_EXAMPLES=OFF
# cmake -S . -B build -DFARSOUNDER_BUILD_TESTS=OFF
# etc.
# =============================================================================
if(FARSOUNDER_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(FARSOUNDER_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

include(CMakePackageConfigHelpers)

# Install the DLL to bin, import lib to lib
install(TARGETS farsounder
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}   # .lib (import library)
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}   # .so on Linux
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}   # .dll on Windows
)

# Install public headers
install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Generate version file
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/farsounderConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

# Configure the package config
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

# Install LICENSE to share folder
install(FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/farsounder
)
