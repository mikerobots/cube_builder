# End-to-End Tests

Shell-based tests that validate complete user workflows through the CLI application.

## Directory Structure

```
e2e/
├── cli_validation/      # Basic CLI functionality validation
└── cli_comprehensive/   # Complex CLI workflows and features
```

## Test Categories

### CLI Validation (`cli_validation/`)
Screenshot-based automated tests using PPM color analysis:
- **`test_basic_voxel_placement.sh`** - Single voxel placement validation
- **`test_camera_views.sh`** - All camera view presets (front/back/top/bottom/left/right/iso)
- **`test_resolution_switching.sh`** - Multiple voxel resolutions (1cm-32cm)
- **`test_multiple_voxels.sh`** - Multiple voxel patterns
- **`test_render_modes.sh`** - Rendering functionality validation

**Key Features**:
- Automated visual validation without human intervention
- PPM color distribution analysis via `tools/analyze_ppm_colors.py`
- Proper failure detection when voxels not visually rendered

### CLI Comprehensive (`cli_comprehensive/`)
Complex workflow tests covering advanced features:
- **`test_integration.sh`** - Complete workflow validation
- **`test_enhancement_validation.sh`** - Feature enhancement testing
- **`test_visual_enhancements.sh`** - Visual feature testing
- **`test_error_handling.sh`** - Error condition validation
- **`test_new_features.sh`** - New feature validation
- **`test_basic_smoke.sh`** - Basic smoke tests
- **`test_minimal.sh`** - Minimal functionality tests

## Usage

### Run All E2E Tests
```bash
./run_e2e_tests.sh all
```

### Run Specific Categories
```bash
./run_e2e_tests.sh cli-validation     # Basic validation tests
./run_e2e_tests.sh cli-comprehensive  # Complex workflow tests
./run_e2e_tests.sh visual            # Visual validation tests
./run_e2e_tests.sh quick             # Quick smoke tests only
```

### Run Individual Test Suites
```bash
# From CLI validation directory
cd tests/e2e/cli_validation
./run_all_tests.sh

# From CLI comprehensive directory  
cd tests/e2e/cli_comprehensive
./run_all_tests.sh
```

## Test Requirements

### Visual Tests
- Require OpenGL context for rendering
- Generate PPM screenshots for analysis
- Use color distribution analysis to verify voxel rendering

### Headless Tests
- Can run without display
- Test command processing and logic
- Validate error handling and edge cases

### Timeout Settings
- Quick tests: 60 seconds
- Full tests: 300 seconds (5 minutes)
- Individual test scripts may have custom timeouts

## Output Analysis

### Screenshot Analysis
Tests use `tools/analyze_ppm_colors.py` to:
- Analyze color distribution in PPM images
- Detect presence of voxels (non-background colors)
- Validate visual correctness across camera angles

### Test Results
- **PASS**: Test completed successfully with expected visual output
- **FAIL**: Test failed with error output shown
- **SKIP**: Test skipped (missing dependencies, etc.)

## Error Handling

Tests include validation for:
- Invalid command parameters
- Boundary conditions (workspace limits, ground plane)
- Memory stress conditions
- File I/O errors
- Resolution switching edge cases