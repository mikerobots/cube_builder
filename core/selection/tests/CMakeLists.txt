# Core/Selection Tests CMakeLists.txt

enable_testing()

# Find GTest
find_package(GTest REQUIRED)

# Selection tests executable
add_executable(VoxelEditor_Selection_Tests
    TestSelectionTypes.cpp
    TestSelectionSet.cpp
    TestSelectionManager.cpp
    TestBoxSelector.cpp
    TestSphereSelector.cpp
    TestFloodFillSelector.cpp
    test_Selection_requirements.cpp
    TestSelectionPerformance.cpp
)

target_link_libraries(VoxelEditor_Selection_Tests
    VoxelEditor_Selection
    GTest::gtest
    GTest::gtest_main
)

# Add tests
include(GoogleTest)
gtest_discover_tests(VoxelEditor_Selection_Tests)