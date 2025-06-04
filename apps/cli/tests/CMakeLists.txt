find_package(GTest REQUIRED)

# CLI Integration Tests
set(TEST_SOURCES
    integration_test.cpp
    test_cli_headless.cpp
    test_cli_commands.cpp
    test_cli_error_handling.cpp
    test_cli_rendering_basic.cpp
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