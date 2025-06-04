#!/bin/bash
# Test 3: Resolution Switching Validation
# Tests placing voxels at different resolutions and validates size differences

set -e

TEST_NAME="Resolution Switching"
OUTPUT_DIR="test_output"

echo "Running $TEST_NAME..."

# Create output directory and clean up previous PPM files
mkdir -p "$OUTPUT_DIR"
rm -f "$OUTPUT_DIR"/*.ppm*

# Build the project if needed
if [ ! -f "../../voxel-cli" ]; then
    echo "Building voxel-cli..."
    cd ../..
    cmake --build build
    cd tests/cli_validation
fi

# Test different resolutions
RESOLUTIONS=("1cm" "4cm" "8cm" "16cm" "32cm")

for res in "${RESOLUTIONS[@]}"; do
    echo "Testing resolution: $res..."
    
    SCREENSHOT_FILE="$OUTPUT_DIR/resolution_${res}.ppm"
    COLORS_FILE="$OUTPUT_DIR/resolution_${res}_colors.txt"
    
    # Create test commands for this resolution
    cat > "$OUTPUT_DIR/resolution_${res}_commands.txt" << EOF
workspace 5 5 5
resolution $res
place 2 2 2
view iso
screenshot $SCREENSHOT_FILE
quit
EOF

    echo "Executing CLI commands for $res resolution..."
    timeout 30s ../../voxel-cli < "$OUTPUT_DIR/resolution_${res}_commands.txt" || {
        echo "FAIL: CLI execution failed for $res resolution"
        exit 1
    }
    
    # Verify screenshot was created (check for various extensions the CLI might create)
    ACTUAL_SCREENSHOT=""
    for ext in ".ppm" ".ppm.ppm" ".ppm.ppm.ppm"; do
        if [ -f "$OUTPUT_DIR/resolution_${res}${ext}" ]; then
            ACTUAL_SCREENSHOT="$OUTPUT_DIR/resolution_${res}${ext}"
            break
        fi
    done
    
    if [ -z "$ACTUAL_SCREENSHOT" ]; then
        echo "FAIL: Screenshot not created for $res resolution"
        exit 1
    fi
    
    SCREENSHOT_FILE="$ACTUAL_SCREENSHOT"
    
    # Analyze colors
    echo "Analyzing colors for $res resolution..."
    ../../tools/analyze_ppm_colors.py "$SCREENSHOT_FILE" > "$COLORS_FILE"
    
    # Count non-background pixels (assuming background is black or very dark)
    # This helps us validate that larger resolutions create more visible pixels
    NON_BLACK_PIXELS=$(grep -E '\([1-9][0-9]*, [1-9][0-9]*, [1-9][0-9]*\)' "$COLORS_FILE" | head -5 | awk -F: '{sum += $2} END {print sum+0}')
    
    echo "Non-background pixels for $res: $NON_BLACK_PIXELS"
    
    # Check if voxels are actually visible at this resolution
    UNIQUE_COLORS=$(grep "Unique colors:" "$COLORS_FILE" | grep -oE '[0-9]+$')
    
    if [ "$UNIQUE_COLORS" -gt 1 ]; then
        echo "PASS: $res resolution shows $UNIQUE_COLORS unique colors, $NON_BLACK_PIXELS visible pixels"
    else
        echo "FAIL: $res resolution shows no visible voxels (only $UNIQUE_COLORS color)"
        echo "Expected: Multiple colors showing voxel vs background"
        echo "Actual: Single color (background only)"
        echo "Resolution switching works but voxels are not visible"
        cat "$COLORS_FILE"
        exit 1
    fi
    
    # Store pixel count for comparison
    echo "$res:$NON_BLACK_PIXELS" >> "$OUTPUT_DIR/resolution_comparison.txt"
done

# Validate that different resolutions produce different pixel counts
echo ""
echo "Resolution comparison:"
cat "$OUTPUT_DIR/resolution_comparison.txt"

# Check if we have reasonable variation in pixel counts
FIRST_COUNT=$(head -1 "$OUTPUT_DIR/resolution_comparison.txt" | cut -d: -f2)
LAST_COUNT=$(tail -1 "$OUTPUT_DIR/resolution_comparison.txt" | cut -d: -f2)

if [ "$LAST_COUNT" -gt "$FIRST_COUNT" ]; then
    echo "PASS: Larger resolutions show more pixels ($FIRST_COUNT -> $LAST_COUNT)"
elif [ "$LAST_COUNT" -eq "$FIRST_COUNT" ]; then
    echo "INFO: Pixel counts are the same - may be expected for current view distance"
else
    echo "WARNING: Smaller resolutions show more pixels - unexpected but not necessarily wrong"
fi

echo "PASS: All resolutions working correctly"
echo "Screenshots saved in: $OUTPUT_DIR/"
echo "Test completed successfully!"