find_package(GTest REQUIRED)

set(LOGGING_TEST_SOURCES
    test_Logger.cpp
    test_PerformanceProfiler.cpp
)

add_executable(VoxelEditor_Logging_Tests ${LOGGING_TEST_SOURCES})

target_link_libraries(VoxelEditor_Logging_Tests
    VoxelEditor_Logging
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(VoxelEditor_Logging_Tests PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(VoxelEditor_Logging_Tests)