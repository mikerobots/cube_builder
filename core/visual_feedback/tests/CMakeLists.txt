find_package(GTest REQUIRED)

# List of test source files
set(VISUAL_FEEDBACK_TEST_SOURCES
    test_unit_core_visual_feedback_face_detector.cpp
    test_unit_core_visual_feedback_highlight_manager.cpp
    test_unit_core_visual_feedback_highlight_renderer.cpp
    test_unit_core_visual_feedback_outline_renderer.cpp
    test_unit_core_visual_feedback_outline_renderer_preview.cpp
    test_unit_core_visual_feedback_overlay_renderer.cpp
    test_unit_core_visual_feedback_preview_manager.cpp
    test_unit_core_visual_feedback_renderer.cpp
    test_unit_core_visual_feedback_requirements.cpp
    test_unit_core_visual_feedback_types.cpp
)

# Create individual test executables for better granularity and auto-discovery
foreach(test_source ${VISUAL_FEEDBACK_TEST_SOURCES})
    # Get test name without extension
    get_filename_component(test_name ${test_source} NAME_WE)
    
    # Create executable
    add_executable(${test_name} ${test_source})
    
    # Link libraries
    target_link_libraries(${test_name}
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
        GTest::gtest
        GTest::gtest_main
    )
    
    # Set C++ standard
    target_compile_features(${test_name} PRIVATE cxx_std_20)
    
    # Add to CTest
    include(GoogleTest)
    gtest_discover_tests(${test_name})
    
    # Set output directory
    set_target_properties(${test_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endforeach()

# Also create combined test executable for backward compatibility
add_executable(VoxelEditor_VisualFeedback_Tests ${VISUAL_FEEDBACK_TEST_SOURCES})

target_link_libraries(VoxelEditor_VisualFeedback_Tests
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
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(VoxelEditor_VisualFeedback_Tests PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(VoxelEditor_VisualFeedback_Tests)

# Set output directory
set_target_properties(VoxelEditor_VisualFeedback_Tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)