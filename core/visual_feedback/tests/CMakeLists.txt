cmake_minimum_required(VERSION 3.16)

# Find GTest
find_package(GTest REQUIRED)

# Test sources
set(TEST_SOURCES
    TestFeedbackTypes.cpp
    TestFaceDetector.cpp
    TestHighlightRenderer.cpp
    TestHighlightManager.cpp
    TestOutlineRenderer.cpp
    TestOverlayRenderer.cpp
    TestFeedbackRenderer.cpp
    TestPreviewManager.cpp
    TestOutlineRendererPreview.cpp
    test_VisualFeedback_requirements.cpp
)

# Create test executable
add_executable(visual_feedback_tests ${TEST_SOURCES})

# Link libraries
target_link_libraries(visual_feedback_tests
    PRIVATE
        VoxelEditor_VisualFeedback
        VoxelEditor_VoxelData
        VoxelEditor_Selection
        VoxelEditor_Camera
        VoxelEditor_Rendering
        VoxelEditor_Math
        VoxelEditor_Memory
        VoxelEditor_Events
        VoxelEditor_Logging
        VoxelEditor_Input
        GTest::GTest
        GTest::Main
)

# Include directories
target_include_directories(visual_feedback_tests
    PRIVATE
        ../include
        ../../
        ../../../
)

# Add to test suite
add_test(NAME visual_feedback_unit_tests COMMAND visual_feedback_tests)