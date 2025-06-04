# CMake generated Testfile for 
# Source directory: /Users/michaelhalloran/cube_edit/core/undo_redo/tests
# Build directory: /Users/michaelhalloran/cube_edit/build/core/undo_redo/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test_undo_redo "/Users/michaelhalloran/cube_edit/build/bin/test_undo_redo")
set_tests_properties(test_undo_redo PROPERTIES  _BACKTRACE_TRIPLES "/Users/michaelhalloran/cube_edit/core/undo_redo/tests/CMakeLists.txt;19;add_test;/Users/michaelhalloran/cube_edit/core/undo_redo/tests/CMakeLists.txt;0;")
add_test(test_simple_command "/Users/michaelhalloran/cube_edit/build/bin/test_simple_command")
set_tests_properties(test_simple_command PROPERTIES  _BACKTRACE_TRIPLES "/Users/michaelhalloran/cube_edit/core/undo_redo/tests/CMakeLists.txt;27;add_test;/Users/michaelhalloran/cube_edit/core/undo_redo/tests/CMakeLists.txt;0;")
add_test(test_history_manager "/Users/michaelhalloran/cube_edit/build/bin/test_history_manager")
set_tests_properties(test_history_manager PROPERTIES  _BACKTRACE_TRIPLES "/Users/michaelhalloran/cube_edit/core/undo_redo/tests/CMakeLists.txt;40;add_test;/Users/michaelhalloran/cube_edit/core/undo_redo/tests/CMakeLists.txt;0;")
