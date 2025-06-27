cmake_minimum_required(VERSION 3.10)

# Shader Manager Integration Test
add_executable(test_integration_shader_manager
    test_integration_shader_manager.cpp
)

target_link_libraries(test_integration_shader_manager
    VoxelEditor_Rendering
    VoxelEditor_Logging
    gtest
    gtest_main
    glfw
    glad
    ${OPENGL_LIBRARIES}
)

# Add test
add_test(NAME test_integration_shader_manager COMMAND test_integration_shader_manager)
set_tests_properties(test_integration_shader_manager PROPERTIES
    LABELS "integration;rendering;shader;opengl"
)

# Camera-Cube Visibility Tests (Google Test version)
add_executable(test_integration_core_camera_cube_visibility
    test_integration_core_camera_cube_visibility.cpp
)

target_link_libraries(test_integration_core_camera_cube_visibility
    VoxelEditor_Camera
    VoxelEditor_VoxelData
    gtest
    gtest_main
)

# Add test
add_test(NAME test_integration_core_camera_cube_visibility COMMAND test_integration_core_camera_cube_visibility)
set_tests_properties(test_integration_core_camera_cube_visibility PROPERTIES
    LABELS "integration;core;camera"
)

# Camera-Cube Visibility Tests (Simple version)
add_executable(test_integration_core_camera_cube_visibility_simple
    test_integration_core_camera_cube_visibility_simple.cpp
)

target_link_libraries(test_integration_core_camera_cube_visibility_simple
    VoxelEditor_Camera
    VoxelEditor_VoxelData
)

# Add test
add_test(NAME test_integration_core_camera_cube_visibility_simple COMMAND test_integration_core_camera_cube_visibility_simple)
set_tests_properties(test_integration_core_camera_cube_visibility_simple PROPERTIES
    LABELS "integration;core;camera"
)

# Ground Plane Voxel Placement Integration Test
add_executable(test_integration_core_ground_plane_voxel_placement
    test_integration_core_ground_plane_voxel_placement.cpp
)

target_link_libraries(test_integration_core_ground_plane_voxel_placement
    VoxelEditor_VoxelData
    VoxelEditor_VisualFeedback
    VoxelEditor_Input
    VoxelEditor_UndoRedo
    VoxelEditor_Logging
    gtest
    gtest_main
)

# Add test
add_test(NAME test_integration_core_ground_plane_voxel_placement COMMAND test_integration_core_ground_plane_voxel_placement)
set_tests_properties(test_integration_core_ground_plane_voxel_placement PROPERTIES
    LABELS "integration;core;voxel"
)

# Mouse Ground Plane Clicking Integration Test
add_executable(test_integration_interaction_mouse_ground_plane_clicking
    test_integration_interaction_mouse_ground_plane_clicking.cpp
)

target_link_libraries(test_integration_interaction_mouse_ground_plane_clicking
    VoxelEditor_VoxelData
    VoxelEditor_VisualFeedback
    VoxelEditor_Input
    VoxelEditor_UndoRedo
    VoxelEditor_Camera
    VoxelEditor_Logging
    gtest
    gtest_main
)

# Add test
add_test(NAME test_integration_interaction_mouse_ground_plane_clicking COMMAND test_integration_interaction_mouse_ground_plane_clicking)
set_tests_properties(test_integration_interaction_mouse_ground_plane_clicking PROPERTIES
    LABELS "integration;interaction;mouse"
)

# Mouse Boundary Clicking Integration Test
add_executable(test_integration_interaction_mouse_boundary_clicking
    test_integration_interaction_mouse_boundary_clicking.cpp
)

target_link_libraries(test_integration_interaction_mouse_boundary_clicking
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)

# Add test
add_test(NAME test_integration_interaction_mouse_boundary_clicking COMMAND test_integration_interaction_mouse_boundary_clicking)
set_tests_properties(test_integration_interaction_mouse_boundary_clicking PROPERTIES
    LABELS "integration;interaction;mouse"
)

# Workspace Boundary Placement Test
add_executable(test_integration_core_workspace_boundary_placement
    test_integration_core_workspace_boundary_placement.cpp
)

target_link_libraries(test_integration_core_workspace_boundary_placement
    VoxelEditor_VoxelData
    VoxelEditor_Logging
    gtest
    gtest_main
)

# Add test
add_test(NAME test_integration_core_workspace_boundary_placement COMMAND test_integration_core_workspace_boundary_placement)
set_tests_properties(test_integration_core_workspace_boundary_placement PROPERTIES
    LABELS "integration;core;workspace"
)

# Visual Feedback Integration Tests (OpenGL-dependent)
add_executable(test_integration_feedback_overlay_renderer
    visual_feedback/test_integration_feedback_overlay_renderer.cpp
)

target_link_libraries(test_integration_feedback_overlay_renderer
    VoxelEditor_VisualFeedback
    VoxelEditor_Rendering
    glfw
    glad
    OpenGL::GL
    gtest
    gtest_main
)

add_test(NAME test_integration_feedback_overlay_renderer COMMAND test_integration_feedback_overlay_renderer)
set_tests_properties(test_integration_feedback_overlay_renderer PROPERTIES
    LABELS "integration;feedback;rendering"
)

add_executable(test_integration_feedback_renderer_components
    visual_feedback/test_integration_feedback_renderer_components.cpp
)

target_link_libraries(test_integration_feedback_renderer_components
    VoxelEditor_VisualFeedback
    gtest
    gtest_main
)

add_test(NAME test_integration_feedback_renderer_components COMMAND test_integration_feedback_renderer_components)
set_tests_properties(test_integration_feedback_renderer_components PROPERTIES
    LABELS "integration;feedback;rendering"
)

add_executable(test_integration_feedback_requirements_validation
    visual_feedback/test_integration_feedback_requirements_validation.cpp
)

target_link_libraries(test_integration_feedback_requirements_validation
    VoxelEditor_VisualFeedback
    glfw
    glad
    OpenGL::GL
    gtest
    gtest_main
)

add_test(NAME test_integration_feedback_requirements_validation COMMAND test_integration_feedback_requirements_validation)
set_tests_properties(test_integration_feedback_requirements_validation PROPERTIES
    LABELS "integration;feedback;validation"
)

# Enhanced Shader Validation Integration Test
add_executable(test_integration_rendering_enhanced_shader_validation
    rendering/test_integration_rendering_enhanced_shader_validation.cpp
)

target_link_libraries(test_integration_rendering_enhanced_shader_validation
    VoxelEditor_Rendering
    glfw
    glad
    OpenGL::GL
    gtest
    gtest_main
)

add_test(NAME test_integration_rendering_enhanced_shader_validation COMMAND test_integration_rendering_enhanced_shader_validation)
set_tests_properties(test_integration_rendering_enhanced_shader_validation PROPERTIES
    LABELS "integration;rendering;shader"
)

# Shader Pipeline Integration Test (VAO and complete rendering pipeline)
# DISABLED: These tests use low-level OpenGL functions not properly initialized
# and are superseded by test_shader_visual_validation
# add_executable(test_shader_pipeline_integration
#     rendering/test_shader_pipeline_integration.cpp
# )
# 
# target_link_libraries(test_shader_pipeline_integration
#     VoxelEditor_Rendering
#     glfw
#     glad
#     glm::glm
#     OpenGL::GL
#     gtest
#     gtest_main
# )
# 
# add_test(NAME ShaderPipelineIntegration COMMAND test_shader_pipeline_integration)

# Shader VAO Integration Test (Simplified VAO testing)
# DISABLED: These tests use low-level OpenGL functions not properly initialized
# and are superseded by test_shader_visual_validation
# add_executable(test_shader_vao_integration
#     rendering/test_shader_vao_integration.cpp
# )
# 
# target_link_libraries(test_shader_vao_integration
#     glfw
#     glad
#     glm::glm
#     OpenGL::GL
#     gtest
#     gtest_main
# )
# 
# add_test(NAME ShaderVAOIntegration COMMAND test_shader_vao_integration)

# Real Shader Pipeline Integration Test (Complete rendering pipeline validation)
# DISABLED: These tests have incorrect include paths and are superseded by test_shader_visual_validation
# add_executable(test_real_shader_pipeline
#     rendering/test_real_shader_pipeline.cpp
# )
# 
# target_link_libraries(test_real_shader_pipeline
#     VoxelEditor_Rendering
#     VoxelEditor_Camera
#     VoxelEditor_VoxelData
#     VoxelEditor_Math
#     VoxelEditor_Logging
#     glfw
#     glad
#     glm::glm
#     OpenGL::GL
#     gtest
#     gtest_main
# )
# 
# add_test(NAME RealShaderPipeline COMMAND test_real_shader_pipeline)

# Shader Real Usage Integration Test (Validates actual shader usage patterns)
# DISABLED: These tests have compilation errors and are superseded by test_shader_visual_validation
# add_executable(test_shader_real_usage
#     rendering/test_shader_real_usage.cpp
# )
# 
# target_link_libraries(test_shader_real_usage
#     VoxelEditor_Rendering
#     VoxelEditor_Camera
#     VoxelEditor_Math
#     VoxelEditor_Events
#     VoxelEditor_Logging
#     glfw
#     glad
#     OpenGL::GL
#     gtest
#     gtest_main
# )
# 
# add_test(NAME ShaderRealUsage COMMAND test_shader_real_usage)

# Shader Usage Validation Integration Test (Validates VAO and shader pipeline)
# DISABLED: These tests have compilation errors and are superseded by test_shader_visual_validation
# add_executable(test_shader_usage_validation
#     rendering/test_shader_usage_validation.cpp
# )
# 
# target_link_libraries(test_shader_usage_validation
#     VoxelEditor_Rendering
#     VoxelEditor_Math
#     VoxelEditor_Logging
#     glfw
#     glad
#     OpenGL::GL
#     gtest
#     gtest_main
# )
# 
# add_test(NAME ShaderUsageValidation COMMAND test_shader_usage_validation)

# Shader Visual Validation Integration Test (Captures and validates visual output)
add_executable(test_integration_visual_shader_output_validation
    rendering/test_integration_visual_shader_output_validation.cpp
)

target_link_libraries(test_integration_visual_shader_output_validation
    VoxelEditor_Rendering
    VoxelEditor_Logging
    glfw
    glad
    OpenGL::GL
    gtest
    gtest_main
)

add_test(NAME test_integration_visual_shader_output_validation COMMAND test_integration_visual_shader_output_validation)
set_tests_properties(test_integration_visual_shader_output_validation PROPERTIES
    LABELS "integration;visual;shader"
)

# Comprehensive Shader Rendering Test
add_executable(test_integration_rendering_shader_rendering_comprehensive
    rendering/test_integration_rendering_shader_rendering_comprehensive.cpp
)

target_link_libraries(test_integration_rendering_shader_rendering_comprehensive
    VoxelEditor_Rendering
    VoxelEditor_Logging
    glfw
    glad
    OpenGL::GL
    gtest
    gtest_main
)

add_test(NAME test_integration_rendering_shader_rendering_comprehensive COMMAND test_integration_rendering_shader_rendering_comprehensive)
set_tests_properties(test_integration_rendering_shader_rendering_comprehensive PROPERTIES
    LABELS "integration;rendering;shader"
)

# OpenGL Wrapper Validation Test
add_executable(test_integration_rendering_opengl_wrapper_validation
    rendering/test_integration_rendering_opengl_wrapper_validation.cpp
)

target_link_libraries(test_integration_rendering_opengl_wrapper_validation
    VoxelEditor_Rendering
    VoxelEditor_Logging
    glfw
    glad
    OpenGL::GL
    gtest
    gtest_main
)

add_test(NAME test_integration_rendering_opengl_wrapper_validation COMMAND test_integration_rendering_opengl_wrapper_validation)
set_tests_properties(test_integration_rendering_opengl_wrapper_validation PROPERTIES
    LABELS "integration;rendering;opengl"
)

# VAO Attribute Validation Test
add_executable(test_integration_rendering_vao_attribute_validation
    rendering/test_integration_rendering_vao_attribute_validation.cpp
)

target_link_libraries(test_integration_rendering_vao_attribute_validation
    VoxelEditor_Rendering
    VoxelEditor_Logging
    glfw
    glad
    OpenGL::GL
    gtest
    gtest_main
)

add_test(NAME test_integration_rendering_vao_attribute_validation COMMAND test_integration_rendering_vao_attribute_validation)
set_tests_properties(test_integration_rendering_vao_attribute_validation PROPERTIES
    LABELS "integration;rendering;vao"
)

# All Shaders Validation Test
add_executable(test_integration_shader_all_shaders_validation
    rendering/test_integration_shader_all_shaders_validation.cpp
)

target_link_libraries(test_integration_shader_all_shaders_validation
    VoxelEditor_Rendering
    VoxelEditor_Logging
    glfw
    glad
    OpenGL::GL
    gtest
    gtest_main
)

add_test(NAME test_integration_shader_all_shaders_validation COMMAND test_integration_shader_all_shaders_validation)
set_tests_properties(test_integration_shader_all_shaders_validation PROPERTIES
    LABELS "integration;shader;validation"
)

# Clear Command Integration Test
add_executable(test_integration_clear_command
    test_integration_clear_command.cpp
)

target_link_libraries(test_integration_clear_command
    VoxelEditor_VoxelData
    VoxelEditor_Events
    VoxelEditor_Math
    gtest
    gtest_main
)

add_test(NAME test_integration_clear_command COMMAND test_integration_clear_command)
set_tests_properties(test_integration_clear_command PROPERTIES
    LABELS "integration;core;voxel"
)

# Debug Integration Test
add_executable(test_integration_debug_192cm
    test_integration_debug_192cm.cpp
)

target_link_libraries(test_integration_debug_192cm
    VoxelEditor_VoxelData
    VoxelEditor_Camera
    VoxelEditor_Math
    VoxelEditor_Logging
    VoxelEditor_Input
    VoxelEditor_Events
    gtest
    gtest_main
)

add_test(NAME test_integration_debug_192cm COMMAND test_integration_debug_192cm)
set_tests_properties(test_integration_debug_192cm PROPERTIES
    LABELS "integration;debug;voxel"
)

# Ground Plane Constraint Integration Test
add_executable(test_integration_ground_plane_constraint
    test_integration_ground_plane_constraint.cpp
)

target_link_libraries(test_integration_ground_plane_constraint
    VoxelEditor_VoxelData
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)

# Add test
add_test(NAME test_integration_ground_plane_constraint COMMAND test_integration_ground_plane_constraint)
set_tests_properties(test_integration_ground_plane_constraint PROPERTIES
    LABELS "integration;core;constraint"
)

# Overlap Detection Integration Test
add_executable(test_integration_overlap_detection
    test_integration_overlap_detection.cpp
)

target_link_libraries(test_integration_overlap_detection
    VoxelEditor_VoxelData
    VoxelEditor_Events
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)

add_test(NAME test_integration_overlap_detection COMMAND test_integration_overlap_detection)
set_tests_properties(test_integration_overlap_detection PROPERTIES
    LABELS "integration;core;voxel"
)

# Coordinate Consistency Integration Test
add_executable(test_integration_coordinate_consistency
    test_integration_coordinate_consistency.cpp
)

target_link_libraries(test_integration_coordinate_consistency
    VoxelEditor_VoxelData
    VoxelEditor_Input
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)

add_test(NAME test_integration_coordinate_consistency COMMAND test_integration_coordinate_consistency)
set_tests_properties(test_integration_coordinate_consistency PROPERTIES
    LABELS "integration;core;coordinate"
)

# Preview Positioning Integration Test
add_executable(test_integration_preview_positioning
    test_integration_preview_positioning.cpp
)

target_link_libraries(test_integration_preview_positioning
    VoxelEditor_VoxelData
    VoxelEditor_VisualFeedback
    VoxelEditor_Input
    VoxelEditor_Math
    VoxelEditor_Events
    VoxelEditor_Logging
    glfw
    glad
    OpenGL::GL
    gtest
    gtest_main
)

add_test(NAME test_integration_preview_positioning COMMAND test_integration_preview_positioning)
set_tests_properties(test_integration_preview_positioning PROPERTIES
    LABELS "integration;preview;visual"
)

# Face-to-Face Alignment Integration Test (without snapping)
add_executable(test_integration_face_to_face_alignment
    test_integration_face_to_face_alignment.cpp
)

target_link_libraries(test_integration_face_to_face_alignment
    VoxelEditor_VoxelData
    VoxelEditor_VisualFeedback
    VoxelEditor_Input
    VoxelEditor_Math
    VoxelEditor_Events
    VoxelEditor_Logging
    gtest
    gtest_main
)

add_test(NAME test_integration_face_to_face_alignment COMMAND test_integration_face_to_face_alignment)
set_tests_properties(test_integration_face_to_face_alignment PROPERTIES
    LABELS "integration;alignment;voxel"
)

# Face Click Pipeline Integration Test
add_executable(test_integration_face_click_pipeline
    test_integration_face_click_pipeline.cpp
)

target_link_libraries(test_integration_face_click_pipeline
    VoxelEditor_VoxelData
    VoxelEditor_Camera
    VoxelEditor_VisualFeedback
    VoxelEditor_Input
    VoxelEditor_UndoRedo
    VoxelEditor_Math
    VoxelEditor_Events
    VoxelEditor_Logging
    gtest
    gtest_main
)

add_test(NAME test_integration_face_click_pipeline COMMAND test_integration_face_click_pipeline)
set_tests_properties(test_integration_face_click_pipeline PROPERTIES
    LABELS "integration;face_click;pipeline"
)

# Add rendering subdirectory for additional rendering tests
add_subdirectory(rendering)

# Set working directory for all integration tests  
set_tests_properties(
    test_integration_core_camera_cube_visibility
    test_integration_core_camera_cube_visibility_simple
    test_integration_core_ground_plane_voxel_placement
    test_integration_interaction_mouse_ground_plane_clicking
    test_integration_interaction_mouse_boundary_clicking
    test_integration_core_workspace_boundary_placement
    test_integration_feedback_overlay_renderer
    test_integration_feedback_renderer_components
    test_integration_feedback_requirements_validation
    test_integration_rendering_enhanced_shader_validation
    test_integration_visual_shader_output_validation
    test_integration_rendering_shader_rendering_comprehensive
    test_integration_rendering_opengl_wrapper_validation
    test_integration_rendering_vao_attribute_validation
    test_integration_shader_all_shaders_validation
    test_integration_clear_command
    test_integration_debug_192cm
    test_integration_ground_plane_constraint
    test_integration_overlap_detection
    test_integration_preview_positioning
    test_integration_face_to_face_alignment
    test_integration_face_click_pipeline
    test_integration_coordinate_consistency
    PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Face Detection Boundary Integration Test
add_executable(test_integration_face_detection_boundary
    test_integration_face_detection_boundary.cpp
)

target_link_libraries(test_integration_face_detection_boundary
    VoxelEditor_VisualFeedback
    VoxelEditor_VoxelData
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)

add_test(NAME test_integration_face_detection_boundary COMMAND test_integration_face_detection_boundary)
set_tests_properties(test_integration_face_detection_boundary PROPERTIES
    LABELS "integration;face_detection;boundary"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Preview Accuracy Integration Test
add_executable(test_integration_preview_accuracy
    test_integration_preview_accuracy.cpp
)

target_link_libraries(test_integration_preview_accuracy
    VoxelEditor_VoxelData
    VoxelEditor_VisualFeedback
    VoxelEditor_Camera
    VoxelEditor_UndoRedo
    VoxelEditor_Input
    VoxelEditor_Math
    VoxelEditor_Logging
    VoxelEditor_Events
    gtest
    gtest_main
)

add_test(NAME test_integration_preview_accuracy COMMAND test_integration_preview_accuracy)
set_tests_properties(test_integration_preview_accuracy PROPERTIES
    LABELS "integration;preview;accuracy"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Grid Overlay Integration Test
add_executable(test_integration_grid_overlay
    test_integration_grid_overlay.cpp
)

target_link_libraries(test_integration_grid_overlay
    VoxelEditor_CLI_Lib
    VoxelEditor_VoxelData
    VoxelEditor_VisualFeedback
    VoxelEditor_Rendering
    VoxelEditor_Camera
    VoxelEditor_Input
    VoxelEditor_Logging
    glfw
    glad
    OpenGL::GL
    glm::glm
    gtest
    gtest_main
    ${CMAKE_DL_LIBS}
)

target_compile_features(test_integration_grid_overlay PRIVATE cxx_std_20)

target_compile_definitions(test_integration_grid_overlay PRIVATE
    $<$<PLATFORM_ID:Darwin>:GL_SILENCE_DEPRECATION>
)

add_test(NAME test_integration_grid_overlay COMMAND test_integration_grid_overlay)
set_tests_properties(test_integration_grid_overlay PROPERTIES
    LABELS "integration;grid;overlay;visual"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)