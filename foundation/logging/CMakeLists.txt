set(LOGGING_SOURCES
    # Header-only implementation
)

set(LOGGING_HEADERS
    Logger.h
    LogOutput.h
    PerformanceProfiler.h
)

add_library(VoxelEditor_Logging INTERFACE)

target_include_directories(VoxelEditor_Logging INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(VoxelEditor_Logging INTERFACE cxx_std_20)

# Add tests subdirectory
add_subdirectory(tests)