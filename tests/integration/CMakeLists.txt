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

# Set working directory for tests
set_tests_properties(CameraCubeVisibility CameraCubeVisibilitySimple PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)