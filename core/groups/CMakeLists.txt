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
add_library(groups ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(groups 
    PUBLIC 
        include
        ../rendering
        ../voxel_data
        ../selection
        ../camera
        ../../foundation/math
        ../../foundation/events
        ../../foundation/memory
        ../../foundation/logging
    PRIVATE
        ../
        ../../
)

# Link dependencies
target_link_libraries(groups
    PUBLIC
        math
        rendering
        voxel_data
        events
    PRIVATE
        memory
        logging
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(groups PRIVATE VOXEL_EDITOR_WINDOWS)
elseif(APPLE)
    target_compile_definitions(groups PRIVATE VOXEL_EDITOR_MACOS)
else()
    target_compile_definitions(groups PRIVATE VOXEL_EDITOR_LINUX)
endif()

# Test executable
if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()