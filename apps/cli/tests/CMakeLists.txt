find_package(GTest REQUIRED)

# CLI Integration Tests
set(TEST_SOURCES
    integration_test.cpp
    test_cli_headless.cpp
    test_cli_commands.cpp
    test_cli_error_handling.cpp
    test_cli_rendering_basic.cpp
    test_zoom_behavior.cpp
)

# Create test executable
add_executable(VoxelEditor_CLI_Tests ${TEST_SOURCES})

# Include directories
target_include_directories(VoxelEditor_CLI_Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

# Link libraries
target_link_libraries(VoxelEditor_CLI_Tests PRIVATE
    # CLI library which includes Application class
    VoxelEditor_CLI_Lib
    
    # Test framework
    GTest::gtest
    GTest::gtest_main
)

# Add test
add_test(NAME VoxelEditor_CLI_Tests COMMAND VoxelEditor_CLI_Tests)

# Enable C++20
target_compile_features(VoxelEditor_CLI_Tests PRIVATE cxx_std_20)

# VoxelMeshGenerator Tests
set(MESH_GENERATOR_TEST_SOURCES
    test_VoxelMeshGenerator.cpp
)

add_executable(VoxelEditor_VoxelMeshGenerator_Tests ${MESH_GENERATOR_TEST_SOURCES})

target_include_directories(VoxelEditor_VoxelMeshGenerator_Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(VoxelEditor_VoxelMeshGenerator_Tests PRIVATE
    # Use the CLI library which includes VoxelMeshGenerator
    VoxelEditor_CLI_Lib
    
    # Test framework
    GTest::gtest
    GTest::gtest_main
)

# Add mesh generator test
add_test(NAME VoxelEditor_VoxelMeshGenerator_Tests COMMAND VoxelEditor_VoxelMeshGenerator_Tests)

# Enable C++20
target_compile_features(VoxelEditor_VoxelMeshGenerator_Tests PRIVATE cxx_std_20)

# Discover tests
include(GoogleTest)
gtest_discover_tests(VoxelEditor_CLI_Tests)
gtest_discover_tests(VoxelEditor_VoxelMeshGenerator_Tests)

# Face clicking tests
add_executable(test_face_clicking
    test_face_clicking.cpp
)

target_include_directories(test_face_clicking PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_face_clicking PRIVATE
    VoxelEditor_CLI_Lib
    VoxelEditor_VisualFeedback
    VoxelEditor_UndoRedo
    VoxelEditor_VoxelData
    VoxelEditor_Camera
    GTest::gtest_main
)

target_compile_features(test_face_clicking PRIVATE cxx_std_20)
gtest_discover_tests(test_face_clicking)

# Click voxel placement tests
add_executable(test_click_voxel_placement
    test_click_voxel_placement.cpp
)

target_include_directories(test_click_voxel_placement PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_click_voxel_placement PRIVATE
    VoxelEditor_VisualFeedback
    VoxelEditor_UndoRedo
    VoxelEditor_VoxelData
    VoxelEditor_Logging
    GTest::gtest_main
)

target_compile_features(test_click_voxel_placement PRIVATE cxx_std_20)
gtest_discover_tests(test_click_voxel_placement)

# Voxel face clicking integration tests
add_executable(test_voxel_face_clicking
    test_voxel_face_clicking.cpp
)

target_include_directories(test_voxel_face_clicking PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_voxel_face_clicking PRIVATE
    VoxelEditor_CLI_Lib
    GTest::gtest_main
)

target_compile_features(test_voxel_face_clicking PRIVATE cxx_std_20)
gtest_discover_tests(test_voxel_face_clicking)

# Simple voxel face clicking tests (without full app)
add_executable(test_voxel_face_clicking_simple
    test_voxel_face_clicking_simple.cpp
)

target_include_directories(test_voxel_face_clicking_simple PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_voxel_face_clicking_simple PRIVATE
    VoxelEditor_VisualFeedback
    VoxelEditor_VoxelData
    VoxelEditor_Logging
    GTest::gtest_main
)

target_compile_features(test_voxel_face_clicking_simple PRIVATE cxx_std_20)
gtest_discover_tests(test_voxel_face_clicking_simple)

# Mouse ray movement tests
add_executable(test_mouse_ray_movement
    test_mouse_ray_movement.cpp
)

target_include_directories(test_mouse_ray_movement PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_mouse_ray_movement PRIVATE
    VoxelEditor_CLI_Lib
    GTest::gtest_main
    glm::glm
)

target_compile_features(test_mouse_ray_movement PRIVATE cxx_std_20)
gtest_discover_tests(test_mouse_ray_movement)

