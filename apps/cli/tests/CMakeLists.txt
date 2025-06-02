cmake_minimum_required(VERSION 3.10)

# CLI Integration Tests
set(TEST_SOURCES
    integration_test.cpp
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
    # Core libraries
    VoxelEditor_VoxelData
    VoxelEditor_Camera
    VoxelEditor_Rendering
    VoxelEditor_Input
    VoxelEditor_Selection
    VoxelEditor_UndoRedo
    VoxelEditor_SurfaceGen
    VoxelEditor_VisualFeedback
    VoxelEditor_Groups
    VoxelEditor_FileIO
    
    # Foundation libraries
    VoxelEditor_Events
    VoxelEditor_Logging
    VoxelEditor_Config
    VoxelEditor_Math
    VoxelEditor_Memory
    
    # Test framework
    GTest::gtest
    GTest::gtest_main
    
    # External dependencies
    glm::glm
)

# Add test
add_test(NAME VoxelEditor_CLI_Tests COMMAND VoxelEditor_CLI_Tests)

# Enable C++20
target_compile_features(VoxelEditor_CLI_Tests PRIVATE cxx_std_20)