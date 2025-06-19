# TODO - Core Subsystem Test Script Refactoring

## 📋 WORK INSTRUCTIONS

**IMPORTANT**: This TODO.md file is shared across multiple agents/developers. To avoid conflicts:

1. **BEFORE STARTING ANY WORK**: Mark the item as "🔄 IN PROGRESS - [Your Name]"
2. **UPDATE STATUS**: Change back to normal when complete or if you stop working on it
3. **ATOMIC UPDATES**: Make small, frequent updates to avoid conflicts
4. **COMMUNICATE**: If you see something marked "IN PROGRESS", work on a different item

## 🎯 Project Goal

Refactor all run_tests.sh scripts in core subsystems to have standardized options and run ALL tests in each subsystem. Each script should categorize tests by execution time and provide consistent user experience across all subsystems.

## 📊 Current Status

All 10 core subsystems have run_tests.sh scripts but with inconsistent interfaces and test coverage. Need to standardize to:

- **--quick**: Run all tests under 1 second
- **--full**: Run all tests under 5 seconds  
- **--slow**: Run all tests over 5 seconds (typically performance tests)

## 🚀 Test Script Refactoring Tasks

### Subsystem Tasks (Pick one and mark IN PROGRESS)

#### 1. Camera System (core/camera/run_tests.sh) - ✅ COMPLETED - Alex
- [x] Profile all tests with timing to categorize by execution time
- [x] Implement standardized --quick/--full/--slow options
- [x] Ensure ALL tests in subsystem are included in at least one mode
- [x] Add consistent error handling and timeout enforcement
- [x] Document test categories and execution times
- [x] Verify script handles all test executables in the subsystem

#### 2. File I/O System (core/file_io/run_tests.sh) - ✅ COMPLETED - Maya
- [x] Profile all tests with timing to categorize by execution time
- [x] Implement standardized --quick/--full/--slow options
- [x] Ensure ALL tests in subsystem are included in at least one mode
- [x] Add consistent error handling and timeout enforcement
- [x] Document test categories and execution times
- [x] Verify script handles all test executables in the subsystem

#### 3. Groups System (core/groups/run_tests.sh) - ✅ COMPLETED - Zara
- [x] Profile all tests with timing to categorize by execution time
- [x] Implement standardized --quick/--full/--slow options
- [x] Ensure ALL tests in subsystem are included in at least one mode
- [x] Add consistent error handling and timeout enforcement
- [x] Document test categories and execution times
- [x] Verify script handles all test executables in the subsystem

#### 4. Input System (core/input/run_tests.sh) - ✅ COMPLETED - Jordan
- [x] Profile all tests with timing to categorize by execution time
- [x] Implement standardized --quick/--full/--slow options
- [x] Ensure ALL tests in subsystem are included in at least one mode
- [x] Add consistent error handling and timeout enforcement
- [x] Document test categories and execution times
- [x] Verify script handles all test executables in the subsystem

#### 5. Rendering System (core/rendering/run_tests.sh) - ✅ COMPLETED - Claude
- [x] Profile all tests with timing to categorize by execution time
- [x] Implement standardized --quick/--full/--slow options
- [x] Ensure ALL tests in subsystem are included in at least one mode
- [x] Add consistent error handling and timeout enforcement
- [x] Document test categories and execution times
- [x] Verify script handles all test executables in the subsystem

#### 6. Selection System (core/selection/run_tests.sh) - ✅ COMPLETED - River
- [x] Profile all tests with timing to categorize by execution time
- [x] Implement standardized --quick/--full/--slow options
- [x] Ensure ALL tests in subsystem are included in at least one mode
- [x] Add consistent error handling and timeout enforcement
- [x] Document test categories and execution times
- [x] Verify script handles all test executables in the subsystem

#### 7. Surface Generation (core/surface_gen/run_tests.sh) - ✅ COMPLETED - Sam
- [x] Profile all tests with timing to categorize by execution time
- [x] Implement standardized --quick/--full/--slow options
- [x] Ensure ALL tests in subsystem are included in at least one mode
- [x] Add consistent error handling and timeout enforcement
- [x] Document test categories and execution times
- [x] Verify script handles all test executables in the subsystem

#### 8. Undo/Redo System (core/undo_redo/run_tests.sh) - ✅ COMPLETED - Phoenix
- [x] Profile all tests with timing to categorize by execution time
- [x] Implement standardized --quick/--full/--slow options
- [x] Ensure ALL tests in subsystem are included in at least one mode
- [x] Add consistent error handling and timeout enforcement
- [x] Document test categories and execution times
- [x] Verify script handles all test executables in the subsystem

#### 9. Visual Feedback (core/visual_feedback/run_tests.sh) - ✅ COMPLETED - Nova
- [x] Profile all tests with timing to categorize by execution time
- [x] Implement standardized --quick/--full/--slow options
- [x] Ensure ALL tests in subsystem are included in at least one mode
- [x] Add consistent error handling and timeout enforcement
- [x] Document test categories and execution times
- [x] Verify script handles all test executables in the subsystem

#### 10. VoxelData System (core/voxel_data/run_tests.sh) - ✅ COMPLETED - Dakota
- [x] Profile all tests with timing to categorize by execution time
- [x] Implement standardized --quick/--full/--slow options
- [x] Ensure ALL tests in subsystem are included in at least one mode
- [x] Add consistent error handling and timeout enforcement
- [x] Document test categories and execution times
- [x] Verify script handles all test executables in the subsystem

## 📋 Standard Script Template

Each run_tests.sh script should follow this standardized template:

### Script Header
```bash
#!/bin/bash

# [Subsystem] test runner with standardized options
# Usage: ./run_tests.sh [build_dir] [--quick|--full|--slow]
#
# Test execution modes:
#   --quick  : Run all tests under 1 second (default)
#   --full   : Run all tests under 5 seconds
#   --slow   : Run all tests over 5 seconds (performance tests)
#
# Expected execution times:
#   --quick mode: <1 second total
#   --full mode:  <5 seconds total
#   --slow mode:  Variable (may take minutes)
```

### Required Features
- **Argument parsing**: Support `[build_dir] [--quick|--full|--slow]` format
- **Test discovery**: Automatically find ALL test executables in the subsystem
- **Timing output**: Use `--gtest_print_time=1` to show individual test times
- **Error handling**: Proper exit codes and timeout enforcement
- **Progress reporting**: Clear console output showing test progress
- **Test categorization**: Document which tests fall into each category

### Test Categorization Rules
- **Quick tests (<1s)**: Basic unit tests, type tests, simple functionality
- **Full tests (<5s)**: Integration tests, moderate complexity tests
- **Slow tests (>5s)**: Performance tests, stress tests, large dataset tests

## 🔧 Implementation Guidelines

### 1. Test Profiling Process
For each subsystem:
1. Run all tests with `--gtest_print_time=1` to get baseline timings
2. Categorize tests by execution time (quick/full/slow)
3. Identify any tests that need optimization or special handling
4. Document test categories and typical execution times

### 2. Script Standardization
- Use consistent argument parsing across all scripts
- Implement same timeout values (60s quick, 300s full, no limit slow)
- Use same color coding and output formatting
- Include help text with `--help` option

### 3. Error Handling
- Proper exit codes for different failure types
- Timeout detection with informative messages
- Build dependency checking before test execution
- Clear error messages for missing executables

### 4. Documentation Requirements
- Header comment explaining usage and timing expectations
- Inline documentation of test categories
- Example commands for common usage patterns
- Performance notes for slow tests

## ✅ Success Criteria

- [x] All 10 core subsystems have standardized run_tests.sh scripts
- [x] Each script supports --quick/--full/--slow options consistently
- [x] ALL tests in each subsystem are included in at least one mode
- [x] Test categorization is based on actual execution times
- [x] Scripts have consistent error handling and user experience
- [x] Documentation is complete for all scripts
- [x] Scripts handle edge cases (missing executables, timeouts, etc.)

## 📌 Quality Standards

- **Performance**: Quick mode completes in <1s, full mode in <5s
- **Reliability**: Scripts handle all error conditions gracefully
- **Consistency**: All scripts have identical interface and behavior
- **Completeness**: No tests are excluded from script coverage
- **Documentation**: Clear usage instructions and timing expectations
- **Maintainability**: Scripts are easy to understand and modify

## 🚀 Execution Plan

1. **Phase 1**: Profile all existing tests to understand current timing characteristics
2. **Phase 2**: Create standardized script template and implement for 2-3 subsystems
3. **Phase 3**: Roll out standardized scripts to remaining subsystems
4. **Phase 4**: Validate all scripts work correctly and handle edge cases - ✅ COMPLETED - Kai
5. **Phase 5**: Update project documentation with new standardized interface

### 🔍 Validation Results (Phase 4):
- **✅ Fully Compliant (6)**: Camera, Selection, Surface Generation, File I/O, Groups, Undo/Redo (ALL FIXED)
- **⚠️ Minor Issue (1)**: Input (accepts invalid `--` options as build directories)
- **🔄 In Progress (3)**: Rendering, Visual Feedback, VoxelData

#### ✅ Fixes Applied:
- **File I/O**: Fixed rigid positional argument parsing to use flexible while loop
- **Groups**: Added validation for invalid `--` arguments  
- **Undo/Redo**: Added validation for invalid `--` arguments

#### ⚠️ Remaining Issue:
- **Input**: Uses legacy positional parsing, treats `--invalid` as build directory instead of rejecting it