# Comprehensive CLI Test Suite

This directory contains an extensive automated test suite for the voxel-cli application, covering both headless and rendering modes.

## Test Categories

### 1. Headless Tests (No Rendering)
Tests that validate command processing, data manipulation, and error handling without rendering.

- **Command Validation Tests**: Test all commands with valid/invalid parameters
- **Error Handling Tests**: Test error conditions and recovery
- **State Management Tests**: Test undo/redo, workspace changes, etc.
- **File I/O Tests**: Test save/load functionality
- **Performance Tests**: Test with large numbers of voxels

### 2. Rendering Tests
Tests that validate visual output through screenshot analysis.

- **Visual Validation Tests**: Test rendering correctness
- **Enhancement Tests**: Test new features (grid, highlighting, previews)
- **Edge Case Tests**: Test boundary conditions with visual verification
- **Performance Rendering Tests**: Test rendering with many voxels

### 3. Integration Tests
Tests that combine multiple features and validate complex workflows.

## Test Framework Features

- **Automatic timeout handling** to prevent hanging tests
- **Command error detection** through exit codes and output parsing
- **Screenshot validation** for rendering tests
- **Performance metrics** collection
- **Detailed logging** for debugging
- **Parallel test execution** support

## Running Tests

### All Tests
```bash
./run_all_tests.sh
```

### Headless Tests Only
```bash
./run_headless_tests.sh
```

### Rendering Tests Only
```bash
./run_rendering_tests.sh
```

### Individual Test Categories
```bash
./test_commands_headless.sh
./test_error_handling.sh
./test_visual_enhancements.sh
# etc.
```

## Test Output

Results are organized in subdirectories:
- `output/headless/` - Headless test results
- `output/rendering/` - Screenshots and analysis
- `output/logs/` - Detailed test logs
- `output/metrics/` - Performance metrics