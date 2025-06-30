# Include the test helpers
include(${CMAKE_SOURCE_DIR}/cmake/TestHelpers.cmake)

# Automatically create test executables for all test_unit_*.cpp files
create_unit_tests(TARGET_LINK_LIBRARIES VoxelEditor_Groups)
