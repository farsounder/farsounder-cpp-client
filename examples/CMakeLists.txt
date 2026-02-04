cmake_minimum_required(VERSION 3.16)

# When built standalone, this is the project. When built via add_subdirectory,
# this just defines the example target.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  # Standalone build - need to find the installed package
  project(farsounder_example LANGUAGES CXX)

  set(CMAKE_CXX_STANDARD 23)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  find_package(farsounder REQUIRED)
endif()

add_executable(farsounder_example basic_client.cpp)
target_link_libraries(farsounder_example PRIVATE farsounder::farsounder)

# On Windows, copy DLLs to output directory
if(WIN32 AND TARGET libzmq)
  add_custom_command(TARGET farsounder_example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:libzmq>
      $<TARGET_FILE_DIR:farsounder_example>
    COMMAND_EXPAND_LISTS
  )
endif()
