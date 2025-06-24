# Core Undo/Redo System
add_library(VoxelEditor_UndoRedo STATIC
    Command.h
    HistoryManager.h
    HistoryManager.cpp
    Transaction.h
    Transaction.cpp
    CompositeCommand.h
    CompositeCommand.cpp
    VoxelCommands.h
    VoxelCommands.cpp
    PlacementCommands.h
    PlacementCommands.cpp
    SelectionCommands.h
    SelectionCommands.cpp
    StateSnapshot.h
    StateSnapshot.cpp
)

target_include_directories(VoxelEditor_UndoRedo
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

target_link_libraries(VoxelEditor_UndoRedo
    PUBLIC
        VoxelEditor_Math
        VoxelEditor_Logging
        VoxelEditor_VoxelData
        VoxelEditor_Selection
        VoxelEditor_Input
)

# Set compile features
target_compile_features(VoxelEditor_UndoRedo PUBLIC cxx_std_17)

# Add tests
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()