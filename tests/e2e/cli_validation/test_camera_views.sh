#!/bin/bash
# Test 2: Camera View Changes Validation
# Tests different camera views and validates voxel visibility from different angles

set -e

TEST_NAME="Camera View Changes"
OUTPUT_DIR="test_output"
CLI_BINARY="../../../build_ninja/bin/VoxelEditor_CLI"

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

# Test different camera views
VIEWS=("front" "back" "top" "bottom" "left" "right" "iso")

for view in "${VIEWS[@]}"; do
    echo "Testing $view view..."
    
    SCREENSHOT_FILE="$OUTPUT_DIR/camera_${view}.ppm"
    COLORS_FILE="$OUTPUT_DIR/camera_${view}_colors.txt"
    
    # Create test commands for this view
    cat > "$OUTPUT_DIR/camera_${view}_commands.txt" << EOF
workspace 5.0 5.0 5.0
resolution 8cm
place 2cm 2cm 2cm
view $view
screenshot $SCREENSHOT_FILE
quit
EOF

    echo "Executing CLI commands for $view view..."
    timeout 30s "$CLI_BINARY" < "$OUTPUT_DIR/camera_${view}_commands.txt" || {
        echo "FAIL: CLI execution failed for $view view"
        exit 1
    }
    
    # Verify screenshot was created (check for various extensions the CLI might create)
    ACTUAL_SCREENSHOT=""
    for ext in ".ppm" ".ppm.ppm" ".ppm.ppm.ppm"; do
        if [ -f "$OUTPUT_DIR/camera_${view}${ext}" ]; then
            ACTUAL_SCREENSHOT="$OUTPUT_DIR/camera_${view}${ext}"
            break
        fi
    done
    
    if [ -z "$ACTUAL_SCREENSHOT" ]; then
        echo "FAIL: Screenshot not created for $view view"
        exit 1
    fi
    
    SCREENSHOT_FILE="$ACTUAL_SCREENSHOT"
    
    # Analyze colors
    echo "Analyzing colors for $view view..."
    ../../../tools/analyze_ppm_colors.py "$SCREENSHOT_FILE" > "$COLORS_FILE"
    
    # Check if voxels are actually visible from this view
    UNIQUE_COLORS=$(grep "Unique colors:" "$COLORS_FILE" | grep -oE '[0-9]+$')
    
    if [ "$UNIQUE_COLORS" -gt 1 ]; then
        echo "PASS: $view view shows $UNIQUE_COLORS unique colors"
    else
        echo "FAIL: $view view shows no visible voxels (only $UNIQUE_COLORS color)"
        echo "Expected: Multiple colors indicating voxel vs background"
        echo "Actual: Single color (background only)"
        echo "Camera view changed but voxels are not visible"
        cat "$COLORS_FILE"
        exit 1
    fi
done

echo "PASS: All camera views working correctly"
echo "Screenshots saved in: $OUTPUT_DIR/"
echo "Test completed successfully!"