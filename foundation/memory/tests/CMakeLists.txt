find_package(GTest REQUIRED)

set(MEMORY_TEST_SOURCES
    test_MemoryPool.cpp
    test_MemoryTracker.cpp
)

add_executable(VoxelEditor_Memory_Tests ${MEMORY_TEST_SOURCES})

target_link_libraries(VoxelEditor_Memory_Tests
    VoxelEditor_Memory
    VoxelEditor_Events
    VoxelEditor_Math
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(VoxelEditor_Memory_Tests PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(VoxelEditor_Memory_Tests)