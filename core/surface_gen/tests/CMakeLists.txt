# Surface Generation Tests
add_executable(test_surface_gen
    TestSurfaceTypes.cpp
    TestMeshBuilder.cpp
    TestDualContouring.cpp
    TestSurfaceGenerator.cpp
)

target_link_libraries(test_surface_gen
    PRIVATE
        VoxelEditor_SurfaceGen
        VoxelEditor_VoxelData
        VoxelEditor_Math
        VoxelEditor_Events
        GTest::gtest_main
)

# Add test
add_test(NAME test_surface_gen COMMAND test_surface_gen)