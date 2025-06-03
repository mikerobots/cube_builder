cmake_minimum_required(VERSION 3.10)

# CLI Library (for testing)
set(CLI_LIB_SOURCES
    src/Application.cpp
    src/CommandTypes.cpp
    src/CommandProcessor.cpp
    src/RenderWindow.cpp
    src/Commands.cpp
    src/MouseInteraction.cpp
    src/AutoComplete.cpp
    src/VoxelMeshGenerator.cpp
    src/SimpleValidation.cpp
)

set(CLI_HEADERS
    include/cli/Application.h
    include/cli/CommandTypes.h
    include/cli/CommandProcessor.h
    include/cli/RenderWindow.h
    include/cli/MouseInteraction.h
    include/cli/AutoComplete.h
    include/cli/VoxelMeshGenerator.h
)

# Create static library for testing
add_library(VoxelEditor_CLI_Lib STATIC ${CLI_LIB_SOURCES} ${CLI_HEADERS})

# Include directories for library
target_include_directories(VoxelEditor_CLI_Lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/foundation
)

# Link libraries for library
target_link_libraries(VoxelEditor_CLI_Lib PUBLIC
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

# Platform-specific settings for library
if(APPLE)
    target_link_libraries(VoxelEditor_CLI_Lib PUBLIC
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
elseif(WIN32)
    target_link_libraries(VoxelEditor_CLI_Lib PUBLIC
        opengl32
    )
else()
    target_link_libraries(VoxelEditor_CLI_Lib PUBLIC
        GL
        X11
        pthread
        dl
    )
endif()

# Enable C++20 features for library
target_compile_features(VoxelEditor_CLI_Lib PUBLIC cxx_std_20)

# Create executable
add_executable(VoxelEditor_CLI src/main.cpp)

# Link the CLI library
target_link_libraries(VoxelEditor_CLI PRIVATE VoxelEditor_CLI_Lib)

# Copy to build directory
set_target_properties(VoxelEditor_CLI PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Add tests subdirectory - always include it
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_subdirectory(tests)
endif()