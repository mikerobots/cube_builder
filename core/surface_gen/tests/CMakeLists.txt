# Include the test helpers
include(${CMAKE_SOURCE_DIR}/cmake/TestHelpers.cmake)

# Automatically create test executables for all test_unit_*.cpp files
create_unit_tests(TARGET_LINK_LIBRARIES VoxelEditor_SurfaceGen VoxelEditor_VoxelData VoxelEditor_Events)

# Automatically create test executables for all test_integration_*.cpp files
create_integration_tests(TARGET_LINK_LIBRARIES VoxelEditor_SurfaceGen VoxelEditor_VoxelData VoxelEditor_Events)