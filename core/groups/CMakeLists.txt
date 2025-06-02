cmake_minimum_required(VERSION 3.16)
project(groups)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/VoxelGroup.cpp
    src/GroupHierarchy.cpp
    src/GroupOperations.cpp
    src/GroupManager.cpp
)

# Header files
set(HEADERS
    include/groups/GroupTypes.h
    include/groups/VoxelGroup.h
    include/groups/GroupHierarchy.h
    include/groups/GroupOperations.h
    include/groups/GroupManager.h
)

# Create library
add_library(VoxelEditor_Groups ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(VoxelEditor_Groups 
    PUBLIC 
        include
        ../
        ../../foundation
    PRIVATE
        ../../
)

# Link dependencies
target_link_libraries(VoxelEditor_Groups
    PUBLIC
        VoxelEditor_Math
        VoxelEditor_Rendering
        VoxelEditor_VoxelData
        VoxelEditor_Events
    PRIVATE
        VoxelEditor_Memory
        VoxelEditor_Logging
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(VoxelEditor_Groups PRIVATE VOXEL_EDITOR_WINDOWS)
elseif(APPLE)
    target_compile_definitions(VoxelEditor_Groups PRIVATE VOXEL_EDITOR_MACOS)
else()
    target_compile_definitions(VoxelEditor_Groups PRIVATE VOXEL_EDITOR_LINUX)
endif()

# Test executable
if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()