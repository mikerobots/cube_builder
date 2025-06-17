cmake_minimum_required(VERSION 3.10)

# Camera-Cube Visibility Tests (Google Test version)
add_executable(test_camera_cube_visibility
    test_camera_cube_visibility.cpp
)

target_link_libraries(test_camera_cube_visibility
    VoxelEditor_Camera
    VoxelEditor_VoxelData
    VoxelEditor_Math
    gtest
    gtest_main
)

# Add test
add_test(NAME CameraCubeVisibility COMMAND test_camera_cube_visibility)

# Camera-Cube Visibility Tests (Simple version)
add_executable(test_camera_cube_visibility_simple
    test_camera_cube_visibility_simple.cpp
)

target_link_libraries(test_camera_cube_visibility_simple
    VoxelEditor_Camera
    VoxelEditor_VoxelData
    VoxelEditor_Math
)

# Add test
add_test(NAME CameraCubeVisibilitySimple COMMAND test_camera_cube_visibility_simple)

# Ground Plane Voxel Placement Integration Test
add_executable(test_ground_plane_voxel_placement
    test_ground_plane_voxel_placement.cpp
)

target_link_libraries(test_ground_plane_voxel_placement
    VoxelEditor_VoxelData
    VoxelEditor_VisualFeedback
    VoxelEditor_Input
    VoxelEditor_UndoRedo
    VoxelEditor_Math
    VoxelEditor_Events
    VoxelEditor_Logging
    gtest
    gtest_main
)

# Add test
add_test(NAME GroundPlaneVoxelPlacement COMMAND test_ground_plane_voxel_placement)

# Mouse Ground Plane Clicking Integration Test
add_executable(test_mouse_ground_plane_clicking
    test_mouse_ground_plane_clicking.cpp
)

target_link_libraries(test_mouse_ground_plane_clicking
    VoxelEditor_VoxelData
    VoxelEditor_VisualFeedback
    VoxelEditor_Input
    VoxelEditor_UndoRedo
    VoxelEditor_Camera
    VoxelEditor_Math
    VoxelEditor_Events
    VoxelEditor_Logging
    gtest
    gtest_main
)

# Add test
add_test(NAME MouseGroundPlaneClicking COMMAND test_mouse_ground_plane_clicking)

# Mouse Boundary Clicking Integration Test
add_executable(test_mouse_boundary_clicking
    test_mouse_boundary_clicking.cpp
)

target_link_libraries(test_mouse_boundary_clicking
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)

# Add test
add_test(NAME MouseBoundaryClicking COMMAND test_mouse_boundary_clicking)

# Workspace Boundary Placement Test
add_executable(test_workspace_boundary_placement
    test_workspace_boundary_placement.cpp
)

target_link_libraries(test_workspace_boundary_placement
    VoxelEditor_VoxelData
    VoxelEditor_Math
    VoxelEditor_Events
    VoxelEditor_Logging
    gtest
    gtest_main
)

# Add test
add_test(NAME WorkspaceBoundaryPlacement COMMAND test_workspace_boundary_placement)

# Set working directory for tests
set_tests_properties(CameraCubeVisibility CameraCubeVisibilitySimple GroundPlaneVoxelPlacement 
    MouseGroundPlaneClicking MouseBoundaryClicking WorkspaceBoundaryPlacement PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)