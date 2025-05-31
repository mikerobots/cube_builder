find_package(GTest REQUIRED)

set(VOXEL_DATA_TEST_SOURCES
    test_VoxelTypes.cpp
    test_SparseOctree.cpp
    test_VoxelGrid.cpp
    test_WorkspaceManager.cpp
    test_VoxelDataManager.cpp
)

add_executable(VoxelEditor_VoxelData_Tests ${VOXEL_DATA_TEST_SOURCES})

target_link_libraries(VoxelEditor_VoxelData_Tests
    VoxelEditor_VoxelData
    VoxelEditor_Math
    VoxelEditor_Events
    VoxelEditor_Memory
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(VoxelEditor_VoxelData_Tests PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(VoxelEditor_VoxelData_Tests)