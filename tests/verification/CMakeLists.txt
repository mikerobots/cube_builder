# Core Functionality Verification Tests
find_package(GTest REQUIRED)

# Create verification test executable
add_executable(CoreFunctionalityTests
    test_core_functionality.cpp
)

# Link with all necessary libraries
target_link_libraries(CoreFunctionalityTests
    VoxelEditor_CLI_Lib  # CLI application library
    VoxelEditor_VoxelData
    VoxelEditor_Rendering
    VoxelEditor_Camera
    VoxelEditor_Input
    VoxelEditor_Selection
    VoxelEditor_UndoRedo
    VoxelEditor_VisualFeedback
    VoxelEditor_Groups
    VoxelEditor_FileIO
    VoxelEditor_SurfaceGen
    VoxelEditor_Math
    VoxelEditor_Events
    VoxelEditor_Memory
    VoxelEditor_Logging
    VoxelEditor_Config
    glad
    glfw
    GTest::gtest
    GTest::gtest_main
)

# Include directories
target_include_directories(CoreFunctionalityTests PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/apps/cli/include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

# Set C++ standard
target_compile_features(CoreFunctionalityTests PRIVATE cxx_std_20)

# Platform-specific settings
if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    target_link_libraries(CoreFunctionalityTests 
        ${COCOA_LIBRARY} 
        ${IOKIT_LIBRARY} 
        ${COREVIDEO_LIBRARY}
    )
endif()

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(CoreFunctionalityTests)