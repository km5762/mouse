file(GLOB TEST_COMMON_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(test_common ${TEST_COMMON_SOURCES})

target_link_libraries(test_common
    PRIVATE
        common
        GTest::gtest_main
)

target_include_directories(test_common PRIVATE
    ${PROJECT_SOURCE_DIR}/common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

include(GoogleTest)
gtest_discover_tests(test_common)

