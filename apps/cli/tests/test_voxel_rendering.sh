#!/bin/bash
# Test script to debug voxel rendering

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/../../.."
BUILD_DIR="$PROJECT_ROOT/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

# Create a test output directory
TEST_OUTPUT="$BUILD_DIR/voxel_render_test"
mkdir -p "$TEST_OUTPUT"

echo "Testing voxel rendering..."

# Test 1: Single voxel at origin
echo -e "\n=== Test 1: Single voxel at origin ==="
(
    echo "workspace 5 5 5"
    echo "resolution 8cm"
    echo "place 0 0 0"  # Origin
    echo "camera front"
    sleep 1
    echo "screenshot $TEST_OUTPUT/test1_origin.ppm"
    sleep 1
    echo "quit"
) | "$VOXEL_CLI"

# Test 2: Voxels at workspace center
echo -e "\n=== Test 2: Voxels at workspace center ==="
(
    echo "workspace 5 5 5"
    echo "resolution 8cm" 
    # 5m workspace with 8cm voxels = 62.5 voxels, so center is around 31
    echo "place 31 31 31"  
    echo "place 32 31 31"
    echo "place 31 32 31"
    echo "place 31 31 32"
    echo "camera iso"
    sleep 1
    echo "screenshot $TEST_OUTPUT/test2_center.ppm"
    sleep 1
    echo "quit"
) | "$VOXEL_CLI"

# Test 3: Large cube of voxels
echo -e "\n=== Test 3: Large cube of voxels ==="
(
    echo "workspace 5 5 5"
    echo "resolution 8cm"
    echo "fill 30 30 30 35 35 35"  # 6x6x6 cube near center
    echo "camera iso"
    sleep 1
    echo "screenshot $TEST_OUTPUT/test3_cube.ppm"
    sleep 1
    echo "quit"
) | "$VOXEL_CLI"

# Test 4: Different camera angles
echo -e "\n=== Test 4: Different camera views ==="
(
    echo "workspace 5 5 5"
    echo "resolution 8cm"
    echo "fill 25 25 25 37 37 37"  # 13x13x13 cube centered
    sleep 1
    echo "camera front"
    sleep 1
    echo "screenshot $TEST_OUTPUT/test4_front.ppm"
    echo "camera top"
    sleep 1
    echo "screenshot $TEST_OUTPUT/test4_top.ppm"
    echo "camera iso"
    sleep 1
    echo "screenshot $TEST_OUTPUT/test4_iso.ppm"
    sleep 1
    echo "quit"
) | "$VOXEL_CLI"

# Analyze results
echo -e "\n=== Analyzing screenshots ==="

for screenshot in "$TEST_OUTPUT"/*.ppm; do
    if [ -f "$screenshot" ]; then
        echo -e "\n$(basename "$screenshot"):"
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
    
    # Calculate statistics
    total_pixels = width * height
    
    # Count pixels by brightness
    dark_pixels = 0  # < 40
    gray_pixels = 0  # 40-60
    bright_pixels = 0  # > 60
    
    for i in range(0, len(pixels), 3):
        r, g, b = pixels[i], pixels[i+1], pixels[i+2]
        brightness = (r + g + b) / 3
        
        if brightness < 40:
            dark_pixels += 1
        elif brightness <= 60:
            gray_pixels += 1
        else:
            bright_pixels += 1
    
    # Average color
    avg_r = sum(pixels[i] for i in range(0, len(pixels), 3)) / total_pixels
    avg_g = sum(pixels[i] for i in range(1, len(pixels), 3)) / total_pixels
    avg_b = sum(pixels[i] for i in range(2, len(pixels), 3)) / total_pixels
    
    print(f"  Dimensions: {width}x{height}")
    print(f"  Average color: R={avg_r:.1f} G={avg_g:.1f} B={avg_b:.1f}")
    print(f"  Dark pixels (<40): {100*dark_pixels/total_pixels:.1f}%")
    print(f"  Gray pixels (40-60): {100*gray_pixels/total_pixels:.1f}%")
    print(f"  Bright pixels (>60): {100*bright_pixels/total_pixels:.1f}%")
    
    # Check for voxel rendering
    if bright_pixels > total_pixels * 0.01:  # More than 1% bright pixels
        print("  ✓ Likely contains rendered voxels")
    else:
        print("  ✗ No visible voxels detected")
EOF
    fi
done

echo -e "\n=== Done ==="