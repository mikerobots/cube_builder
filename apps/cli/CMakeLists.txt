cmake_minimum_required(VERSION 3.10)

# CLI executable
set(CLI_SOURCES
    src/main.cpp
    src/Application.cpp
    src/CommandTypes.cpp
    src/CommandProcessor.cpp
    src/RenderWindow.cpp
    src/Commands.cpp
    src/MouseInteraction.cpp
    src/AutoComplete.cpp
)

set(CLI_HEADERS
    include/cli/Application.h
    include/cli/CommandTypes.h
    include/cli/CommandProcessor.h
    include/cli/RenderWindow.h
    include/cli/MouseInteraction.h
    include/cli/AutoComplete.h
)

# Create executable
add_executable(VoxelEditor_CLI ${CLI_SOURCES} ${CLI_HEADERS})

# Include directories
target_include_directories(VoxelEditor_CLI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

# Link libraries
target_link_libraries(VoxelEditor_CLI PRIVATE
    # Core libraries
    VoxelEditor_VoxelData
    VoxelEditor_Camera
    VoxelEditor_Rendering
    VoxelEditor_Input
    VoxelEditor_Selection
    VoxelEditor_UndoRedo
    VoxelEditor_SurfaceGen
    VoxelEditor_VisualFeedback
    VoxelEditor_Groups
    VoxelEditor_FileIO
    
    # Foundation libraries
    VoxelEditor_Events
    VoxelEditor_Logging
    VoxelEditor_Config
    VoxelEditor_Math
    VoxelEditor_Memory
    
    # External dependencies
    glfw
    glad
    glm::glm
)

# Platform-specific settings
if(APPLE)
    target_link_libraries(VoxelEditor_CLI PRIVATE
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
elseif(WIN32)
    target_link_libraries(VoxelEditor_CLI PRIVATE
        opengl32
    )
else()
    target_link_libraries(VoxelEditor_CLI PRIVATE
        GL
        X11
        pthread
        dl
    )
endif()

# Copy to build directory
set_target_properties(VoxelEditor_CLI PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Enable C++20 features (for starts_with, ends_with)
target_compile_features(VoxelEditor_CLI PRIVATE cxx_std_20)

# Add tests subdirectory
if(BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_subdirectory(tests)
endif()