include(GoogleTest)

add_executable(farsounder_tests
  requests_conversion_tests.cpp
  history_parse_tests.cpp
  subscriber_conversion_tests.cpp
)

target_include_directories(farsounder_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_BINARY_DIR}/generated
    ${protobuf_SOURCE_DIR}/src
)

target_link_libraries(farsounder_tests
  PRIVATE
    farsounder
    farsounder_proto
    GTest::gtest
    GTest::gtest_main
    protobuf::libprotobuf
    nlohmann_json::nlohmann_json
)

add_custom_command(TARGET farsounder_tests POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:farsounder>
          $<TARGET_FILE_DIR:farsounder_tests>
)

gtest_discover_tests(farsounder_tests)
