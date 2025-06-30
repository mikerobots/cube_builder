# TestHelpers.cmake
# Provides functions for automatically building test executables

# Function to automatically create test executables for all test_unit_*.cpp files
# Usage: create_unit_tests(TARGET_LINK_LIBRARIES library1 library2 ...)
function(create_unit_tests)
    # Parse arguments
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "" "TARGET_LINK_LIBRARIES")
    
    # Find GTest if not already found
    find_package(GTest REQUIRED)
    
    # Discover all test_unit_*.cpp files in current directory
    file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_unit_*.cpp")
    
    # Create executable for each test file
    foreach(test_source ${TEST_SOURCES})
        # Extract test name from file path
        get_filename_component(test_name ${test_source} NAME_WE)
        
        # Create executable
        add_executable(${test_name} ${test_source})
        
        # Link libraries
        target_link_libraries(${test_name}
            ${ARG_TARGET_LINK_LIBRARIES}
            GTest::gtest
            GTest::gtest_main
        )
        
        # Set C++ standard
        target_compile_features(${test_name} PRIVATE cxx_std_20)
        
        # Add to CTest
        include(GoogleTest)
        gtest_discover_tests(${test_name})
        
        # Set output directory to centralized bin directory
        set_target_properties(${test_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
        
        # Add to a global property to track all unit tests
        set_property(GLOBAL APPEND PROPERTY UNIT_TEST_TARGETS ${test_name})
    endforeach()
endfunction()

# Function to create integration tests (similar pattern)
function(create_integration_tests)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "" "TARGET_LINK_LIBRARIES")
    
    find_package(GTest REQUIRED)
    
    # Discover all test_integration_*.cpp files
    file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_integration_*.cpp")
    
    foreach(test_source ${TEST_SOURCES})
        get_filename_component(test_name ${test_source} NAME_WE)
        
        add_executable(${test_name} ${test_source})
        
        target_link_libraries(${test_name}
            ${ARG_TARGET_LINK_LIBRARIES}
            GTest::gtest
            GTest::gtest_main
        )
        
        target_compile_features(${test_name} PRIVATE cxx_std_20)
        
        include(GoogleTest)
        gtest_discover_tests(${test_name})
        
        set_target_properties(${test_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
        
        set_property(GLOBAL APPEND PROPERTY INTEGRATION_TEST_TARGETS ${test_name})
    endforeach()
endfunction()

# Function to list all unit test targets (useful for creating meta-targets)
function(get_all_unit_tests output_var)
    get_property(test_targets GLOBAL PROPERTY UNIT_TEST_TARGETS)
    set(${output_var} ${test_targets} PARENT_SCOPE)
endfunction()