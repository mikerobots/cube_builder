# Integration Test Documentation

This document describes the integration test suite and how to use the test runner.

## Test Runner

The main test runner script is `run_integration_tests.sh` in the project root. It provides a unified interface to run integration tests by category.

### Usage

```bash
# Show available test groups
./run_integration_tests.sh

# Run specific test group(s)
./run_integration_tests.sh core
./run_integration_tests.sh cli rendering
./run_integration_tests.sh boundary interaction

# Run all tests
./run_integration_tests.sh all

# Run quick smoke tests only
./run_integration_tests.sh quick

# Run visual validation tests
./run_integration_tests.sh visual
```

## Test Groups

### Core Groups

#### `core` - Core Integration Tests
C++ integration tests for core functionality:
- Camera and cube visibility
- Ground plane voxel placement
- Workspace boundary placement
- Mouse interaction tests

#### `cli` - CLI Validation Tests
Shell script tests that validate CLI functionality through screenshots:
- Basic voxel placement
- Camera view presets
- Multiple voxel patterns
- Render modes
- Resolution switching

#### `cli-cpp` - CLI C++ Integration Tests
C++ tests for CLI components:
- Command processing
- Error handling
- Headless mode
- Rendering pipeline
- Mouse/keyboard input
- Face clicking
- Zoom behavior

#### `comprehensive` - Comprehensive CLI Test Suite
Extended test suite with complete workflows:
- Full save/load/export workflows
- Enhancement validation
- Visual enhancements
- Voxel storage
- Error scenarios

#### `shader` - Shader Integration Tests
Tests for shader compilation and rendering:
- Shader compilation
- Ground plane shaders
- Voxel shaders
- Visual validation

#### `boundary` - Boundary and Edge Case Tests
Tests for edge cases and boundaries:
- Workspace boundary placement
- Edge rendering
- Grid vs edges
- Boundary validation

#### `rendering` - Rendering Validation Tests
Tests that validate rendering output:
- Camera views
- Render modes
- Screenshot validation
- Visual correctness

#### `interaction` - Interaction Tests
Mouse and keyboard interaction tests:
- Click-based voxel placement
- Face clicking
- Mouse ray movement
- Zoom behavior
- Ground plane clicking

### Meta Groups

#### `all` - Run All Tests
Runs every integration test in the suite.

#### `quick` - Quick Smoke Tests
Runs only essential tests for quick validation:
- Basic voxel placement
- Basic smoke test
- Minimal test set

#### `visual` - Visual Validation Tests
Runs tests that validate visual output through screenshot analysis:
- Voxel rendering
- Camera views
- Render modes
- Visual enhancements

## Test Output

The test runner provides:
- Colored output (✓ for pass, ✗ for fail, - for skip)
- Test execution time
- Summary statistics
- Failed test details
- Error logs for failures (last 10 lines)

## Adding New Tests

### Shell Script Tests
1. Create test script in appropriate directory
2. Make it executable: `chmod +x test_name.sh`
3. Follow naming convention: `test_*.sh`
4. Add to appropriate group in `run_integration_tests.sh`

### C++ Tests
1. Create test file following naming convention: `test_*.cpp`
2. Add to appropriate `CMakeLists.txt`
3. Test will be auto-discovered if it follows the naming pattern

## Test Directories

- `/tests/integration/` - Core integration tests (C++)
- `/tests/cli_validation/` - CLI validation scripts
- `/tests/cli_comprehensive/` - Comprehensive test suite
- `/apps/cli/tests/` - CLI-specific tests
- `/apps/shader_test/` - Shader test framework
- `/tests/verification/` - Core functionality verification

## Best Practices

1. **Test Isolation**: Each test should be independent and not rely on state from other tests
2. **Cleanup**: Tests should clean up any files they create
3. **Timeouts**: Use timeouts for tests that might hang
4. **Logging**: Write detailed logs to help debug failures
5. **Validation**: Use screenshot analysis for visual validation
6. **Error Messages**: Provide clear error messages on failure

## Debugging Failed Tests

1. Check the test output in `/tmp/<test_name>.log`
2. Run the individual test directly for more detailed output
3. Use debug flags if available in the test
4. Check screenshots in test output directories
5. Verify build is up-to-date

## Continuous Integration

The test runner is designed to work in CI environments:
- Returns 0 on success, 1 on failure
- Provides summary statistics
- Outputs are CI-friendly
- Can run specific groups for different CI stages