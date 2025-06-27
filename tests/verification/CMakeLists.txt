# Core Functionality Verification Tests
find_package(GTest REQUIRED)

# Create verification test executable
add_executable(test_integration_verification_core_functionality
    test_integration_verification_core_functionality.cpp
)

# Link with all necessary libraries
target_link_libraries(test_integration_verification_core_functionality
    VoxelEditor_CLI_Lib  # CLI application library
    VoxelEditor_FileIO  # Top-level library that brings in most dependencies
    VoxelEditor_Config
    glad
    glfw
    GTest::gtest
    GTest::gtest_main
)

# Include directories
target_include_directories(test_integration_verification_core_functionality PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/apps/cli/include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

# Set C++ standard
target_compile_features(test_integration_verification_core_functionality PRIVATE cxx_std_20)

# Platform-specific settings
if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    target_link_libraries(test_integration_verification_core_functionality 
        ${COCOA_LIBRARY} 
        ${IOKIT_LIBRARY} 
        ${COREVIDEO_LIBRARY}
    )
endif()

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(test_integration_verification_core_functionality)