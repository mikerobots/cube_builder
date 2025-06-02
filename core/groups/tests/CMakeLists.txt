cmake_minimum_required(VERSION 3.16)

# Find GTest
find_package(GTest REQUIRED)

# Test sources
set(TEST_SOURCES
    TestGroupTypes.cpp
    TestVoxelGroup.cpp
    TestGroupHierarchy.cpp
    TestGroupOperations.cpp
    TestGroupManager.cpp
)

# Create test executable
add_executable(groups_tests ${TEST_SOURCES})

# Link libraries
target_link_libraries(groups_tests
    PRIVATE
        VoxelEditor_Groups
        VoxelEditor_VoxelData
        VoxelEditor_Rendering
        VoxelEditor_Math
        VoxelEditor_Events
        VoxelEditor_Memory
        VoxelEditor_Logging
        GTest::GTest
        GTest::Main
)

# Include directories
target_include_directories(groups_tests
    PRIVATE
        ../include
        ../../
        ../../../
)

# Add to test suite
add_test(NAME groups_unit_tests COMMAND groups_tests)