set(CONFIG_SOURCES
    # Header-only implementation
)

set(CONFIG_HEADERS
    ConfigValue.h
    ConfigSection.h
    ConfigManager.h
)

add_library(VoxelEditor_Config INTERFACE)

target_include_directories(VoxelEditor_Config INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(VoxelEditor_Config INTERFACE
    VoxelEditor_Events
)

target_compile_features(VoxelEditor_Config INTERFACE cxx_std_20)

# Add tests subdirectory
add_subdirectory(tests)