# Core Rendering Module
add_library(VoxelEditor_Rendering STATIC
    # Header files
    RenderTypes.h
    RenderConfig.h
    RenderStats.h
    RenderEngine.h
    OpenGLRenderer.h
    RenderState.h
    ShaderManager.h
    
    # Source files
    RenderTypes.cpp
    # TODO: Add implementations when OpenGL integration is complete
    # RenderEngine.cpp
    # OpenGLRenderer.cpp
    # RenderState.cpp
    # ShaderManager.cpp
)

# Set target properties
set_target_properties(VoxelEditor_Rendering PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Include directories
target_include_directories(VoxelEditor_Rendering PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(VoxelEditor_Rendering
    VoxelEditor_Math
    VoxelEditor_Events
    VoxelEditor_Camera
)

# OpenGL dependencies (when implementation is added)
# find_package(OpenGL REQUIRED)
# target_link_libraries(VoxelEditor_Rendering ${OPENGL_LIBRARIES})

# Platform-specific OpenGL loader (when implementation is added)
# if(WIN32)
#     target_link_libraries(VoxelEditor_Rendering opengl32)
# elseif(APPLE)
#     find_library(OPENGL_LIBRARY OpenGL)
#     target_link_libraries(VoxelEditor_Rendering ${OPENGL_LIBRARY})
# else()
#     target_link_libraries(VoxelEditor_Rendering GL)
# endif()

# Preprocessor definitions
target_compile_definitions(VoxelEditor_Rendering PRIVATE
    VOXEL_EDITOR_RENDERING_MODULE
)

# Compiler-specific options
if(MSVC)
    target_compile_options(VoxelEditor_Rendering PRIVATE /W4)
else()
    target_compile_options(VoxelEditor_Rendering PRIVATE -Wall -Wextra -pedantic)
endif()

# Add tests subdirectory
add_subdirectory(tests)