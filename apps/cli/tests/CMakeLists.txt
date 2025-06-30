# Include the test helpers
include(${CMAKE_SOURCE_DIR}/cmake/TestHelpers.cmake)

# Override the default create_unit_tests to handle CLI-specific dependencies
function(create_cli_tests)
    find_package(GTest REQUIRED)
    
    # Discover all test files
    file(GLOB UNIT_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_unit_*.cpp")
    file(GLOB INTEGRATION_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_integration_*.cpp")
    
    # Function to determine appropriate libraries for each test
    function(get_cli_test_libraries TEST_NAME OUT_LIBRARIES)
        # Default libraries for all tests
        set(LIBS
            VoxelEditor_CLI_Lib
            GTest::gtest
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
        
        if(${TEST_NAME} MATCHES ".*mouse_ray.*" OR
           ${TEST_NAME} MATCHES ".*ray_generation.*")
            list(APPEND LIBS glm::glm)
        endif()
        
        if(${TEST_NAME} MATCHES ".*click_voxel.*" OR 
           ${TEST_NAME} MATCHES ".*voxel_face_clicking_simple.*")
            list(APPEND LIBS VoxelEditor_Logging)
        endif()
        
        if(${TEST_NAME} MATCHES ".*smoothing.*")
            list(APPEND LIBS 
                VoxelEditor_SurfaceGen
                VoxelEditor_FileIO
            )
        endif()
        
        if(${TEST_NAME} MATCHES ".*select_commands.*")
            list(APPEND LIBS
                VoxelEditor_Selection
                VoxelEditor_VoxelData
                VoxelEditor_UndoRedo
                VoxelEditor_Groups
            )
        endif()
        
        if(${TEST_NAME} MATCHES ".*mouse_interaction.*")
            list(APPEND LIBS
                VoxelEditor_Camera
                VoxelEditor_VoxelData
                VoxelEditor_Input
                VoxelEditor_VisualFeedback
                VoxelEditor_UndoRedo
                GTest::gmock
            )
        endif()
        
        if(${TEST_NAME} MATCHES ".*ray_generation_accuracy.*")
            list(APPEND LIBS
                VoxelEditor_Math
                glm::glm
            )
        endif()
        
        # Remove duplicates
        list(REMOVE_DUPLICATES LIBS)
        set(${OUT_LIBRARIES} ${LIBS} PARENT_SCOPE)
    endfunction()
    
    # Create unit test executables
    foreach(test_source ${UNIT_TEST_SOURCES})
        get_filename_component(test_name ${test_source} NAME_WE)
        
        message(STATUS "Creating CLI unit test: ${test_name}")
        
        add_executable(${test_name} ${test_source})
        
        target_include_directories(${test_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
            ${CMAKE_SOURCE_DIR}/core
            ${CMAKE_SOURCE_DIR}/foundation
        )
        
        get_cli_test_libraries(${test_name} TEST_LIBS)
        target_link_libraries(${test_name} ${TEST_LIBS})
        
        target_compile_features(${test_name} PRIVATE cxx_std_20)
        
        include(GoogleTest)
        gtest_discover_tests(${test_name})
        
        set_target_properties(${test_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
    endforeach()
    
    # Create integration test executables
    foreach(test_source ${INTEGRATION_TEST_SOURCES})
        get_filename_component(test_name ${test_source} NAME_WE)
        
        message(STATUS "Creating CLI integration test: ${test_name}")
        
        add_executable(${test_name} ${test_source})
        
        target_include_directories(${test_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
            ${CMAKE_SOURCE_DIR}/core
            ${CMAKE_SOURCE_DIR}/foundation
        )
        
        get_cli_test_libraries(${test_name} TEST_LIBS)
        target_link_libraries(${test_name} ${TEST_LIBS})
        
        target_compile_features(${test_name} PRIVATE cxx_std_20)
        
        # Mark rendering tests as expected to skip
        if(${test_name} MATCHES ".*rendering.*")
            message(STATUS "Note: ${test_name} will skip at runtime due to API compatibility issues")
        endif()
        
        include(GoogleTest)
        gtest_discover_tests(${test_name})
        
        set_target_properties(${test_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
    endforeach()
endfunction()

# Create all CLI tests with proper dependencies
create_cli_tests()