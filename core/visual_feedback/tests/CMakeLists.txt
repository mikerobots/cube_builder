cmake_minimum_required(VERSION 3.16)

# Find GTest
find_package(GTest REQUIRED)

# Test sources
set(TEST_SOURCES
    TestFeedbackTypes.cpp
    TestFaceDetector.cpp
    TestHighlightRenderer.cpp
    TestOutlineRenderer.cpp
    TestOverlayRenderer.cpp
    TestFeedbackRenderer.cpp
)

# Create test executable
add_executable(visual_feedback_tests ${TEST_SOURCES})

# Link libraries
target_link_libraries(visual_feedback_tests
    PRIVATE
        visual_feedback
        voxel_data
        selection
        camera
        rendering
        math
        memory
        events
        logging
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