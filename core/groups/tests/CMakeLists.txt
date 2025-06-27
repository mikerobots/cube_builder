cmake_minimum_required(VERSION 3.16)

# Find GTest
find_package(GTest REQUIRED)

# List of test source files
set(GROUPS_TEST_SOURCES
    test_unit_core_groups_types.cpp
    test_unit_core_groups_voxel_group.cpp
    test_unit_core_groups_hierarchy.cpp
    test_unit_core_groups_operations.cpp
    test_unit_core_groups_manager.cpp
    test_unit_core_groups_requirements.cpp
)

# Create individual test executables for better granularity and auto-discovery
foreach(test_source ${GROUPS_TEST_SOURCES})
    # Get test name without extension
    get_filename_component(test_name ${test_source} NAME_WE)
    
    # Create executable
    add_executable(${test_name} ${test_source})
    
    # Link libraries
    target_link_libraries(${test_name}
        PRIVATE
            VoxelEditor_Groups
            GTest::GTest
            GTest::Main
    )
    
    # Include directories
    target_include_directories(${test_name}
        PRIVATE
            ../include
            ../../
            ../../../
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
add_executable(test_unit_core_groups ${GROUPS_TEST_SOURCES})

# Link libraries
target_link_libraries(test_unit_core_groups
    PRIVATE
        VoxelEditor_Groups
        GTest::GTest
        GTest::Main
)

# Include directories
target_include_directories(test_unit_core_groups
    PRIVATE
        ../include
        ../../
        ../../../
)

# Set C++ standard
target_compile_features(test_unit_core_groups PRIVATE cxx_std_20)

# Set output directory
set_target_properties(test_unit_core_groups PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)