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

# Set up Homebrew paths for macOS
if(APPLE)
    set(CMAKE_PREFIX_PATH "/opt/homebrew;${CMAKE_PREFIX_PATH}")
    list(APPEND CMAKE_LIBRARY_PATH "/opt/homebrew/lib")
    list(APPEND CMAKE_INCLUDE_PATH "/opt/homebrew/include")
endif()

# External dependencies
include(FetchContent)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

# GLAD
add_subdirectory(external/glad)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

# Add subdirectories
add_subdirectory(foundation)
add_subdirectory(core)

# Add applications
if(BUILD_CLI)
    add_subdirectory(apps/cli)
endif()

if(BUILD_QT_DESKTOP)
    add_subdirectory(apps/desktop)
endif()

if(BUILD_VR)
    add_subdirectory(apps/vr)
endif()