cmake_minimum_required(VERSION 3.16)

# Find GTest
find_package(GTest REQUIRED)

# Test sources
set(TEST_SOURCES
    TestFileTypes.cpp
    TestBinaryIO.cpp
    TestProject.cpp
    TestBinaryFormat.cpp
    TestSTLExporter.cpp
    TestCompression.cpp
    TestFileVersioning.cpp
    TestFileManager.cpp
    test_FileIO_requirements.cpp
)

# Create test executable
add_executable(file_io_tests ${TEST_SOURCES})

# Link libraries
target_link_libraries(file_io_tests
    PRIVATE
        VoxelEditor_FileIO
        VoxelEditor_VoxelData
        VoxelEditor_Groups
        VoxelEditor_Selection
        VoxelEditor_Camera
        VoxelEditor_Rendering
        VoxelEditor_Math
        VoxelEditor_Events
        VoxelEditor_Memory
        VoxelEditor_Logging
        GTest::GTest
        GTest::Main
)

# Include directories
target_include_directories(file_io_tests
    PRIVATE
        ../include
        ../../
        ../../../
)

# Add to test suite
add_test(NAME file_io_unit_tests COMMAND file_io_tests)