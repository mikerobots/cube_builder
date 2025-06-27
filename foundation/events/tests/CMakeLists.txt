find_package(GTest REQUIRED)

set(EVENTS_TEST_SOURCES
    test_unit_foundation_events_dispatcher.cpp
)

add_executable(test_unit_foundation_events ${EVENTS_TEST_SOURCES})

target_link_libraries(test_unit_foundation_events
    VoxelEditor_Events
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(test_unit_foundation_events PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(test_unit_foundation_events)