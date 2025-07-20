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

add_executable(test_integration_feedback_shadow_placement
    visual_feedback/test_integration_feedback_shadow_placement.cpp
)

target_link_libraries(test_integration_feedback_shadow_placement
    VoxelEditor_VisualFeedback
    VoxelEditor_VoxelData
    VoxelEditor_Math
    VoxelEditor_Logging
    glfw
    glad
    OpenGL::GL
    gtest
    gtest_main
)

add_test(NAME test_integration_feedback_shadow_placement COMMAND test_integration_feedback_shadow_placement)
set_tests_properties(test_integration_feedback_shadow_placement PROPERTIES
    LABELS "integration;feedback;shadow"
)

# OutlineRenderer Integration Test
add_executable(test_integration_visual_feedback_outline_renderer
    visual_feedback/test_integration_visual_feedback_outline_renderer.cpp
)

target_link_libraries(test_integration_visual_feedback_outline_renderer
    VoxelEditor_VisualFeedback
    VoxelEditor_VoxelData
    VoxelEditor_Camera
    VoxelEditor_Math
    VoxelEditor_Logging
    glfw
    glad
    OpenGL::GL
    gtest
    gtest_main
)

add_test(NAME test_integration_visual_feedback_outline_renderer COMMAND test_integration_visual_feedback_outline_renderer)
set_tests_properties(test_integration_visual_feedback_outline_renderer PROPERTIES
    LABELS "integration;feedback;outline;rendering"
)

# OutlineRenderer Debug Test
add_executable(test_integration_visual_feedback_outline_renderer_debug
    visual_feedback/test_integration_visual_feedback_outline_renderer_debug.cpp
)

target_link_libraries(test_integration_visual_feedback_outline_renderer_debug
    VoxelEditor_VisualFeedback
    VoxelEditor_VoxelData
    VoxelEditor_Camera
    VoxelEditor_Math
    VoxelEditor_Logging
    glfw
    glad
    OpenGL::GL
    gtest
    gtest_main
)

add_test(NAME test_integration_visual_feedback_outline_renderer_debug COMMAND test_integration_visual_feedback_outline_renderer_debug)
set_tests_properties(test_integration_visual_feedback_outline_renderer_debug PROPERTIES
    LABELS "integration;feedback;outline;debug"
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

# Ground Plane Constraint Integration Test - DISABLED (missing source file)
# add_executable(test_integration_ground_plane_constraint
#     test_integration_ground_plane_constraint.cpp
# )
# 
# target_link_libraries(test_integration_ground_plane_constraint
#     VoxelEditor_VoxelData
#     VoxelEditor_Math
#     VoxelEditor_Logging
#     gtest
#     gtest_main
# )
# 
# # Add test
# add_test(NAME test_integration_ground_plane_constraint COMMAND test_integration_ground_plane_constraint)
# set_tests_properties(test_integration_ground_plane_constraint PROPERTIES
#     LABELS "integration;core;constraint"
# )

# Overlap Detection Integration Test - DISABLED (missing source file)
# add_executable(test_integration_overlap_detection
#     test_integration_overlap_detection.cpp
# )
# 
# target_link_libraries(test_integration_overlap_detection
#     VoxelEditor_VoxelData
#     VoxelEditor_Events
#     VoxelEditor_Math
#     VoxelEditor_Logging
#     gtest
#     gtest_main
# )
# 
# add_test(NAME test_integration_overlap_detection COMMAND test_integration_overlap_detection)
# set_tests_properties(test_integration_overlap_detection PROPERTIES
#     LABELS "integration;core;voxel"
# )

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
    test_integration_feedback_shadow_placement
    test_integration_visual_feedback_outline_renderer
    test_integration_visual_feedback_outline_renderer_debug
    test_integration_rendering_enhanced_shader_validation
    test_integration_visual_shader_output_validation
    test_integration_rendering_shader_rendering_comprehensive
    test_integration_rendering_opengl_wrapper_validation
    test_integration_rendering_vao_attribute_validation
    test_integration_shader_all_shaders_validation
    test_integration_clear_command
    test_integration_debug_192cm
    # test_integration_ground_plane_constraint  # DISABLED - missing source
    # test_integration_overlap_detection  # DISABLED - missing source
    test_integration_coordinate_consistency
    test_integration_preview_positioning
    test_integration_face_to_face_alignment
    test_integration_face_click_pipeline
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

# Overlay Rendering Position Test
add_executable(test_integration_overlay_rendering_positions
    test_integration_overlay_rendering_positions.cpp
)

target_link_libraries(test_integration_overlay_rendering_positions
    VoxelEditor_VisualFeedback
    VoxelEditor_Rendering
    VoxelEditor_Camera
    VoxelEditor_VoxelData
    VoxelEditor_Events
    VoxelEditor_Logging
    glfw
    glad
    OpenGL::GL
    gtest
    gtest_main
)

add_test(NAME test_integration_overlay_rendering_positions COMMAND test_integration_overlay_rendering_positions)
set_tests_properties(test_integration_overlay_rendering_positions PROPERTIES
    LABELS "integration;overlay;rendering;position"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# STL Export Integration Test
add_executable(test_integration_core_stl_export
    test_integration_core_stl_export.cpp
)

target_link_libraries(test_integration_core_stl_export
    VoxelEditor_VoxelData
    VoxelEditor_FileIO
    VoxelEditor_SurfaceGen
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)

add_test(NAME test_integration_core_stl_export COMMAND test_integration_core_stl_export)
set_tests_properties(test_integration_core_stl_export PROPERTIES
    LABELS "integration;core;stl;export"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Dual Contouring Adjacent Voxels Test
add_executable(test_integration_core_dual_contouring_adjacent
    test_integration_core_dual_contouring_adjacent.cpp
)

target_link_libraries(test_integration_core_dual_contouring_adjacent
    VoxelEditor_VoxelData
    VoxelEditor_SurfaceGen
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)

add_test(NAME test_integration_core_dual_contouring_adjacent COMMAND test_integration_core_dual_contouring_adjacent)
set_tests_properties(test_integration_core_dual_contouring_adjacent PROPERTIES
    LABELS "integration;core;dual_contouring;mesh"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Dual Contouring Vertex Validation Test
add_executable(test_integration_core_dual_contouring_vertex_validation
    test_integration_core_dual_contouring_vertex_validation.cpp
)

target_link_libraries(test_integration_core_dual_contouring_vertex_validation
    VoxelEditor_VoxelData
    VoxelEditor_SurfaceGen
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)

add_test(NAME test_integration_core_dual_contouring_vertex_validation COMMAND test_integration_core_dual_contouring_vertex_validation)
set_tests_properties(test_integration_core_dual_contouring_vertex_validation PROPERTIES
    LABELS "integration;core;dual_contouring;vertex_validation"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Dual Contouring Debug Test
add_executable(test_integration_core_dual_contouring_debug
    test_integration_core_dual_contouring_debug.cpp
)

target_link_libraries(test_integration_core_dual_contouring_debug
    VoxelEditor_VoxelData
    VoxelEditor_SurfaceGen
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)

add_test(NAME test_integration_core_dual_contouring_debug COMMAND test_integration_core_dual_contouring_debug)
set_tests_properties(test_integration_core_dual_contouring_debug PROPERTIES
    LABELS "integration;core;dual_contouring;debug"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Dual Contouring Simple Test
add_executable(test_integration_core_dual_contouring_simple
    test_integration_core_dual_contouring_simple.cpp
)

target_link_libraries(test_integration_core_dual_contouring_simple
    VoxelEditor_VoxelData
    VoxelEditor_SurfaceGen
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)

add_test(NAME test_integration_core_dual_contouring_simple COMMAND test_integration_core_dual_contouring_simple)
set_tests_properties(test_integration_core_dual_contouring_simple PROPERTIES
    LABELS "integration;core;dual_contouring;simple"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Dual Contouring Dense Test
add_executable(test_integration_core_dual_contouring_dense
    test_integration_core_dual_contouring_dense.cpp
)

target_link_libraries(test_integration_core_dual_contouring_dense
    VoxelEditor_VoxelData
    VoxelEditor_SurfaceGen
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)

add_test(NAME test_integration_core_dual_contouring_dense COMMAND test_integration_core_dual_contouring_dense)
set_tests_properties(test_integration_core_dual_contouring_dense PROPERTIES
    LABELS "integration;core;dual_contouring;dense"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# SimpleMesher Scene Integration Test
add_executable(test_integration_core_surface_gen_simple_mesher_scenes
    test_integration_core_surface_gen_simple_mesher_scenes.cpp
)

target_link_libraries(test_integration_core_surface_gen_simple_mesher_scenes
    VoxelEditor_VoxelData
    VoxelEditor_SurfaceGen
    VoxelEditor_FileIO
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)

add_test(NAME test_integration_core_surface_gen_simple_mesher_scenes COMMAND test_integration_core_surface_gen_simple_mesher_scenes)
set_tests_properties(test_integration_core_surface_gen_simple_mesher_scenes PROPERTIES
    LABELS "integration;core;surface_gen;simple_mesher"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# STL Export with SimpleMesher Test
add_executable(test_integration_core_stl_export_simple_mesher
    test_integration_core_stl_export_simple_mesher.cpp
)

target_link_libraries(test_integration_core_stl_export_simple_mesher
    VoxelEditor_VoxelData
    VoxelEditor_SurfaceGen
    VoxelEditor_FileIO
    VoxelEditor_Rendering
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)

add_test(NAME test_integration_core_stl_export_simple_mesher COMMAND test_integration_core_stl_export_simple_mesher)
set_tests_properties(test_integration_core_stl_export_simple_mesher PROPERTIES
    LABELS "integration;core;stl;export;simple_mesher"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# STL Export Voxel Alignment Test
add_executable(test_integration_core_stl_export_voxel_alignment
    test_integration_core_stl_export_voxel_alignment.cpp
)

target_link_libraries(test_integration_core_stl_export_voxel_alignment
    VoxelEditor_SurfaceGen
    VoxelEditor_VoxelData
    VoxelEditor_FileIO
    VoxelEditor_Rendering
    VoxelEditor_VoxelMath
    VoxelEditor_Events
    VoxelEditor_Logging
    gtest
    gtest_main
)

add_test(NAME test_integration_core_stl_export_voxel_alignment COMMAND test_integration_core_stl_export_voxel_alignment)
set_tests_properties(test_integration_core_stl_export_voxel_alignment PROPERTIES
    LABELS "integration;core;stl;export;alignment"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Multi-resolution STL export test
add_executable(test_integration_multiresolution_stl_export
    test_integration_multiresolution_stl_export.cpp
)
target_link_libraries(test_integration_multiresolution_stl_export
    VoxelEditor_VoxelData
    VoxelEditor_SurfaceGen
    VoxelEditor_FileIO
    VoxelEditor_Rendering
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)
add_test(NAME test_integration_multiresolution_stl_export COMMAND test_integration_multiresolution_stl_export)
set_tests_properties(test_integration_multiresolution_stl_export PROPERTIES
    LABELS "integration;core;stl;export;multiresolution"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# STL Voxel Position Accuracy Test
add_executable(test_integration_stl_voxel_position_accuracy
    test_integration_stl_voxel_position_accuracy.cpp
)
target_link_libraries(test_integration_stl_voxel_position_accuracy
    VoxelEditor_VoxelData
    VoxelEditor_SurfaceGen
    VoxelEditor_FileIO
    VoxelEditor_Rendering
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)
add_test(NAME test_integration_stl_voxel_position_accuracy COMMAND test_integration_stl_voxel_position_accuracy)
set_tests_properties(test_integration_stl_voxel_position_accuracy PROPERTIES
    LABELS "integration;core;stl;export;position"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Simple Mesh Debug Test
add_executable(test_simple_mesh_debug
    ../../test_simple_mesh_debug.cpp
)
target_link_libraries(test_simple_mesh_debug
    VoxelEditor_VoxelData
    VoxelEditor_SurfaceGen
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)
add_test(NAME test_simple_mesh_debug COMMAND test_simple_mesh_debug)
set_tests_properties(test_simple_mesh_debug PROPERTIES
    LABELS "debug;mesh;position"
)

# Multi-resolution Debug Test
add_executable(test_multi_resolution_debug
    ../../test_multi_resolution_debug.cpp
)
target_link_libraries(test_multi_resolution_debug
    VoxelEditor_VoxelData
    VoxelEditor_SurfaceGen
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)
add_test(NAME test_multi_resolution_debug COMMAND test_multi_resolution_debug)
set_tests_properties(test_multi_resolution_debug PROPERTIES
    LABELS "debug;mesh;multiresolution"
)

# Voxel Position Check Test
add_executable(test_voxel_position_check
    ../../test_voxel_position_check.cpp
)
target_link_libraries(test_voxel_position_check
    VoxelEditor_VoxelData
    VoxelEditor_SurfaceGen
    VoxelEditor_Math
    VoxelEditor_Logging
    gtest
    gtest_main
)
add_test(NAME test_voxel_position_check COMMAND test_voxel_position_check)
set_tests_properties(test_voxel_position_check PROPERTIES
    LABELS "debug;position;accuracy"
)