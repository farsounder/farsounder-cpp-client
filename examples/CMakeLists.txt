# =============================================================================
# FarSounder SDK Example
# =============================================================================
# This CMakeLists.txt demonstrates how to use the farsounder library.
#
# BUILDING AS PART OF MAIN PROJECT (default):
#   cmake -S . -B build
#   cmake --build build
#
# BUILDING STANDALONE (install and sdk binaries):
#   1. Build and install the SDK:
#      cmake -S . -B build
#      cmake --build build --config Release
#      cmake --install build --prefix C:/farsounder-sdk --config Release
#
#   2. Build the example:
#      cmake -S examples -B build-example -DCMAKE_PREFIX_PATH="C:/farsounder-sdk"
#      cmake --build build-example --config Release
#
#   3. Run the example:
#      build-example/Release/farsounder_example.exe
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
  
  # For standalone builds, we need to copy the DLL next to the executable
  set(FARSOUNDER_STANDALONE TRUE)
endif()

# Build the example - just link to farsounder, no other deps needed!
add_executable(farsounder_example basic_client.cpp)
target_link_libraries(farsounder_example PRIVATE farsounder::farsounder)

# Copy the DLL to the output directory for standalone builds
if(FARSOUNDER_STANDALONE AND WIN32)
  # Get the DLL location from the imported target
  get_target_property(FARSOUNDER_DLL_LOCATION farsounder::farsounder IMPORTED_LOCATION)
  
  if(FARSOUNDER_DLL_LOCATION)
    add_custom_command(TARGET farsounder_example POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FARSOUNDER_DLL_LOCATION}"
        "$<TARGET_FILE_DIR:farsounder_example>"
      COMMENT "Copying farsounder.dll to output directory"
    )
  endif()
endif()
