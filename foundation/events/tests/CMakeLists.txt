find_package(GTest REQUIRED)

set(EVENTS_TEST_SOURCES
    test_EventDispatcher.cpp
)

add_executable(VoxelEditor_Events_Tests ${EVENTS_TEST_SOURCES})

target_link_libraries(VoxelEditor_Events_Tests
    VoxelEditor_Events
    VoxelEditor_Math
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(VoxelEditor_Events_Tests PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(VoxelEditor_Events_Tests)