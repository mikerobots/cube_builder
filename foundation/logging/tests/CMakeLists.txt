find_package(GTest REQUIRED)

set(LOGGING_TEST_SOURCES
    test_unit_foundation_logging_logger.cpp
    test_unit_foundation_logging_performance_profiler.cpp
)

add_executable(test_unit_foundation_logging ${LOGGING_TEST_SOURCES})

target_link_libraries(test_unit_foundation_logging
    VoxelEditor_Logging
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(test_unit_foundation_logging PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(test_unit_foundation_logging)