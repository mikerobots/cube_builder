cmake_minimum_required(VERSION 3.10)

# Generate build information
string(TIMESTAMP BUILD_DATE "%Y-%m-%d")
string(TIMESTAMP BUILD_TIME "%H:%M:%S")
string(TIMESTAMP BUILD_TIMESTAMP "%s" UTC)

# Get git information if available
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
else()
    set(GIT_COMMIT_HASH "unknown")
    set(GIT_BRANCH "unknown")
endif()

# Set project version if not defined
if(NOT DEFINED PROJECT_VERSION)
    set(PROJECT_VERSION "1.0.0")
endif()

# Configure build info header
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/cli/BuildInfo.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/cli/BuildInfo.h
    @ONLY
)

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
    # Command registry system
    src/CommandRegistry.cpp
    src/CommandModule.cpp
    # Modular command files
    src/commands/EditCommands.cpp
    src/commands/FileCommands.cpp
    src/commands/SelectCommands.cpp
    src/commands/ViewCommands.cpp
    src/commands/SystemCommands.cpp
    src/commands/MeshCommands.cpp
)

set(CLI_HEADERS
    include/cli/Application.h
    include/cli/CommandTypes.h
    include/cli/CommandProcessor.h
    include/cli/RenderWindow.h
    include/cli/MouseInteraction.h
    include/cli/AutoComplete.h
    include/cli/VoxelMeshGenerator.h
    # Modular command headers
    include/cli/CommandRegistry.h
    include/cli/EditCommands.h
    include/cli/FileCommands.h
    include/cli/SelectCommands.h
    include/cli/ViewCommands.h
    include/cli/SystemCommands.h
    include/cli/MeshCommands.h
)

# Create static library for testing
add_library(VoxelEditor_CLI_Lib STATIC ${CLI_LIB_SOURCES} ${CLI_HEADERS})

# Include directories for library
target_include_directories(VoxelEditor_CLI_Lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include  # For generated BuildInfo.h
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

# Copy shader files to build directory for CLI
add_custom_command(TARGET VoxelEditor_CLI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
        $<TARGET_FILE_DIR:VoxelEditor_CLI>/core/rendering/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/core/rendering/shaders
        $<TARGET_FILE_DIR:VoxelEditor_CLI>/core/rendering/shaders
    COMMENT "Copying shader files for VoxelEditor_CLI"
)

# Add tests subdirectory - always include it
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_subdirectory(tests)
endif()