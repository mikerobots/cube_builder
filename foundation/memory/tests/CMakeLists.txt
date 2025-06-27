find_package(GTest REQUIRED)

set(MEMORY_TEST_SOURCES
    test_unit_foundation_memory_pool.cpp
    test_unit_foundation_memory_tracker.cpp
)

add_executable(test_unit_foundation_memory ${MEMORY_TEST_SOURCES})

target_link_libraries(test_unit_foundation_memory
    VoxelEditor_Memory
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(test_unit_foundation_memory PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(test_unit_foundation_memory)