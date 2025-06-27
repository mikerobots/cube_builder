find_package(GTest REQUIRED)

# List of test source files
set(CAMERA_TEST_SOURCES
    test_unit_core_camera_camera.cpp
    test_unit_core_camera_controller.cpp
    test_unit_core_camera_orbit_camera.cpp
    test_unit_core_camera_orbit_transformations.cpp
    test_unit_core_camera_requirements.cpp
    test_unit_core_camera_set_distance.cpp
    test_unit_core_camera_viewport.cpp
    test_unit_core_camera_zoom_functionality.cpp
)

# Create individual test executables for better granularity and auto-discovery
foreach(test_source ${CAMERA_TEST_SOURCES})
    # Get test name without extension
    get_filename_component(test_name ${test_source} NAME_WE)
    
    # Create executable
    add_executable(${test_name} ${test_source})
    
    # Link libraries
    target_link_libraries(${test_name}
        VoxelEditor_Camera
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
add_executable(VoxelEditor_Camera_Tests ${CAMERA_TEST_SOURCES})

target_link_libraries(VoxelEditor_Camera_Tests
    VoxelEditor_Camera
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(VoxelEditor_Camera_Tests PRIVATE cxx_std_20)

set_target_properties(VoxelEditor_Camera_Tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)