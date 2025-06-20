cmake_minimum_required(VERSION 3.10)

# Camera-Cube Visibility Tests (Google Test version)
add_executable(test_camera_cube_visibility
    test_camera_cube_visibility.cpp
)

target_link_libraries(test_camera_cube_visibility
    VoxelEditor_Camera
    VoxelEditor_VoxelData
    VoxelEditor_Math
    gtest
    gtest_main
)

# Add test
add_test(NAME CameraCubeVisibility COMMAND test_camera_cube_visibility)

# Camera-Cube Visibility Tests (Simple version)
add_executable(test_camera_cube_visibility_simple
    test_camera_cube_visibility_simple.cpp
)

target_link_libraries(test_camera_cube_visibility_simple
    VoxelEditor_Camera
    VoxelEditor_VoxelData
    VoxelEditor_Math
)

# Add test
add_test(NAME CameraCubeVisibilitySimple COMMAND test_camera_cube_visibility_simple)

# Ground Plane Voxel Placement Integration Test
add_executable(test_ground_plane_voxel_placement
    test_ground_plane_voxel_placement.cpp
)

target_link_libraries(test_ground_plane_voxel_placement
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
add_test(NAME GroundPlaneVoxelPlacement COMMAND test_ground_plane_voxel_placement)

# Mouse Ground Plane Clicking Integration Test
add_executable(test_mouse_ground_plane_clicking
    test_mouse_ground_plane_clicking.cpp
)

target_link_libraries(test_mouse_ground_plane_clicking
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
add_test(NAME MouseGroundPlaneClicking COMMAND test_mouse_ground_plane_clicking)

# Mouse Boundary Clicking Integration Test
add_executable(test_mouse_boundary_clicking
    test_mouse_boundary_clicking.cpp
)

target_link_libraries(test_mouse_boundary_clicking
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)

# Add test
add_test(NAME MouseBoundaryClicking COMMAND test_mouse_boundary_clicking)

# Workspace Boundary Placement Test
add_executable(test_workspace_boundary_placement
    test_workspace_boundary_placement.cpp
)

target_link_libraries(test_workspace_boundary_placement
    VoxelEditor_VoxelData
    VoxelEditor_Math
    VoxelEditor_Events
    VoxelEditor_Logging
    gtest
    gtest_main
)

# Add test
add_test(NAME WorkspaceBoundaryPlacement COMMAND test_workspace_boundary_placement)

# Visual Feedback Integration Tests (OpenGL-dependent)
add_executable(test_overlay_renderer_integration
    visual_feedback/test_overlay_renderer_integration.cpp
)

target_link_libraries(test_overlay_renderer_integration
    VoxelEditor_VisualFeedback
    VoxelEditor_Camera
    VoxelEditor_Math
    gtest
    gtest_main
)

add_test(NAME OverlayRendererIntegration COMMAND test_overlay_renderer_integration)

add_executable(test_feedback_renderer_integration
    visual_feedback/test_feedback_renderer_integration.cpp
)

target_link_libraries(test_feedback_renderer_integration
    VoxelEditor_VisualFeedback
    VoxelEditor_Rendering
    VoxelEditor_VoxelData
    VoxelEditor_Math
    gtest
    gtest_main
)

add_test(NAME FeedbackRendererIntegration COMMAND test_feedback_renderer_integration)

add_executable(test_visual_feedback_requirements_integration
    visual_feedback/test_visual_feedback_requirements_integration.cpp
)

target_link_libraries(test_visual_feedback_requirements_integration
    VoxelEditor_VisualFeedback
    VoxelEditor_VoxelData
    VoxelEditor_Math
    gtest
    gtest_main
)

add_test(NAME VisualFeedbackRequirementsIntegration COMMAND test_visual_feedback_requirements_integration)

# Enhanced Shader Validation Integration Test
add_executable(test_enhanced_shader_validation_integration
    rendering/test_enhanced_shader_validation_integration.cpp
)

target_link_libraries(test_enhanced_shader_validation_integration
    VoxelEditor_Rendering
    glfw
    glad
    OpenGL::GL
    gtest
    gtest_main
)

add_test(NAME EnhancedShaderValidationIntegration COMMAND test_enhanced_shader_validation_integration)

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
add_executable(test_shader_visual_validation
    rendering/test_shader_visual_validation.cpp
)

target_link_libraries(test_shader_visual_validation
    VoxelEditor_Rendering
    VoxelEditor_Math
    VoxelEditor_Logging
    glfw
    glad
    OpenGL::GL
    gtest
    gtest_main
)

add_test(NAME ShaderVisualValidation COMMAND test_shader_visual_validation)

# Comprehensive Shader Rendering Test
add_executable(test_shader_rendering_comprehensive
    rendering/test_shader_rendering_comprehensive.cpp
)

target_link_libraries(test_shader_rendering_comprehensive
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

add_test(NAME ShaderRenderingComprehensive COMMAND test_shader_rendering_comprehensive)

# OpenGL Wrapper Validation Test
add_executable(test_opengl_wrapper_validation
    rendering/test_opengl_wrapper_validation.cpp
)

target_link_libraries(test_opengl_wrapper_validation
    VoxelEditor_Rendering
    VoxelEditor_Math
    VoxelEditor_Logging
    glfw
    glad
    OpenGL::GL
    gtest
    gtest_main
)

add_test(NAME OpenGLWrapperValidation COMMAND test_opengl_wrapper_validation)

# VAO Attribute Validation Test
add_executable(test_vao_attribute_validation
    rendering/test_vao_attribute_validation.cpp
)

target_link_libraries(test_vao_attribute_validation
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

add_test(NAME VAOAttributeValidation COMMAND test_vao_attribute_validation)

# Set working directory for tests
set_tests_properties(CameraCubeVisibility CameraCubeVisibilitySimple GroundPlaneVoxelPlacement 
    MouseGroundPlaneClicking MouseBoundaryClicking WorkspaceBoundaryPlacement
    OverlayRendererIntegration FeedbackRendererIntegration VisualFeedbackRequirementsIntegration
    EnhancedShaderValidationIntegration ShaderVisualValidation
    ShaderRenderingComprehensive OpenGLWrapperValidation VAOAttributeValidation PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)