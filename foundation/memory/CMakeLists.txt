cmake_minimum_required(VERSION 3.16)

set(MEMORY_SOURCES
    MemoryPool.h
    MemoryTracker.h
    MemoryOptimizer.h
)

add_library(VoxelEditor_Memory INTERFACE)

target_include_directories(VoxelEditor_Memory INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(VoxelEditor_Memory INTERFACE
    VoxelEditor_Events
    VoxelEditor_Math
)

target_compile_features(VoxelEditor_Memory INTERFACE cxx_std_20)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()