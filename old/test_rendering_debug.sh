#!/bin/bash
# Debug test for voxel rendering

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_ROOT/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

# Create output directory
TEST_OUTPUT="$BUILD_DIR/rendering_debug"
mkdir -p "$TEST_OUTPUT"

echo "=== Voxel Rendering Debug Test ==="
echo "Testing with different configurations..."

# Test 1: Single large red voxel at center
echo -e "\nTest 1: Large red voxel at center"
(
    echo "workspace 2 2 2"      # Small 2m workspace
    echo "resolution 64cm"      # Large 64cm voxels
    echo "place 1 1 1"          # Place at center
    echo "camera front"
    echo "zoom 2.0"
    sleep 1
    echo "screenshot $TEST_OUTPUT/test1_large_red.ppm"
    sleep 1
    echo "quit"
) | "$VOXEL_CLI" 2>&1 | tee "$TEST_OUTPUT/test1_output.log"

# Test 2: Multiple voxels in a pattern
echo -e "\nTest 2: Multiple voxels pattern"
(
    echo "workspace 5 5 5"
    echo "resolution 16cm"
    echo "fill 10 10 10 20 20 20"  # Create a 11x11x11 cube
    echo "camera iso"
    echo "zoom 1.5"
    sleep 1
    echo "screenshot $TEST_OUTPUT/test2_cube_pattern.ppm"
    sleep 1
    echo "quit"
) | "$VOXEL_CLI" 2>&1 | tee "$TEST_OUTPUT/test2_output.log"

# Test 3: Check OpenGL state
echo -e "\nTest 3: OpenGL state check"
(
    echo "workspace 3 3 3"
    echo "resolution 32cm"
    echo "place 5 5 5"
    echo "camera front"
    sleep 1
    echo "screenshot $TEST_OUTPUT/test3_gl_state.ppm"
    sleep 1
    echo "quit"
) | "$VOXEL_CLI" 2>&1 | grep -E "(OpenGL|GL_|Shader|shader|ERROR|Warning)" | tee "$TEST_OUTPUT/test3_gl_output.log"

# Analyze screenshots
echo -e "\n=== Analyzing Screenshots ==="
for screenshot in "$TEST_OUTPUT"/*.ppm; do
    if [ -f "$screenshot" ]; then
        echo -e "\nAnalyzing $(basename "$screenshot"):"
        python3 - <<EOF
import os

filename = '$screenshot'
with open(filename, 'rb') as f:
    # Read PPM header
    magic = f.readline().decode('ascii').strip()
    dimensions = f.readline().decode('ascii').strip()
    maxval = f.readline().decode('ascii').strip()
    
    width, height = map(int, dimensions.split())
    pixels = f.read(width * height * 3)
    
    # Analyze pixel distribution
    pixel_counts = {}
    for i in range(0, len(pixels), 3):
        r, g, b = pixels[i], pixels[i+1], pixels[i+2]
        color = (r, g, b)
        pixel_counts[color] = pixel_counts.get(color, 0) + 1
    
    # Sort by frequency
    sorted_colors = sorted(pixel_counts.items(), key=lambda x: x[1], reverse=True)
    
    print(f"  Image size: {width}x{height}")
    print(f"  Unique colors: {len(pixel_counts)}")
    print("  Top 5 colors (RGB, count, percentage):")
    total_pixels = width * height
    for color, count in sorted_colors[:5]:
        percentage = 100.0 * count / total_pixels
        print(f"    {color}: {count} pixels ({percentage:.2f}%)")
    
    # Check if only background color
    if len(pixel_counts) == 1:
        print("  WARNING: Only one color found - no voxels visible!")
    elif len(pixel_counts) < 5:
        print("  WARNING: Very few colors - possible rendering issue")
EOF
    fi
done

echo -e "\n=== Checking Logs ==="
echo "Test 1 output:"
grep -E "(vertices|Mesh|Created|shader)" "$TEST_OUTPUT/test1_output.log" | tail -10

echo -e "\nTest 2 output:"
grep -E "(vertices|Mesh|Created|shader)" "$TEST_OUTPUT/test2_output.log" | tail -10

echo -e "\nOpenGL state from Test 3:"
cat "$TEST_OUTPUT/test3_gl_output.log"

echo -e "\n=== Done ==="