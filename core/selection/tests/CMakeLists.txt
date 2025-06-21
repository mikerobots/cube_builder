# Core/Selection Tests CMakeLists.txt

enable_testing()

# Find GTest
find_package(GTest REQUIRED)

# List of test source files
set(SELECTION_TEST_SOURCES
    test_unit_core_selection_types.cpp
    test_unit_core_selection_set.cpp
    test_unit_core_selection_manager.cpp
    test_unit_core_selection_box_selector.cpp
    test_unit_core_selection_sphere_selector.cpp
    test_unit_core_selection_flood_fill.cpp
    test_unit_core_selection_requirements.cpp
)

# Create individual test executables for better granularity and auto-discovery
foreach(test_source ${SELECTION_TEST_SOURCES})
    # Get test name without extension
    get_filename_component(test_name ${test_source} NAME_WE)
    
    # Create executable
    add_executable(${test_name} ${test_source})
    
    # Link libraries
    target_link_libraries(${test_name}
        VoxelEditor_Selection
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
add_executable(test_unit_core_selection ${SELECTION_TEST_SOURCES})

target_link_libraries(test_unit_core_selection
    VoxelEditor_Selection
    GTest::gtest
    GTest::gtest_main
)

# Set C++ standard
target_compile_features(test_unit_core_selection PRIVATE cxx_std_20)

# Add to CTest
include(GoogleTest)
gtest_discover_tests(test_unit_core_selection)

# Set output directory
set_target_properties(test_unit_core_selection PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Selection performance tests - also create individual executable
add_executable(test_performance_core_selection_manager test_performance_core_selection_manager.cpp)

target_link_libraries(test_performance_core_selection_manager
    VoxelEditor_Selection
    GTest::gtest
    GTest::gtest_main
)

# Set C++ standard
target_compile_features(test_performance_core_selection_manager PRIVATE cxx_std_20)

# Add to CTest
include(GoogleTest)
gtest_discover_tests(test_performance_core_selection_manager)

# Set output directory
set_target_properties(test_performance_core_selection_manager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Keep the old name for backward compatibility
add_executable(test_performance_core_selection test_performance_core_selection_manager.cpp)

target_link_libraries(test_performance_core_selection
    VoxelEditor_Selection
    GTest::gtest
    GTest::gtest_main
)

# Set C++ standard
target_compile_features(test_performance_core_selection PRIVATE cxx_std_20)

# Add tests
include(GoogleTest)
gtest_discover_tests(test_performance_core_selection)

# Set output directory
set_target_properties(test_performance_core_selection PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)