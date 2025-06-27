# Find GTest package
find_package(GTest REQUIRED)

# List of test source files
set(UNDO_REDO_TEST_SOURCES
    test_unit_core_undo_redo_command.cpp
    test_unit_core_undo_redo_simple_command.cpp
    test_unit_core_undo_redo_history_manager.cpp
    test_unit_core_undo_redo_placement_commands.cpp
    test_unit_core_undo_redo_requirements.cpp
    test_unit_core_undo_redo_voxel_commands.cpp
)

# Create individual test executables for better granularity and auto-discovery
foreach(test_source ${UNDO_REDO_TEST_SOURCES})
    # Get test name without extension
    get_filename_component(test_name ${test_source} NAME_WE)
    
    # Create executable
    add_executable(${test_name} ${test_source})
    
    # Link libraries
    target_link_libraries(${test_name}
        PRIVATE
            VoxelEditor_UndoRedo
            VoxelEditor_VoxelData
            VoxelEditor_Events
            VoxelEditor_Logging
            GTest::gtest_main
            GTest::gmock
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

# Note: Combined test executable disabled due to test conflicts
# Individual test executables are preferred for better isolation