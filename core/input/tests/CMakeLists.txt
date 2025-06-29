find_package(GTest REQUIRED)

# Auto-discover test files
file(GLOB INPUT_TEST_SOURCES 
    "test_unit_core_input_*.cpp"
    "test_unit_placement_*.cpp"
    "test_integration_placement_*.cpp"
)

# Create individual test executables for better granularity and auto-discovery
foreach(test_source ${INPUT_TEST_SOURCES})
    # Get test name without extension
    get_filename_component(test_name ${test_source} NAME_WE)
    
    # Create executable
    add_executable(${test_name} ${test_source})
    
    # Link libraries
    target_link_libraries(${test_name}
        VoxelEditor_Input
        VoxelEditor_Camera
        VoxelEditor_VoxelData
        VoxelEditor_VisualFeedback
        GTest::gtest
        GTest::gtest_main
    )
    
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
endforeach()

# Also create combined test executable for backward compatibility
# Only create if we have source files
if(INPUT_TEST_SOURCES)
    add_executable(VoxelEditor_Input_Tests ${INPUT_TEST_SOURCES})

# Link dependencies
target_link_libraries(VoxelEditor_Input_Tests
    VoxelEditor_Input
    VoxelEditor_Camera
    VoxelEditor_VoxelData
    VoxelEditor_VisualFeedback
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(VoxelEditor_Input_Tests PRIVATE cxx_std_20)

# Compiler-specific options
if(MSVC)
    target_compile_options(VoxelEditor_Input_Tests PRIVATE /W4)
else()
    target_compile_options(VoxelEditor_Input_Tests PRIVATE -Wall -Wextra -pedantic)
endif()

include(GoogleTest)
gtest_discover_tests(VoxelEditor_Input_Tests)

# Set output directory
set_target_properties(VoxelEditor_Input_Tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

endif() # if(INPUT_TEST_SOURCES)