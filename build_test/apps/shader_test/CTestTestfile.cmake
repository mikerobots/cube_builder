# CMake generated Testfile for 
# Source directory: /Users/michaelhalloran/cube_edit/apps/shader_test
# Build directory: /Users/michaelhalloran/cube_edit/build_test/apps/shader_test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[ShaderValidation]=] "/Users/michaelhalloran/cube_edit/build_test/bin/test_integration_shader_basic_functionality" "--all" "--report" "shader_test_report.txt")
set_tests_properties([=[ShaderValidation]=] PROPERTIES  LABELS "shader;rendering" TIMEOUT "60" WORKING_DIRECTORY "/Users/michaelhalloran/cube_edit/build_test/bin" _BACKTRACE_TRIPLES "/Users/michaelhalloran/cube_edit/apps/shader_test/CMakeLists.txt;165;add_test;/Users/michaelhalloran/cube_edit/apps/shader_test/CMakeLists.txt;0;")
