cmake_minimum_required(VERSION 3.16)
project(file_io)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LZ4 compression library
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(LZ4 liblz4)
endif()

# If pkg-config didn't find LZ4, try to find it manually
if(NOT LZ4_FOUND)
    find_path(LZ4_INCLUDE_DIR lz4.h)
    find_library(LZ4_LIBRARY NAMES lz4)
    if(LZ4_INCLUDE_DIR AND LZ4_LIBRARY)
        set(LZ4_FOUND TRUE)
        set(LZ4_INCLUDE_DIRS ${LZ4_INCLUDE_DIR})
        set(LZ4_LIBRARIES ${LZ4_LIBRARY})
    endif()
endif()

# Source files
set(SOURCES
    src/FileTypes.cpp
    src/BinaryIO.cpp
    src/Project.cpp
    src/BinaryFormat.cpp
    src/STLExporter.cpp
    src/Compression.cpp
    src/FileVersioning.cpp
    src/FileManager.cpp
)

# Header files
set(HEADERS
    include/file_io/FileTypes.h
    include/file_io/BinaryIO.h
    include/file_io/Project.h
    include/file_io/BinaryFormat.h
    include/file_io/STLExporter.h
    include/file_io/Compression.h
    include/file_io/FileVersioning.h
    include/file_io/FileManager.h
)

# Create library
add_library(file_io ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(file_io 
    PUBLIC 
        include
        ../rendering
        ../voxel_data
        ../selection
        ../camera
        ../groups
        ../visual_feedback
        ../../foundation/math
        ../../foundation/events
        ../../foundation/memory
        ../../foundation/logging
    PRIVATE
        ../
        ../../
)

# Add LZ4 include directories if found
if(LZ4_FOUND)
    target_include_directories(file_io PRIVATE ${LZ4_INCLUDE_DIRS})
    target_compile_definitions(file_io PRIVATE HAVE_LZ4)
endif()

# Link dependencies
target_link_libraries(file_io
    PUBLIC
        math
        rendering
        voxel_data
        groups
        selection
        camera
        events
    PRIVATE
        memory
        logging
)

# Link LZ4 if found
if(LZ4_FOUND)
    target_link_libraries(file_io PRIVATE ${LZ4_LIBRARIES})
endif()

# Platform-specific settings
if(WIN32)
    target_compile_definitions(file_io PRIVATE VOXEL_EDITOR_WINDOWS)
elseif(APPLE)
    target_compile_definitions(file_io PRIVATE VOXEL_EDITOR_MACOS)
else()
    target_compile_definitions(file_io PRIVATE VOXEL_EDITOR_LINUX)
endif()

# Test executable
if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()