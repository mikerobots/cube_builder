#!/bin/bash
# Test 1: Basic Voxel Placement Validation
# Places a single yellow voxel and validates it appears in the screenshot

set -e

TEST_NAME="Basic Voxel Placement"
OUTPUT_DIR="test_output"
SCREENSHOT_FILE="$OUTPUT_DIR/basic_voxel.ppm"
EXPECTED_COLORS_FILE="$OUTPUT_DIR/basic_voxel_colors.txt"

echo "Running $TEST_NAME..."

# Create output directory and clean up previous PPM files
mkdir -p "$OUTPUT_DIR"
rm -f "$OUTPUT_DIR"/*.ppm*

# Build the project if needed
CLI_BINARY="../../../build_ninja/bin/VoxelEditor_CLI"
if [ ! -f "$CLI_BINARY" ]; then
    echo "Building voxel-cli..."
    cd ../../..
    cmake --build build_ninja
    cd tests/e2e/cli_validation
fi

# Create test commands
cat > "$OUTPUT_DIR/basic_voxel_commands.txt" << 'EOF'
workspace 5.0 5.0 5.0
resolution 8cm
place 0cm 0cm 0cm
screenshot test_output/basic_voxel.ppm
quit
EOF

echo "Executing CLI commands..."
timeout 30s "$CLI_BINARY" < "$OUTPUT_DIR/basic_voxel_commands.txt" || {
    echo "CLI execution timed out or failed"
    exit 1
}

# Verify screenshot was created (check for various extensions the CLI might create)
ACTUAL_SCREENSHOT=""
for ext in ".ppm" ".ppm.ppm" ".ppm.ppm.ppm"; do
    if [ -f "$OUTPUT_DIR/basic_voxel${ext}" ]; then
        ACTUAL_SCREENSHOT="$OUTPUT_DIR/basic_voxel${ext}"
        break
    fi
done

if [ -z "$ACTUAL_SCREENSHOT" ]; then
    echo "FAIL: Screenshot not created"
    echo "Expected one of: $OUTPUT_DIR/basic_voxel.ppm, $OUTPUT_DIR/basic_voxel.ppm.ppm, $OUTPUT_DIR/basic_voxel.ppm.ppm.ppm"
    ls -la "$OUTPUT_DIR"/basic_voxel* 2>/dev/null || echo "No files found"
    exit 1
fi

SCREENSHOT_FILE="$ACTUAL_SCREENSHOT"

echo "Analyzing colors in screenshot..."
../../../tools/analyze_ppm_colors.py "$SCREENSHOT_FILE" > "$EXPECTED_COLORS_FILE"

# Check if voxels are actually visible - require multiple colors for PASS
TOTAL_COLORS=$(grep "Unique colors:" "$EXPECTED_COLORS_FILE" | grep -oE '[0-9]+$')

# Look for non-background colored pixels
NON_BLACK_COLORS=$(grep -E '\([1-9][0-9]*, [1-9][0-9]*, [1-9][0-9]*\)' "$EXPECTED_COLORS_FILE" | wc -l)

if [ "$TOTAL_COLORS" -gt 1 ]; then
    echo "PASS: Found visible voxel rendering - $TOTAL_COLORS unique colors"
    echo "Test completed successfully!"
else
    echo "FAIL: No visible voxel rendering detected"
    echo "Expected: Multiple colors indicating voxel vs background"
    echo "Actual: Only $TOTAL_COLORS color found (background only)"
    echo ""
    echo "This means voxels are not visually distinguishable from background."
    echo "The CLI works but voxels are invisible due to color/material issues."
    echo ""
    echo "Color analysis:"
    cat "$EXPECTED_COLORS_FILE"
    exit 1
fi

echo "Screenshot saved to: $SCREENSHOT_FILE"
echo "Color analysis saved to: $EXPECTED_COLORS_FILE"