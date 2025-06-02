cmake_minimum_required(VERSION 3.16)

# Find GTest
find_package(GTest REQUIRED)

# Test sources
# TODO: Add test source files when they are created
set(TEST_SOURCES
    # TestFileTypes.cpp
    # TestBinaryIO.cpp
    # TestProject.cpp
    # TestBinaryFormat.cpp
    # TestSTLExporter.cpp
    # TestCompression.cpp
    # TestFileVersioning.cpp
    # TestFileManager.cpp
)

# TODO: Uncomment the following when test source files are created
# # Create test executable
# add_executable(file_io_tests ${TEST_SOURCES})
# 
# # Link libraries
# target_link_libraries(file_io_tests
#     PRIVATE
#         file_io
#         voxel_data
#         groups
#         selection
#         camera
#         rendering
#         math
#         VoxelEditor_Events
#         memory
#         logging
#         GTest::GTest
#         GTest::Main
# )
# 
# # Include directories
# target_include_directories(file_io_tests
#     PRIVATE
#         ../include
#         ../../
#         ../../../
# )
# 
# # Add to test suite
# add_test(NAME file_io_unit_tests COMMAND file_io_tests)