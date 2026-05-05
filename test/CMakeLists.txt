file(GLOB_RECURSE MR_TEST_SRCS *.cc)

add_executable(mr_tests ${MR_TEST_SRCS})

target_link_libraries(mr_tests PRIVATE
    mr_common
    mr_client
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(mr_tests)
