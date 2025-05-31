cmake_minimum_required(VERSION 3.16)
project(VoxelEditor VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_CLI "Build command line application" ON)
option(BUILD_QT_DESKTOP "Build Qt desktop application" OFF)
option(BUILD_VR "Build VR application" OFF)

# Find packages
find_package(OpenGL REQUIRED)

if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()
endif()

# Add subdirectories
add_subdirectory(foundation)
add_subdirectory(core)