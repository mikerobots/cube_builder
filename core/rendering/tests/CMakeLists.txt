find_package(GTest REQUIRED)

# List of test source files
set(RENDERING_TEST_SOURCES
    test_performance_core_rendering_engine.cpp
    test_stress_core_rendering_engine.cpp
    test_unit_core_rendering_config.cpp
    test_unit_core_rendering_edge_rendering.cpp
    test_unit_core_rendering_engine.cpp
    test_unit_core_rendering_enhanced_shader_validation.cpp
    test_unit_core_rendering_file_based_shader_validation.cpp
    test_unit_core_rendering_ground_plane_checkerboard.cpp
    test_unit_core_rendering_ground_plane_grid.cpp
    test_unit_core_rendering_ground_plane_grid_dynamics.cpp
    test_unit_core_rendering_ground_plane_shader_files.cpp
    test_unit_core_rendering_highlight_shader_validation.cpp
    test_unit_core_rendering_incremental.cpp
    test_unit_core_rendering_inline_shader_validation.cpp
    test_unit_core_rendering_opengl_renderer.cpp
    test_unit_core_rendering_requirements.cpp
    test_unit_core_rendering_shader_attribute_binding.cpp
    test_unit_core_rendering_shader_attribute_validation.cpp
    test_unit_core_rendering_shader_manager.cpp
    test_unit_core_rendering_shader_uniforms.cpp
    test_unit_core_rendering_shader_visual_validation.cpp
    test_unit_core_rendering_state.cpp
    test_unit_core_rendering_stats.cpp
    test_unit_core_rendering_types.cpp
)

# Find GLFW for OpenGL context in tests - use the fetched dependency
find_package(glfw3 QUIET)

# Create individual test executables for better granularity and auto-discovery
foreach(test_source ${RENDERING_TEST_SOURCES})
    # Get test name without extension
    get_filename_component(test_name ${test_source} NAME_WE)
    
    # Create executable
    add_executable(${test_name} ${test_source})
    
    # Link libraries
    target_link_libraries(${test_name}
        VoxelEditor_Rendering
        VoxelEditor_SurfaceGen  # For SurfaceTypes
        VoxelEditor_VisualFeedback  # For HighlightRenderer
        glad
        glfw  # Use the fetched GLFW target
        GTest::gtest
        GTest::gtest_main
    )
    
    # Add GLFW compile definition
    target_compile_definitions(${test_name} PRIVATE HAVE_GLFW)
    
    # Set C++ standard
    target_compile_features(${test_name} PRIVATE cxx_std_20)
    
    # Compiler-specific options
    if(MSVC)
        target_compile_options(${test_name} PRIVATE /W4)
    else()
        target_compile_options(${test_name} PRIVATE -Wall -Wextra -pedantic)
    endif()
    
    # Add to CTest
    include(GoogleTest)
    gtest_discover_tests(${test_name})
    
    # Set output directory
    set_target_properties(${test_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    
    # Copy shader files to build directory for this test
    add_custom_command(TARGET ${test_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory 
            $<TARGET_FILE_DIR:${test_name}>/core/rendering/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/core/rendering/shaders
            $<TARGET_FILE_DIR:${test_name}>/core/rendering/shaders
        COMMENT "Copying shader files for ${test_name}"
    )
endforeach()

# Also create combined test executable for backward compatibility
add_executable(VoxelEditor_Rendering_Tests ${RENDERING_TEST_SOURCES})

# Link dependencies
target_link_libraries(VoxelEditor_Rendering_Tests
    VoxelEditor_Rendering
    VoxelEditor_SurfaceGen  # For SurfaceTypes
    VoxelEditor_VisualFeedback  # For HighlightRenderer
    glad
    glfw  # Use the fetched GLFW target
    GTest::gtest
    GTest::gtest_main
)

# Add GLFW compile definition
target_compile_definitions(VoxelEditor_Rendering_Tests PRIVATE HAVE_GLFW)

target_compile_features(VoxelEditor_Rendering_Tests PRIVATE cxx_std_20)

# Compiler-specific options
if(MSVC)
    target_compile_options(VoxelEditor_Rendering_Tests PRIVATE /W4)
else()
    target_compile_options(VoxelEditor_Rendering_Tests PRIVATE -Wall -Wextra -pedantic)
endif()

include(GoogleTest)
gtest_discover_tests(VoxelEditor_Rendering_Tests)

# Set output directory
set_target_properties(VoxelEditor_Rendering_Tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Copy shader files to build directory for rendering tests
add_custom_command(TARGET VoxelEditor_Rendering_Tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
        $<TARGET_FILE_DIR:VoxelEditor_Rendering_Tests>/core/rendering/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/core/rendering/shaders
        $<TARGET_FILE_DIR:VoxelEditor_Rendering_Tests>/core/rendering/shaders
    COMMENT "Copying shader files for VoxelEditor_Rendering_Tests"
)