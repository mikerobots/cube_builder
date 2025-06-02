cmake_minimum_required(VERSION 3.10)
project(glad)

# Note: In a real project, you would download GLAD files from:
# https://glad.dav1d.de/
# For now, we'll create a stub library

add_library(glad STATIC
    src/glad.c
)

target_include_directories(glad PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Create stub files for now
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/include/glad/glad.h
"#pragma once
// GLAD stub - replace with actual GLAD generated files
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
typedef void* (*GLADloadproc)(const char *name);
int gladLoadGLLoader(GLADloadproc load);
")

file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c
"// GLAD stub implementation
#include \"glad/glad.h\"
int gladLoadGLLoader(GLADloadproc load) { return 1; }
")