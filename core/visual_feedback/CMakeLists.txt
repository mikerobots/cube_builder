cmake_minimum_required(VERSION 3.16)
project(visual_feedback)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/FeedbackTypes.cpp
    src/FaceDetector.cpp
    src/HighlightRenderer.cpp
    src/HighlightManager.cpp
    src/OutlineRenderer.cpp
    src/OverlayRenderer.cpp
    src/FeedbackRenderer.cpp
    src/PreviewManager.cpp
)

# Header files
set(HEADERS
    include/visual_feedback/FeedbackTypes.h
    include/visual_feedback/FaceDetector.h
    include/visual_feedback/HighlightRenderer.h
    include/visual_feedback/HighlightManager.h
    include/visual_feedback/OutlineRenderer.h
    include/visual_feedback/OverlayRenderer.h
    include/visual_feedback/FeedbackRenderer.h
    include/visual_feedback/PreviewManager.h
)

# Create library
add_library(VoxelEditor_VisualFeedback ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(VoxelEditor_VisualFeedback 
    PUBLIC 
        include
        ../rendering
        ../voxel_data
        ../selection
        ../camera
        ../input
        ../../foundation/math
        ../../foundation/events
        ../../foundation/memory
        ../../foundation/logging
    PRIVATE
        ../
        ../../
)

# Link dependencies
target_link_libraries(VoxelEditor_VisualFeedback
    PUBLIC
        VoxelEditor_Math
        VoxelEditor_Rendering
        VoxelEditor_VoxelData
        VoxelEditor_Selection
        VoxelEditor_Camera
        VoxelEditor_Input
    PRIVATE
        VoxelEditor_Memory
        VoxelEditor_Events
        VoxelEditor_Logging
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(VoxelEditor_VisualFeedback PRIVATE VOXEL_EDITOR_WINDOWS)
elseif(APPLE)
    target_compile_definitions(VoxelEditor_VisualFeedback PRIVATE VOXEL_EDITOR_MACOS)
else()
    target_compile_definitions(VoxelEditor_VisualFeedback PRIVATE VOXEL_EDITOR_LINUX)
endif()

# Test executable
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()