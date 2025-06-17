cmake_minimum_required(VERSION 3.16)

set(MATH_SOURCES
    Vector2f.h
    Vector3f.h
    Vector3i.h
    Matrix4f.h
)

add_library(VoxelEditor_Math INTERFACE)

target_include_directories(VoxelEditor_Math INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(VoxelEditor_Math INTERFACE cxx_std_20)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()