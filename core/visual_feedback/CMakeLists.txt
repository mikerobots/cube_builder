cmake_minimum_required(VERSION 3.16)
project(visual_feedback)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/FeedbackTypes.cpp
    src/FaceDetector.cpp
    src/HighlightRenderer.cpp
    src/OutlineRenderer.cpp
    src/OverlayRenderer.cpp
    src/FeedbackRenderer.cpp
)

# Header files
set(HEADERS
    include/visual_feedback/FeedbackTypes.h
    include/visual_feedback/FaceDetector.h
    include/visual_feedback/HighlightRenderer.h
    include/visual_feedback/OutlineRenderer.h
    include/visual_feedback/OverlayRenderer.h
    include/visual_feedback/FeedbackRenderer.h
)

# Create library
add_library(visual_feedback ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(visual_feedback 
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
target_link_libraries(visual_feedback
    PUBLIC
        math
        rendering
        voxel_data
        selection
        camera
    PRIVATE
        memory
        events
        logging
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(visual_feedback PRIVATE VOXEL_EDITOR_WINDOWS)
elseif(APPLE)
    target_compile_definitions(visual_feedback PRIVATE VOXEL_EDITOR_MACOS)
else()
    target_compile_definitions(visual_feedback PRIVATE VOXEL_EDITOR_LINUX)
endif()

# Test executable
if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()