find_package(GTest REQUIRED)

# List of performance test source files
set(PERFORMANCE_TEST_SOURCES
    test_performance_memory_usage_validation.cpp
)

# Create individual test executables for better granularity and auto-discovery
foreach(test_source ${PERFORMANCE_TEST_SOURCES})
    # Get test name without extension
    get_filename_component(test_name ${test_source} NAME_WE)
    
    # Create executable
    add_executable(${test_name} ${test_source})
    
    # Link libraries
    target_link_libraries(${test_name}
        VoxelEditor_VoxelData
        VoxelEditor_UndoRedo
        GTest::gtest
        GTest::gtest_main
    )
    
    # Set C++ standard
    target_compile_features(${test_name} PRIVATE cxx_std_17)
    
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
add_executable(VoxelEditor_Performance_Tests ${PERFORMANCE_TEST_SOURCES})

# Link dependencies
target_link_libraries(VoxelEditor_Performance_Tests
    VoxelEditor_VoxelData
    VoxelEditor_UndoRedo
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(VoxelEditor_Performance_Tests PRIVATE cxx_std_17)

# Compiler-specific options
if(MSVC)
    target_compile_options(VoxelEditor_Performance_Tests PRIVATE /W4)
else()
    target_compile_options(VoxelEditor_Performance_Tests PRIVATE -Wall -Wextra -pedantic)
endif()

include(GoogleTest)
gtest_discover_tests(VoxelEditor_Performance_Tests)

# Set output directory
set_target_properties(VoxelEditor_Performance_Tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)