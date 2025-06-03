find_package(GTest REQUIRED)

set(RENDERING_TEST_SOURCES
    test_RenderTypes.cpp
    test_RenderConfig.cpp
    test_RenderStats.cpp
    test_OpenGLRenderer.cpp
    test_RenderEngine.cpp
    test_ShaderManager.cpp
    # test_RenderState.cpp       # When implementation is added
)

add_executable(VoxelEditor_Rendering_Tests ${RENDERING_TEST_SOURCES})

# Link dependencies
target_link_libraries(VoxelEditor_Rendering_Tests
    VoxelEditor_Rendering
    VoxelEditor_Math
    VoxelEditor_Events
    VoxelEditor_Camera
    glad
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(VoxelEditor_Rendering_Tests PRIVATE cxx_std_20)

# Compiler-specific options
if(MSVC)
    target_compile_options(VoxelEditor_Rendering_Tests PRIVATE /W4)
else()
    target_compile_options(VoxelEditor_Rendering_Tests PRIVATE -Wall -Wextra -pedantic)
endif()

include(GoogleTest)
gtest_discover_tests(VoxelEditor_Rendering_Tests)