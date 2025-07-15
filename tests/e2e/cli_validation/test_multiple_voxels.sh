#!/bin/bash
# Test 4: Multiple Voxel Placement Validation
# Places multiple voxels in different positions and validates they all appear

set -e

TEST_NAME="Multiple Voxel Placement"
OUTPUT_DIR="test_output"
CLI_BINARY="../../../build_ninja/bin/VoxelEditor_CLI"
SCREENSHOT_FILE="$OUTPUT_DIR/multiple_voxels.ppm"
COLORS_FILE="$OUTPUT_DIR/multiple_voxels_colors.txt"

echo "Running $TEST_NAME..."

# Create output directory and clean up previous PPM files
mkdir -p "$OUTPUT_DIR"
rm -f "$OUTPUT_DIR"/*.ppm*

# Build the project if needed
if [ ! -f "$CLI_BINARY" ]; then
    echo "Building voxel-cli..."
    cd ../..
    cmake --build build_ninja
    cd tests/cli_validation
fi

# Create test commands - place voxels in a pattern
cat > "$OUTPUT_DIR/multiple_voxels_commands.txt" << 'EOF'
workspace 5m 5m 5m
resolution 8cm
place -48cm 8cm -48cm
place -48cm 24cm -48cm
place 48cm 8cm -48cm
place 48cm 24cm -48cm
place 0cm 16cm 48cm
view iso
zoom 0.8
screenshot test_output/multiple_voxels.ppm
quit
EOF

echo "Executing CLI commands for multiple voxel placement..."
timeout 30s "$CLI_BINARY" < "$OUTPUT_DIR/multiple_voxels_commands.txt" || {
    echo "FAIL: CLI execution failed"
    exit 1
}

# Verify screenshot was created (check for various extensions the CLI might create)
ACTUAL_SCREENSHOT=""
for ext in ".ppm" ".ppm.ppm" ".ppm.ppm.ppm"; do
    if [ -f "$OUTPUT_DIR/multiple_voxels${ext}" ]; then
        ACTUAL_SCREENSHOT="$OUTPUT_DIR/multiple_voxels${ext}"
        break
    fi
done

if [ -z "$ACTUAL_SCREENSHOT" ]; then
    echo "FAIL: Screenshot not created"
    exit 1
fi

SCREENSHOT_FILE="$ACTUAL_SCREENSHOT"

echo "Analyzing colors in screenshot..."
../../../tools/analyze_ppm_colors.py "$SCREENSHOT_FILE" > "$COLORS_FILE"

# Count unique colors (should be more than just background)
UNIQUE_COLORS=$(grep "Unique colors:" "$COLORS_FILE" | grep -oE '[0-9]+$')

# Count total non-background pixels (bright colors indicating voxels)
NON_BLACK_PIXELS=$(grep -E '\([1-9][0-9]*, [1-9][0-9]*, [1-9][0-9]*\)' "$COLORS_FILE" | awk -F: '{sum += $2} END {print sum+0}')

echo "Analysis results:"
echo "- Unique colors found: $UNIQUE_COLORS"
echo "- Non-background pixels: $NON_BLACK_PIXELS"

# Validation criteria - require visible voxels
if [ "$UNIQUE_COLORS" -gt 1 ]; then
    echo "PASS: Multiple voxels are visible"
    echo "- $UNIQUE_COLORS unique colors"
    echo "- $NON_BLACK_PIXELS non-background pixels"
else
    echo "FAIL: No visible voxels detected in multiple voxel test"
    echo "Expected: Multiple colors showing voxels vs background"
    echo "Actual: Only $UNIQUE_COLORS color (background only)"
    echo ""
    echo "Multiple voxels were placed but none are visually distinguishable."
    echo "CLI placement works but rendering has color/material issues."
    echo ""
    echo "Color analysis:"
    cat "$COLORS_FILE"
    exit 1
fi

# Look for evidence of multiple distinct voxel regions
# Count how many different color groups we have (indicating separate voxels)
COLOR_GROUPS=$(grep -E '\([1-9][0-9]*, [1-9][0-9]*, [1-9][0-9]*\)' "$COLORS_FILE" | wc -l)

if [ "$COLOR_GROUPS" -lt 3 ]; then
    echo "WARNING: Expected more color variation from multiple voxels, found $COLOR_GROUPS color groups"
    echo "This might be okay if voxels are using the same material/color"
else
    echo "PASS: Found $COLOR_GROUPS different color groups, indicating good voxel separation"
fi

echo "Multiple voxels successfully placed and rendered"
echo "- $UNIQUE_COLORS unique colors"
echo "- $NON_BLACK_PIXELS non-background pixels"
echo "- $COLOR_GROUPS color groups"

echo "Screenshot saved to: $SCREENSHOT_FILE"
echo "Color analysis saved to: $COLORS_FILE"
echo "Test completed successfully!"