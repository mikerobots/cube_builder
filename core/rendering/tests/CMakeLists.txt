find_package(GTest REQUIRED)

set(RENDERING_TEST_SOURCES
    test_RenderTypes.cpp
    test_RenderConfig.cpp
    test_RenderStats.cpp
    test_OpenGLRenderer.cpp
    test_RenderEngine.cpp
    test_ShaderManager.cpp
    test_ShaderAttributeBinding.cpp
    test_ShaderUniforms.cpp
    test_ShaderManagerLogging.cpp
    test_ShaderManagerSimple.cpp
    test_ShaderManagerDebug.cpp
    test_ShaderManagerFixed.cpp
    test_ShaderManagerSafe.cpp
    test_RenderIncremental.cpp
    test_RenderState.cpp
    test_EnhancedShaderValidation.cpp
    test_EdgeRendering.cpp
)

add_executable(VoxelEditor_Rendering_Tests ${RENDERING_TEST_SOURCES})

# Find GLFW for OpenGL context in tests
find_package(glfw3 QUIET)

# Link dependencies
target_link_libraries(VoxelEditor_Rendering_Tests
    VoxelEditor_Rendering
    VoxelEditor_Math
    VoxelEditor_Events
    VoxelEditor_Camera
    VoxelEditor_CLI_Lib  # For VoxelMeshGenerator
    VoxelEditor_SurfaceGen  # For SurfaceTypes
    glad
    GTest::gtest
    GTest::gtest_main
)

# Add GLFW if found (for OpenGL context tests)
if(glfw3_FOUND)
    target_link_libraries(VoxelEditor_Rendering_Tests glfw)
    target_compile_definitions(VoxelEditor_Rendering_Tests PRIVATE HAVE_GLFW)
endif()

target_compile_features(VoxelEditor_Rendering_Tests PRIVATE cxx_std_20)

# Compiler-specific options
if(MSVC)
    target_compile_options(VoxelEditor_Rendering_Tests PRIVATE /W4)
else()
    target_compile_options(VoxelEditor_Rendering_Tests PRIVATE -Wall -Wextra -pedantic)
endif()

include(GoogleTest)
gtest_discover_tests(VoxelEditor_Rendering_Tests)