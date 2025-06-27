cmake_minimum_required(VERSION 3.16)

# Find GTest
find_package(GTest REQUIRED)

# List of test source files
set(FILE_IO_TEST_SOURCES
    test_unit_core_file_io_file_types.cpp
    test_unit_core_file_io_binary_io.cpp
    test_unit_core_file_io_project.cpp
    test_unit_core_file_io_binary_format.cpp
    test_unit_core_file_io_stl_exporter.cpp
    test_unit_core_file_io_compression.cpp
    test_unit_core_file_io_versioning.cpp
    test_unit_core_file_io_file_manager.cpp
    test_unit_core_file_io_requirements.cpp
)

# Create individual test executables for better granularity and auto-discovery
foreach(test_source ${FILE_IO_TEST_SOURCES})
    # Get test name without extension
    get_filename_component(test_name ${test_source} NAME_WE)
    
    # Create executable
    add_executable(${test_name} ${test_source})
    
    # Link libraries
    target_link_libraries(${test_name}
        PRIVATE
            VoxelEditor_FileIO
            # VoxelEditor_FileIO already transitively links these:
            # VoxelEditor_VoxelData, VoxelEditor_Groups, VoxelEditor_Selection, VoxelEditor_Rendering
            VoxelEditor_Camera
            VoxelEditor_Math
            VoxelEditor_Events
            VoxelEditor_Memory
            VoxelEditor_Logging
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
add_executable(test_unit_core_file_io ${FILE_IO_TEST_SOURCES})

# Link libraries
target_link_libraries(test_unit_core_file_io
    PRIVATE
        VoxelEditor_FileIO
        # VoxelEditor_FileIO already transitively links these:
        # VoxelEditor_VoxelData, VoxelEditor_Groups, VoxelEditor_Selection, VoxelEditor_Rendering
        VoxelEditor_Camera
        VoxelEditor_Math
        VoxelEditor_Events
        VoxelEditor_Memory
        VoxelEditor_Logging
        GTest::GTest
        GTest::Main
)

# Include directories
target_include_directories(test_unit_core_file_io
    PRIVATE
        ../include
        ../../
        ../../../
)

# Set C++ standard
target_compile_features(test_unit_core_file_io PRIVATE cxx_std_20)

# Set output directory
set_target_properties(test_unit_core_file_io PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)