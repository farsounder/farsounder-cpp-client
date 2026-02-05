# =============================================================================
# FarSounder SDK Example
# =============================================================================
# This CMakeLists.txt demonstrates how to use the farsounder library.
#
# BUILDING AS PART OF MAIN PROJECT (default):
#   cmake -S . -B build
#   cmake --build build
#
# BUILDING STANDALONE (after installing the SDK):
#   1. Build and install the SDK:
#      cmake -S . -B build
#      cmake --build build --config Release
#      cmake --install build --prefix C:/farsounder-sdk
#
#   2. Build the example:
#      cmake -S examples -B build-example -DCMAKE_PREFIX_PATH="C:/farsounder-sdk"
#      cmake --build build-example
# =============================================================================

cmake_minimum_required(VERSION 3.16)

# Detect if we're being built standalone or as part of the main project
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  # Standalone build
  project(farsounder_example LANGUAGES CXX)

  set(CMAKE_CXX_STANDARD 23)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  # Find the installed farsounder package
  find_package(farsounder REQUIRED)
endif()

# Build the example - just link to farsounder, no other deps needed!
add_executable(farsounder_example basic_client.cpp)
target_link_libraries(farsounder_example PRIVATE farsounder::farsounder)
