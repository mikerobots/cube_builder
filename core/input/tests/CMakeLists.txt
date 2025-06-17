find_package(GTest REQUIRED)

set(INPUT_TEST_SOURCES
    test_InputTypes.cpp
    test_InputMapping.cpp
    test_MouseHandler.cpp
    test_KeyboardHandler.cpp
    test_TouchHandler.cpp
    test_VRInputHandler.cpp
    test_PlacementValidation.cpp
    test_ModifierKeyTracking.cpp
    test_PlaneDetector.cpp
    # test_InputManager.cpp     # When full implementation is added
)

add_executable(VoxelEditor_Input_Tests ${INPUT_TEST_SOURCES})

# Link dependencies
target_link_libraries(VoxelEditor_Input_Tests
    VoxelEditor_Input
    VoxelEditor_Math
    VoxelEditor_Events
    VoxelEditor_Camera
    VoxelEditor_VoxelData
    VoxelEditor_VisualFeedback
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(VoxelEditor_Input_Tests PRIVATE cxx_std_20)

# Compiler-specific options
if(MSVC)
    target_compile_options(VoxelEditor_Input_Tests PRIVATE /W4)
else()
    target_compile_options(VoxelEditor_Input_Tests PRIVATE -Wall -Wextra -pedantic)
endif()

include(GoogleTest)
gtest_discover_tests(VoxelEditor_Input_Tests)