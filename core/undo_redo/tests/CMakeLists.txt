# Find GTest package
find_package(GTest REQUIRED)

# Undo/Redo tests
add_executable(test_undo_redo
    TestCommand.cpp
)

# Simple command tests
add_executable(test_simple_command
    TestSimpleCommand.cpp
)

target_link_libraries(test_undo_redo
    PRIVATE
        VoxelEditor_UndoRedo
        VoxelEditor_Math
        VoxelEditor_Logging
        GTest::gtest_main
)

add_test(NAME test_undo_redo COMMAND test_undo_redo)

target_link_libraries(test_simple_command
    PRIVATE
        VoxelEditor_UndoRedo
        GTest::gtest_main
)

add_test(NAME test_simple_command COMMAND test_simple_command)

# History manager tests
add_executable(test_history_manager
    TestHistoryManager.cpp
)

target_link_libraries(test_history_manager
    PRIVATE
        VoxelEditor_UndoRedo
        GTest::gtest_main
)

add_test(NAME test_history_manager COMMAND test_history_manager)

# Placement commands tests
add_executable(test_placement_commands
    TestPlacementCommands.cpp
)

target_link_libraries(test_placement_commands
    PRIVATE
        VoxelEditor_UndoRedo
        VoxelEditor_VoxelData
        VoxelEditor_Math
        VoxelEditor_Logging
        GTest::gtest_main
)

add_test(NAME test_placement_commands COMMAND test_placement_commands)

# Requirements tests
add_executable(test_undo_redo_requirements
    test_UndoRedo_requirements.cpp
)

target_link_libraries(test_undo_redo_requirements
    PRIVATE
        VoxelEditor_UndoRedo
        VoxelEditor_VoxelData
        VoxelEditor_Math
        VoxelEditor_Events
        VoxelEditor_Logging
        GTest::gtest_main
)

add_test(NAME test_undo_redo_requirements COMMAND test_undo_redo_requirements)