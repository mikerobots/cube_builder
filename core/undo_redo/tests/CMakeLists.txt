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