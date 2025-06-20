# Test Organization

This directory contains all tests organized by type and purpose.

## Directory Structure

```
tests/
├── integration/        # C++ integration tests
├── e2e/               # End-to-end shell-based tests
├── unit/              # Unit tests (per subsystem)
├── performance/       # Performance benchmarks
├── verification/      # Core functionality verification
├── visual/           # Visual validation tests
└── core/             # Core test utilities
```

## Test Categories

### Integration Tests (`integration/`)
**Runner**: `./run_integration_tests.sh`

C++ tests that verify cross-component functionality:
- Core integration tests
- CLI C++ integration tests  
- Mouse/keyboard interaction tests
- Shader integration tests
- Visual feedback system integration
- Core functionality verification

### End-to-End Tests (`e2e/`)
**Runner**: `./run_e2e_tests.sh`

Shell-based tests that validate complete user workflows:
- CLI validation tests (screenshot-based)
- Comprehensive CLI workflow tests
- Visual validation tests
- Rendering validation tests
- Boundary and edge case tests

#### E2E Subdirectories:
- `cli_validation/` - Basic CLI functionality validation with screenshot analysis
- `cli_comprehensive/` - Complex CLI workflows and feature validation

### Unit Tests (`unit/`)
**Runner**: `./run_all_unit.sh`

Individual subsystem unit tests organized by component.

### Performance Tests (`performance/`)
Performance benchmarks and stress tests.

### Verification Tests (`verification/`)
Core functionality verification tests that validate fundamental system behavior.

### Visual Tests (`visual/`)
Visual validation tests using screenshot analysis and color verification.

## Usage

### Quick Testing
```bash
# Quick integration tests
./run_integration_tests.sh quick

# Quick e2e smoke tests  
./run_e2e_tests.sh quick

# All unit tests
./run_all_unit.sh
```

### Full Testing
```bash
# All integration tests
./run_integration_tests.sh all

# All e2e tests
./run_e2e_tests.sh all

# Specific test groups
./run_integration_tests.sh core cli-cpp
./run_e2e_tests.sh cli-validation visual
```

### Test Selection
```bash
# List available test groups
./run_integration_tests.sh list
./run_e2e_tests.sh list

# Run specific categories
./run_integration_tests.sh interaction shader
./run_e2e_tests.sh workflow boundary
```

## Migration Notes

The test organization was restructured to separate:
1. **Integration tests** - C++ tests verifying component interactions
2. **End-to-end tests** - Shell-based tests validating user workflows

This provides clearer separation of concerns and allows for more targeted test execution.