# Core Undo/Redo System
add_library(core_undo_redo STATIC
    Command.h
    HistoryManager.h
    HistoryManager.cpp
    Transaction.h
    Transaction.cpp
    CompositeCommand.h
    CompositeCommand.cpp
    VoxelCommands.h
    VoxelCommands.cpp
    SelectionCommands.h
    SelectionCommands.cpp
    StateSnapshot.h
    StateSnapshot.cpp
)

target_include_directories(core_undo_redo
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

target_link_libraries(core_undo_redo
    PUBLIC
        VoxelEditor_Math
        VoxelEditor_Logging
        VoxelEditor_VoxelData
        VoxelEditor_Selection
)

# Set compile features
target_compile_features(core_undo_redo PUBLIC cxx_std_17)

# Add tests
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()