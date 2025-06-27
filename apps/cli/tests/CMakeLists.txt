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
    
    if(${TEST_NAME} MATCHES ".*smoothing_export.*")
        list(APPEND LIBS 
            VoxelEditor_SurfaceGen
            VoxelEditor_FileIO
        )
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

# CLI Unit Tests (CoordinateSystemConstraints)
add_executable(test_unit_cli_coordinate_system_constraints test_unit_cli_coordinate_system_constraints.cpp)

target_include_directories(test_unit_cli_coordinate_system_constraints PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_unit_cli_coordinate_system_constraints PRIVATE
    VoxelEditor_CLI_Lib
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(test_unit_cli_coordinate_system_constraints PRIVATE cxx_std_20)

# Add coordinate system constraints test
add_test(NAME cli_coordinate_system_constraints_tests COMMAND test_unit_cli_coordinate_system_constraints)

# Discover tests
gtest_discover_tests(test_unit_cli_coordinate_system_constraints)

# CLI Unit Tests (Arbitrary Positions) - Test CLI commands with arbitrary 1cm positions
add_executable(test_unit_cli_arbitrary_positions test_unit_cli_arbitrary_positions.cpp)

target_include_directories(test_unit_cli_arbitrary_positions PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_unit_cli_arbitrary_positions PRIVATE
    VoxelEditor_CLI_Lib
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(test_unit_cli_arbitrary_positions PRIVATE cxx_std_20)

# Add arbitrary positions test
add_test(NAME cli_arbitrary_positions_tests COMMAND test_unit_cli_arbitrary_positions)

# Discover tests
gtest_discover_tests(test_unit_cli_arbitrary_positions)

# CLI Unit Tests (SelectCommands)
add_executable(test_unit_cli_select_commands test_unit_cli_select_commands.cpp)

target_include_directories(test_unit_cli_select_commands PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_unit_cli_select_commands PRIVATE
    VoxelEditor_CLI_Lib
    VoxelEditor_Selection
    VoxelEditor_VoxelData
    VoxelEditor_UndoRedo
    VoxelEditor_Groups
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(test_unit_cli_select_commands PRIVATE cxx_std_20)

# Add select commands test
add_test(NAME cli_select_commands_tests COMMAND test_unit_cli_select_commands)

# Discover tests
gtest_discover_tests(test_unit_cli_select_commands)

# CLI Unit Tests (MouseInteraction)
add_executable(test_unit_cli_mouse_interaction test_unit_cli_mouse_interaction.cpp)

target_include_directories(test_unit_cli_mouse_interaction PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_unit_cli_mouse_interaction PRIVATE
    VoxelEditor_CLI_Lib
    VoxelEditor_Camera
    VoxelEditor_VoxelData
    VoxelEditor_Input
    VoxelEditor_VisualFeedback
    VoxelEditor_UndoRedo
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)

target_compile_features(test_unit_cli_mouse_interaction PRIVATE cxx_std_20)

# Add mouse interaction test
add_test(NAME cli_mouse_interaction_tests COMMAND test_unit_cli_mouse_interaction)

# Discover tests
gtest_discover_tests(test_unit_cli_mouse_interaction)

# CLI Unit Tests (Ray Generation Accuracy)
add_executable(test_unit_cli_ray_generation_accuracy test_unit_cli_ray_generation_accuracy.cpp)

target_include_directories(test_unit_cli_ray_generation_accuracy PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_unit_cli_ray_generation_accuracy PRIVATE
    VoxelEditor_Math
    GTest::gtest
    GTest::gtest_main
    glm::glm
)

target_compile_features(test_unit_cli_ray_generation_accuracy PRIVATE cxx_std_20)

# Add ray generation accuracy test
add_test(NAME cli_ray_generation_accuracy_tests COMMAND test_unit_cli_ray_generation_accuracy)

# Discover tests
gtest_discover_tests(test_unit_cli_ray_generation_accuracy)

# CLI Stress Tests (Rapid Click Placement)
add_executable(test_stress_rapid_click_placement test_stress_rapid_click_placement.cpp)

target_include_directories(test_stress_rapid_click_placement PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_stress_rapid_click_placement PRIVATE
    VoxelEditor_CLI_Lib
    VoxelEditor_Camera
    VoxelEditor_VoxelData
    VoxelEditor_Input
    VoxelEditor_VisualFeedback
    VoxelEditor_UndoRedo
    VoxelEditor_Logging
    GTest::gtest
    GTest::gtest_main
    glfw
    OpenGL::GL
    ${CMAKE_DL_LIBS}
)

target_compile_features(test_stress_rapid_click_placement PRIVATE cxx_std_20)

# Add stress test
add_test(NAME cli_stress_rapid_click_placement_tests COMMAND test_stress_rapid_click_placement)

# Discover tests
gtest_discover_tests(test_stress_rapid_click_placement)

# CLI Unit Tests (Smoothing Commands)
add_executable(test_unit_cli_smoothing_commands test_unit_cli_smoothing_commands.cpp)

target_include_directories(test_unit_cli_smoothing_commands PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

target_link_libraries(test_unit_cli_smoothing_commands PRIVATE
    VoxelEditor_CLI_Lib
    VoxelEditor_SurfaceGen
    GTest::gtest
    GTest::gtest_main
)

target_compile_features(test_unit_cli_smoothing_commands PRIVATE cxx_std_20)

# Add smoothing commands test
add_test(NAME cli_smoothing_commands_tests COMMAND test_unit_cli_smoothing_commands)

# Discover tests
gtest_discover_tests(test_unit_cli_smoothing_commands)
