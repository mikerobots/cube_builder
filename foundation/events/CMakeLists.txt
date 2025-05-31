cmake_minimum_required(VERSION 3.16)

set(EVENTS_SOURCES
    EventBase.h
    EventHandler.h
    EventDispatcher.h
    CommonEvents.h
)

add_library(VoxelEditor_Events INTERFACE)

target_include_directories(VoxelEditor_Events INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(VoxelEditor_Events INTERFACE
    VoxelEditor_Math
)

target_compile_features(VoxelEditor_Events INTERFACE cxx_std_20)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()