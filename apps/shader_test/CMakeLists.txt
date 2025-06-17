cmake_minimum_required(VERSION 3.20)

# Shader Test Application
add_executable(ShaderTest
    src/main.cpp
    src/ShaderTestFramework.cpp
    src/ShaderTestRunner.cpp
    src/TestMeshGenerator.cpp
)

# Simple Shader Test Application
add_executable(SimpleShaderTest
    src/SimpleShaderTest.cpp
)

# Render Integration Test Application
add_executable(RenderIntegrationTest
    src/RenderIntegrationTest.cpp
    src/TestMeshGenerator.cpp
)

# Set output directory for RenderIntegrationTest
set_target_properties(RenderIntegrationTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy shader files to build directory for RenderIntegrationTest
add_custom_command(TARGET RenderIntegrationTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
        $<TARGET_FILE_DIR:RenderIntegrationTest>/core/rendering/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/core/rendering/shaders
        $<TARGET_FILE_DIR:RenderIntegrationTest>/core/rendering/shaders
    COMMENT "Copying shader files for RenderIntegrationTest"
)

# Create test output directory for RenderIntegrationTest
add_custom_command(TARGET RenderIntegrationTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
        $<TARGET_FILE_DIR:RenderIntegrationTest>/test_output
    COMMENT "Creating test output directory for RenderIntegrationTest"
)

# Geometric Shader Validation Test
add_executable(GeometricShaderValidation
    src/GeometricShaderValidation.cpp
    src/TestMeshGenerator.cpp
)

target_include_directories(GeometricShaderValidation PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(GeometricShaderValidation PRIVATE
    VoxelEditor_Rendering
    VoxelEditor_Camera
    VoxelEditor_Math
    VoxelEditor_Logging
    glad
    glfw
    ${OPENGL_LIBRARIES}
)

# Set output directory for GeometricShaderValidation
set_target_properties(GeometricShaderValidation PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy shader files to build directory for GeometricShaderValidation
add_custom_command(TARGET GeometricShaderValidation POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
        $<TARGET_FILE_DIR:GeometricShaderValidation>/core/rendering/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/core/rendering/shaders
        $<TARGET_FILE_DIR:GeometricShaderValidation>/core/rendering/shaders
    COMMENT "Copying shader files for GeometricShaderValidation"
)

# Create test output directory for GeometricShaderValidation
add_custom_command(TARGET GeometricShaderValidation POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
        $<TARGET_FILE_DIR:GeometricShaderValidation>/test_output
    COMMENT "Creating test output directory for GeometricShaderValidation"
)

target_include_directories(ShaderTest PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(ShaderTest PRIVATE
    VoxelEditor_Rendering
    VoxelEditor_Math
    VoxelEditor_Logging
    glad
    glfw
)

target_include_directories(SimpleShaderTest PRIVATE
    ${CMAKE_SOURCE_DIR}/external/glad/include
)

target_link_libraries(SimpleShaderTest PRIVATE
    glad
    glfw
    ${OPENGL_LIBRARIES}
)

target_include_directories(RenderIntegrationTest PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(RenderIntegrationTest PRIVATE
    VoxelEditor_Rendering
    VoxelEditor_Camera
    VoxelEditor_Math
    VoxelEditor_Logging
    glad
    glfw
    ${OPENGL_LIBRARIES}
)

# Set output directory
set_target_properties(ShaderTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(SimpleShaderTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy shader files to build directory for testing
add_custom_command(TARGET ShaderTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
        $<TARGET_FILE_DIR:ShaderTest>/core/rendering/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/core/rendering/shaders
        $<TARGET_FILE_DIR:ShaderTest>/core/rendering/shaders
    COMMENT "Copying shader files for testing"
)

# Create test output directory
add_custom_command(TARGET ShaderTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
        $<TARGET_FILE_DIR:ShaderTest>/test_output
    COMMENT "Creating test output directory"
)

# Copy shader files for SimpleShaderTest
add_custom_command(TARGET SimpleShaderTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
        $<TARGET_FILE_DIR:SimpleShaderTest>/core/rendering/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/core/rendering/shaders
        $<TARGET_FILE_DIR:SimpleShaderTest>/core/rendering/shaders
    COMMENT "Copying shader files for SimpleShaderTest"
)

# Add as a test
add_test(NAME ShaderValidation 
    COMMAND ShaderTest --all --report shader_test_report.txt
    WORKING_DIRECTORY $<TARGET_FILE_DIR:ShaderTest>
)

# Set test properties
set_tests_properties(ShaderValidation PROPERTIES
    LABELS "shader;rendering"
    TIMEOUT 60
)