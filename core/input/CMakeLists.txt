# Core Input Module
add_library(VoxelEditor_Input STATIC
    # Header files
    InputTypes.h
    InputHandler.h
    InputMapping.h
    InputManager.h
    MouseHandler.h
    KeyboardHandler.h
    TouchHandler.h
    VRInputHandler.h
    
    # Source files (placeholder implementations)
    InputTypes.cpp
    # TODO: Add full implementations when platform integration is complete
    # InputMapping.cpp
    # InputManager.cpp
    # MouseHandler.cpp
    # KeyboardHandler.cpp
    # TouchHandler.cpp
    # VRInputHandler.cpp
)

# Set target properties
set_target_properties(VoxelEditor_Input PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Include directories
target_include_directories(VoxelEditor_Input PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(VoxelEditor_Input
    VoxelEditor_Math
    VoxelEditor_Events
    VoxelEditor_Camera
)

# Platform-specific dependencies (when implementation is added)
# if(WIN32)
#     target_link_libraries(VoxelEditor_Input user32 xinput)
# elseif(APPLE)
#     find_library(IOKIT_LIBRARY IOKit)
#     target_link_libraries(VoxelEditor_Input ${IOKIT_LIBRARY})
# else()
#     # Linux input libraries
#     target_link_libraries(VoxelEditor_Input X11 Xi)
# endif()

# VR support (when implementation is added)
# find_package(OpenXR QUIET)
# if(OpenXR_FOUND)
#     target_link_libraries(VoxelEditor_Input OpenXR::openxr_loader)
#     target_compile_definitions(VoxelEditor_Input PRIVATE VOXEL_EDITOR_VR_SUPPORT)
# endif()

# Thread support for event queue
find_package(Threads REQUIRED)
target_link_libraries(VoxelEditor_Input Threads::Threads)

# Preprocessor definitions
target_compile_definitions(VoxelEditor_Input PRIVATE
    VOXEL_EDITOR_INPUT_MODULE
)

# Compiler-specific options
if(MSVC)
    target_compile_options(VoxelEditor_Input PRIVATE /W4)
else()
    target_compile_options(VoxelEditor_Input PRIVATE -Wall -Wextra -pedantic)
endif()

# Add tests subdirectory
add_subdirectory(tests)