# CLI Validation Test Suite

This directory contains automated tests for validating the voxel-cli application functionality using screenshot analysis.

## Overview

The test suite validates core CLI functionality by:
1. Executing CLI commands via stdin
2. Capturing screenshots as PPM files
3. Analyzing the color distribution in screenshots
4. Validating expected visual results

## Test Dependencies

- **voxel-cli**: The CLI application (built from project root)
- **PPM Color Analyzer**: `../../tools/analyze_ppm_colors.py`
- **timeout**: For preventing hanging tests
- **Standard Unix tools**: grep, awk, head, tail, etc.

## Available Tests

### 1. Basic Voxel Placement (`test_basic_voxel_placement.sh`)
**Purpose**: Validates that a single voxel can be placed and appears in the screenshot.

**Commands tested**:
- `workspace 5 5 5`
- `resolution 8cm`
- `place 2 2 2`
- `screenshot`

**Validation**: Checks for yellow/bright colored pixels indicating voxel presence.

### 2. Camera View Changes (`test_camera_views.sh`)
**Purpose**: Tests all camera view presets and validates voxel visibility from different angles.

**Commands tested**:
- `view front|back|top|bottom|left|right|iso`

**Validation**: Ensures each view shows non-background colors and produces unique perspectives.

### 3. Resolution Switching (`test_resolution_switching.sh`)
**Purpose**: Validates that different voxel resolutions render with appropriate size differences.

**Commands tested**:
- `resolution 1cm|4cm|8cm|16cm|32cm`

**Validation**: 
- Each resolution produces visible voxels
- Larger resolutions generally show more pixels (when applicable)
- Compares pixel counts across resolutions

### 4. Multiple Voxel Placement (`test_multiple_voxels.sh`)
**Purpose**: Tests placing multiple voxels in different positions and validates they all appear.

**Commands tested**:
- Multiple `place` commands with different coordinates
- `zoom 0.8` for better view

**Validation**: 
- Multiple unique colors present
- Significant non-background pixel count
- Evidence of multiple distinct voxel regions

### 5. Render Mode Validation (`test_render_modes.sh`)
**Purpose**: Tests different rendering modes and validates visual differences.

**Commands tested**:
- `render solid`
- `render wireframe`

**Validation**:
- Each mode produces visible results
- Different modes show different color distributions
- Compares solid vs wireframe visual characteristics

## Running Tests

### Individual Tests
```bash
cd tests/cli_validation
./test_basic_voxel_placement.sh
./test_camera_views.sh
./test_resolution_switching.sh
./test_multiple_voxels.sh
./test_render_modes.sh
```

### All Tests
```bash
cd tests/cli_validation
./run_all_tests.sh
```

### Prerequisites
Ensure the voxel-cli is built:
```bash
cd ../../
cmake --build build
cd tests/cli_validation
```

## Test Output

Each test creates output in the `test_output/` directory:
- **Screenshots**: `.ppm` files showing rendered results
- **Color Analysis**: `.txt` files with pixel color distribution
- **Commands**: `.txt` files with the exact CLI commands executed
- **Comparison Data**: Files comparing results across test variations

### Example Output Structure
```
test_output/
├── basic_voxel.ppm
├── basic_voxel_colors.txt
├── basic_voxel_commands.txt
├── camera_front.ppm
├── camera_front_colors.txt
├── resolution_8cm.ppm
├── resolution_comparison.txt
└── render_mode_comparison.txt
```

## Validation Criteria

### Success Indicators
- **Non-zero voxel pixels**: Bright/colored pixels indicating voxel presence
- **Multiple unique colors**: Evidence of proper shading/lighting
- **Consistent behavior**: Repeatable results across runs
- **Expected differences**: Different settings produce visually different results

### Failure Indicators
- **All black/background**: No visible voxels rendered
- **CLI timeouts**: Commands hanging or not responding
- **Missing screenshots**: File I/O issues
- **Identical results**: Different settings producing identical outputs

## Troubleshooting

### Common Issues

1. **CLI not found**
   ```bash
   # Build the project first
   cd ../../
   cmake --build build
   ```

2. **Tests timeout**
   - Check if CLI is hanging on input
   - Verify display/OpenGL context availability
   - Ensure `quit` command is in command scripts

3. **No voxels visible**
   - Check if voxels are placed within workspace bounds
   - Verify camera position can see voxel locations
   - Test with different zoom levels

4. **Permission errors**
   ```bash
   chmod +x *.sh
   ```

### Debug Mode
To debug failing tests, examine the generated files:
```bash
# View the actual commands sent to CLI
cat test_output/basic_voxel_commands.txt

# View color analysis results
cat test_output/basic_voxel_colors.txt

# View screenshot (if you have image viewer)
open test_output/basic_voxel.ppm
```

## Extending the Test Suite

### Adding New Tests

1. **Create test script**: Follow naming pattern `test_<feature>.sh`
2. **Set executable**: `chmod +x test_<feature>.sh`
3. **Follow pattern**:
   - Set up test environment
   - Create command file
   - Execute CLI with timeout
   - Analyze screenshot
   - Validate results
   - Report pass/fail

### Test Script Template
```bash
#!/bin/bash
set -e

TEST_NAME="Your Test Name"
OUTPUT_DIR="test_output"
SCREENSHOT_FILE="$OUTPUT_DIR/your_test.ppm"

echo "Running $TEST_NAME..."
mkdir -p "$OUTPUT_DIR"

# Create CLI commands
cat > "$OUTPUT_DIR/commands.txt" << 'EOF'
# Your CLI commands here
quit
EOF

# Execute CLI
timeout 30s ../../voxel-cli < "$OUTPUT_DIR/commands.txt"

# Analyze results
../../tools/analyze_ppm_colors.py "$SCREENSHOT_FILE" > "$OUTPUT_DIR/analysis.txt"

# Validate and report results
# Your validation logic here
echo "PASS: Test completed successfully!"
```

## Integration with CI/CD

These tests are designed to be CI/CD friendly:
- Exit codes indicate pass/fail status
- Minimal dependencies (standard Unix tools)
- Configurable timeouts
- Clear output formatting
- Deterministic behavior

Add to CI pipeline:
```yaml
- name: Run CLI Validation Tests
  run: |
    cd tests/cli_validation
    ./run_all_tests.sh
```