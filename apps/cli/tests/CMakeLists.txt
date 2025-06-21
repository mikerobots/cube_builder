find_package(GTest REQUIRED)

# Auto-discover CLI and interaction integration test files
file(GLOB CLI_INTEGRATION_TEST_FILES "test_integration_cli_*.cpp" "test_integration_interaction_*.cpp")

# Function to determine appropriate libraries for each test
function(get_test_libraries TEST_NAME OUT_LIBRARIES)
    # Default libraries for all tests
    set(LIBS
        VoxelEditor_CLI_Lib
        GTest::gtest_main
    )
    
    # Add specific libraries based on test name patterns
    if(${TEST_NAME} MATCHES ".*face_clicking.*" OR 
       ${TEST_NAME} MATCHES ".*click_voxel.*" OR
       ${TEST_NAME} MATCHES ".*voxel_face.*")
        list(APPEND LIBS
            VoxelEditor_VisualFeedback
            VoxelEditor_UndoRedo
            VoxelEditor_VoxelData
            VoxelEditor_Camera
        )
    endif()
    
    if(${TEST_NAME} MATCHES ".*mouse_ray.*")
        list(APPEND LIBS glm::glm)
    endif()
    
    if(${TEST_NAME} MATCHES ".*click_voxel.*" OR 
       ${TEST_NAME} MATCHES ".*voxel_face_clicking_simple.*")
        list(APPEND LIBS VoxelEditor_Logging)
    endif()
    
    # Remove duplicates
    list(REMOVE_DUPLICATES LIBS)
    set(${OUT_LIBRARIES} ${LIBS} PARENT_SCOPE)
endfunction()

# Create individual executables for each CLI integration test file
foreach(TEST_FILE ${CLI_INTEGRATION_TEST_FILES})
    # Extract the test name (filename without extension)
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    
    message(STATUS "Creating CLI integration test: ${TEST_NAME}")
    
    # Create executable
    add_executable(${TEST_NAME} ${TEST_FILE})
    
    # Standard include directories
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/foundation
    )
    
    # Get appropriate libraries for this test
    get_test_libraries(${TEST_NAME} TEST_LIBS)
    
    # Link libraries
    target_link_libraries(${TEST_NAME} PRIVATE ${TEST_LIBS})
    
    # Enable C++20
    target_compile_features(${TEST_NAME} PRIVATE cxx_std_20)
    
    # Mark rendering tests as expected to skip due to API issues
    if(${TEST_NAME} MATCHES ".*rendering.*")
        message(STATUS "Note: ${TEST_NAME} will skip at runtime due to API compatibility issues")
    endif()
    
    # Discover Google Tests
    gtest_discover_tests(${TEST_NAME})
    
endforeach()

# CLI Unit Tests (VoxelMeshGenerator) - kept separate as it's a unit test
add_executable(test_unit_cli_voxel_mesh_generator test_unit_cli_voxel_mesh_generator.cpp)

target_include_directories(test_unit_cli_voxel_mesh_generator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_unit_cli_voxel_mesh_generator PRIVATE
    VoxelEditor_CLI_Lib
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(test_unit_cli_voxel_mesh_generator PRIVATE cxx_std_20)

# Add mesh generator test
add_test(NAME cli_unit_tests COMMAND test_unit_cli_voxel_mesh_generator)

# Discover tests
include(GoogleTest)
gtest_discover_tests(test_unit_cli_voxel_mesh_generator)
