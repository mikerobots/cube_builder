# Rendering Integration Tests

# Visual validation test executable
add_executable(test_integration_shader_comprehensive_validation
    test_integration_shader_comprehensive_validation.cpp
)

target_include_directories(test_integration_shader_comprehensive_validation PRIVATE
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_integration_shader_comprehensive_validation PRIVATE
    VoxelEditor_Rendering
    VoxelEditor_Math
    VoxelEditor_Logging
    OpenGL::GL
    glfw
    glad
    glm::glm
    GTest::gtest_main
    ${CMAKE_DL_LIBS}
)

# Enable required features
target_compile_features(test_integration_shader_comprehensive_validation PRIVATE cxx_std_20)

# Add test
gtest_discover_tests(test_integration_shader_comprehensive_validation)

# Pixel Validation Helpers Test
add_executable(test_integration_visual_pixel_validation_helpers
    test_integration_visual_pixel_validation_helpers.cpp
)

target_include_directories(test_integration_visual_pixel_validation_helpers PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_integration_visual_pixel_validation_helpers PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# Enable required features
target_compile_features(test_integration_visual_pixel_validation_helpers PRIVATE cxx_std_17)

# Add test
gtest_discover_tests(test_integration_visual_pixel_validation_helpers)

# Multi-Mesh Scene Validation Test
add_executable(test_integration_rendering_multi_mesh_scene_validation
    test_integration_rendering_multi_mesh_scene_validation.cpp
)

target_include_directories(test_integration_rendering_multi_mesh_scene_validation PRIVATE
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_integration_rendering_multi_mesh_scene_validation PRIVATE
    VoxelEditor_Rendering
    VoxelEditor_Math
    OpenGL::GL
    glfw
    glad
    glm::glm
    GTest::gtest_main
)

# Enable required features
target_compile_features(test_integration_rendering_multi_mesh_scene_validation PRIVATE cxx_std_20)

# Add test
gtest_discover_tests(test_integration_rendering_multi_mesh_scene_validation)

# Shader Uniform Validation Test
add_executable(test_integration_shader_uniform_validation
    test_integration_shader_uniform_validation.cpp
)

target_include_directories(test_integration_shader_uniform_validation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_integration_shader_uniform_validation PRIVATE
    VoxelEditor_Rendering
    VoxelEditor_Math
    VoxelEditor_Logging
    OpenGL::GL
    glfw
    glad
    glm::glm
    GTest::gtest_main
)

# Enable required features
target_compile_features(test_integration_shader_uniform_validation PRIVATE cxx_std_20)

# Add test
gtest_discover_tests(test_integration_shader_uniform_validation)

# Shader Performance Benchmark Test
add_executable(test_performance_rendering_shader_benchmark
    test_performance_rendering_shader_benchmark.cpp
)

target_include_directories(test_performance_rendering_shader_benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_performance_rendering_shader_benchmark PRIVATE
    VoxelEditor_Rendering
    VoxelEditor_Math
    VoxelEditor_Logging
    OpenGL::GL
    glfw
    glad
    glm::glm
    GTest::gtest_main
)

# Enable required features
target_compile_features(test_performance_rendering_shader_benchmark PRIVATE cxx_std_20)

# Add test
gtest_discover_tests(test_performance_rendering_shader_benchmark)

# Reference Image Comparison Test
add_executable(test_integration_visual_reference_image_comparison
    test_integration_visual_reference_image_comparison.cpp
)

target_include_directories(test_integration_visual_reference_image_comparison PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_integration_visual_reference_image_comparison PRIVATE
    VoxelEditor_Rendering
    VoxelEditor_Math
    VoxelEditor_Logging
    OpenGL::GL
    glfw
    glad
    glm::glm
    GTest::gtest_main
)

# Enable required features
target_compile_features(test_integration_visual_reference_image_comparison PRIVATE cxx_std_20)

# Add test
gtest_discover_tests(test_integration_visual_reference_image_comparison)

# Shader Lighting Validation Test
add_executable(test_integration_shader_lighting_validation
    test_integration_shader_lighting_validation.cpp
)

target_include_directories(test_integration_shader_lighting_validation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_integration_shader_lighting_validation PRIVATE
    VoxelEditor_Rendering
    VoxelEditor_Math
    VoxelEditor_Logging
    OpenGL::GL
    glfw
    glad
    glm::glm
    GTest::gtest_main
)

target_compile_features(test_integration_shader_lighting_validation PRIVATE cxx_std_20)
gtest_discover_tests(test_integration_shader_lighting_validation)

# Shader Validation Comprehensive Test
add_executable(test_integration_shader_validation_comprehensive
    test_integration_shader_validation_comprehensive.cpp
)

target_include_directories(test_integration_shader_validation_comprehensive PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_integration_shader_validation_comprehensive PRIVATE
    VoxelEditor_Rendering
    VoxelEditor_Math
    VoxelEditor_Logging
    OpenGL::GL
    glfw
    glad
    glm::glm
    GTest::gtest_main
)

target_compile_features(test_integration_shader_validation_comprehensive PRIVATE cxx_std_20)
gtest_discover_tests(test_integration_shader_validation_comprehensive)