cmake_minimum_required(VERSION 3.10)

# Camera-Cube Visibility Tests (Google Test version)
add_executable(test_integration_core_camera_cube_visibility
    test_integration_core_camera_cube_visibility.cpp
)

target_link_libraries(test_integration_core_camera_cube_visibility
    VoxelEditor_Camera
    VoxelEditor_VoxelData
    VoxelEditor_Math
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
    VoxelEditor_Math
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
    VoxelEditor_Math
    VoxelEditor_Events
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
    VoxelEditor_Math
    VoxelEditor_Events
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
    VoxelEditor_Math
    VoxelEditor_Events
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
    VoxelEditor_Camera
    VoxelEditor_Math
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
    VoxelEditor_Rendering
    VoxelEditor_VoxelData
    VoxelEditor_Math
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
    VoxelEditor_VoxelData
    VoxelEditor_Math
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
    VoxelEditor_Math
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
    VoxelEditor_Camera
    VoxelEditor_Math
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
    VoxelEditor_Math
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
    VoxelEditor_Camera
    VoxelEditor_Math
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
    VoxelEditor_Camera
    VoxelEditor_Math
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
    PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Add rendering subdirectory for additional rendering tests
add_subdirectory(rendering)