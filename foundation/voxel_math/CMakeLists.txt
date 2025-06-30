cmake_minimum_required(VERSION 3.16)

# Define the voxel math library
add_library(VoxelEditor_VoxelMath STATIC
    src/VoxelBounds.cpp
    src/VoxelGrid.cpp
    src/FaceOperations.cpp
    src/WorkspaceValidation.cpp
    src/VoxelCollision.cpp
    src/VoxelRaycast.cpp
    src/VoxelMathOptimized.cpp
)

# Set include directories
target_include_directories(VoxelEditor_VoxelMath PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(VoxelEditor_VoxelMath PUBLIC
    VoxelEditor_Math         # For Vector3f, CoordinateTypes, etc.
    VoxelEditor_VoxelData    # For VoxelTypes, VoxelGrid
    Eigen3::Eigen            # For optimized vector math operations
)

# Set compile features
target_compile_features(VoxelEditor_VoxelMath PUBLIC cxx_std_17)

# Add tests if testing is enabled
if(BUILD_TESTING)
    # Unit tests
    add_executable(test_unit_voxel_bounds
        tests/test_unit_voxel_bounds.cpp
    )
    target_link_libraries(test_unit_voxel_bounds
        VoxelEditor_VoxelMath
        VoxelEditor_Math
        VoxelEditor_VoxelData
        GTest::gtest_main
    )
    add_test(NAME test_unit_voxel_bounds COMMAND test_unit_voxel_bounds)

    add_executable(test_unit_voxel_grid
        tests/test_unit_voxel_grid.cpp
    )
    target_link_libraries(test_unit_voxel_grid
        VoxelEditor_VoxelMath
        VoxelEditor_Math
        VoxelEditor_VoxelData
        GTest::gtest_main
    )
    add_test(NAME test_unit_voxel_grid COMMAND test_unit_voxel_grid)

    add_executable(test_unit_face_operations
        tests/test_unit_face_operations.cpp
    )
    target_link_libraries(test_unit_face_operations
        VoxelEditor_VoxelMath
        VoxelEditor_Math
        VoxelEditor_VoxelData
        GTest::gtest_main
    )
    add_test(NAME test_unit_face_operations COMMAND test_unit_face_operations)

    add_executable(test_unit_workspace_validation
        tests/test_unit_workspace_validation.cpp
    )
    target_link_libraries(test_unit_workspace_validation
        VoxelEditor_VoxelMath
        VoxelEditor_Math
        VoxelEditor_VoxelData
        GTest::gtest_main
    )
    add_test(NAME test_unit_workspace_validation COMMAND test_unit_workspace_validation)

    add_executable(test_unit_voxel_collision
        tests/test_unit_voxel_collision.cpp
    )
    target_link_libraries(test_unit_voxel_collision
        VoxelEditor_VoxelMath
        VoxelEditor_Math
        VoxelEditor_VoxelData
        GTest::gtest_main
    )
    add_test(NAME test_unit_voxel_collision COMMAND test_unit_voxel_collision)

    add_executable(test_unit_voxel_raycast
        tests/test_unit_voxel_raycast.cpp
    )
    target_link_libraries(test_unit_voxel_raycast
        VoxelEditor_VoxelMath
        VoxelEditor_Math
        VoxelEditor_VoxelData
        GTest::gtest_main
    )
    add_test(NAME test_unit_voxel_raycast COMMAND test_unit_voxel_raycast)

    add_executable(test_unit_voxel_math_simd
        tests/test_unit_voxel_math_simd.cpp
    )
    target_link_libraries(test_unit_voxel_math_simd
        VoxelEditor_VoxelMath
        VoxelEditor_Math
        VoxelEditor_VoxelData
        GTest::gtest_main
    )
    add_test(NAME test_unit_voxel_math_simd COMMAND test_unit_voxel_math_simd)
endif()