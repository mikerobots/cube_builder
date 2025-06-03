cmake_minimum_required(VERSION 3.16)
project(PipelineTest)

set(CMAKE_CXX_STANDARD 17)

# Use GLFW from parent project (built as part of main project)
set(GLFW_DIR "${CMAKE_SOURCE_DIR}/../build/_deps/glfw-src")
set(GLFW_BINARY_DIR "${CMAKE_SOURCE_DIR}/../build/_deps/glfw-build")

# Add GLFW as subdirectory if not already built
if(NOT EXISTS ${GLFW_BINARY_DIR})
    # Configure and build GLFW in parent project first
    message(FATAL_ERROR "Please build the main project first to generate GLFW: cmake --build ../build")
endif()

# Link to the built GLFW library
add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION ${GLFW_BINARY_DIR}/src/libglfw3.a
    INTERFACE_INCLUDE_DIRECTORIES ${GLFW_DIR}/include
)

# Use the same GLAD as main project
set(GLAD_DIR "${CMAKE_SOURCE_DIR}/../external/glad")
add_library(glad_pipeline STATIC
    ${GLAD_DIR}/src/glad.c
)
target_include_directories(glad_pipeline PUBLIC ${GLAD_DIR}/include)

# Pipeline test executable
add_executable(pipeline_test
    main.cpp
    window.cpp
    shader.cpp
    renderer.cpp
)

target_link_libraries(pipeline_test
    glfw
    glad_pipeline
)

# macOS specific
if(APPLE)
    target_link_libraries(pipeline_test
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
endif()

# Silence OpenGL deprecation warnings on macOS
if(APPLE)
    target_compile_definitions(pipeline_test PRIVATE GL_SILENCE_DEPRECATION)
endif()