find_package(GTest REQUIRED)

# List of test source files
set(VOXEL_DATA_TEST_SOURCES
    test_unit_core_voxel_data_batch_operations.cpp
    test_unit_core_voxel_data_collision_simple.cpp
    test_unit_core_voxel_data_extent_validation.cpp
    test_unit_core_voxel_data_grid.cpp
    test_unit_core_voxel_data_manager.cpp
    test_unit_core_voxel_data_position_validation.cpp
    test_unit_core_voxel_data_region_operations.cpp
    test_unit_core_voxel_data_requirements.cpp
    test_unit_core_voxel_data_sparse_octree.cpp
    test_unit_core_voxel_data_types.cpp
    test_unit_core_voxel_data_workspace_manager.cpp
)

# List of performance test source files
set(VOXEL_DATA_PERF_TEST_SOURCES
    test_uperf_core_voxel_data_manager.cpp
    test_uperf_core_voxel_data_requirements.cpp
)

# Create individual test executables for better granularity and auto-discovery
foreach(test_source ${VOXEL_DATA_TEST_SOURCES})
    # Get test name without extension
    get_filename_component(test_name ${test_source} NAME_WE)
    
    # Create executable
    add_executable(${test_name} ${test_source})
    
    # Link libraries
    target_link_libraries(${test_name}
        VoxelEditor_VoxelData
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

# Create individual performance test executables
foreach(test_source ${VOXEL_DATA_PERF_TEST_SOURCES})
    # Get test name without extension
    get_filename_component(test_name ${test_source} NAME_WE)
    
    # Create executable
    add_executable(${test_name} ${test_source})
    
    # Link libraries
    target_link_libraries(${test_name}
        VoxelEditor_VoxelData
        GTest::gtest
        GTest::gtest_main
    )
    
    # Set C++ standard
    target_compile_features(${test_name} PRIVATE cxx_std_20)
    
    # Add to CTest with longer timeout
    include(GoogleTest)
    gtest_discover_tests(${test_name} PROPERTIES TIMEOUT 300)
    
    # Set output directory
    set_target_properties(${test_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endforeach()

# Also create combined test executable for backward compatibility
add_executable(VoxelEditor_VoxelData_Tests ${VOXEL_DATA_TEST_SOURCES})

target_link_libraries(VoxelEditor_VoxelData_Tests
    VoxelEditor_VoxelData
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(VoxelEditor_VoxelData_Tests PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(VoxelEditor_VoxelData_Tests)

# Set output directory
set_target_properties(VoxelEditor_VoxelData_Tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)