find_package(GTest REQUIRED)

# List of test source files
set(SURFACE_GEN_TEST_SOURCES
    test_unit_core_surface_gen_dual_contouring.cpp
    test_unit_core_surface_gen_generator.cpp
    test_unit_core_surface_gen_generator_optimized.cpp
    test_unit_core_surface_gen_mesh_builder.cpp
    test_unit_core_surface_gen_mesh_smoother.cpp
    test_unit_core_surface_gen_topology_preserver.cpp
    test_unit_core_surface_gen_mesh_validator.cpp
    test_unit_core_surface_gen_performance.cpp
    test_unit_core_surface_gen_requirements.cpp
    test_unit_core_surface_gen_settings.cpp
    test_unit_core_surface_gen_types.cpp
    test_unit_core_surface_gen_preview_simple.cpp
    test_integration_surface_gen_mock.cpp
)

# Create individual test executables for better granularity and auto-discovery
foreach(test_source ${SURFACE_GEN_TEST_SOURCES})
    # Get test name without extension
    get_filename_component(test_name ${test_source} NAME_WE)
    
    # Create executable
    add_executable(${test_name} ${test_source})
    
    # Link libraries
    target_link_libraries(${test_name}
        VoxelEditor_SurfaceGen
        VoxelEditor_VoxelData
        VoxelEditor_Events
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
add_executable(VoxelEditor_SurfaceGen_Tests ${SURFACE_GEN_TEST_SOURCES})

target_link_libraries(VoxelEditor_SurfaceGen_Tests
    VoxelEditor_SurfaceGen
    VoxelEditor_VoxelData
    VoxelEditor_Events
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(VoxelEditor_SurfaceGen_Tests PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(VoxelEditor_SurfaceGen_Tests)

# Set output directory
set_target_properties(VoxelEditor_SurfaceGen_Tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)