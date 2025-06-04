#!/bin/bash
# Test script with larger voxels for better visibility

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/../../.."
BUILD_DIR="$PROJECT_ROOT/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

# Create a test output directory
TEST_OUTPUT="$BUILD_DIR/large_voxel_test"
mkdir -p "$TEST_OUTPUT"

echo "Testing with larger voxels..."

# Test with 64cm voxels (much larger)
echo -e "\n=== Test with 64cm voxels ==="
(
    echo "workspace 5 5 5"
    echo "resolution 64cm"  # Much larger voxels
    echo "place 3 3 3"      # Center of 5m workspace with 64cm voxels
    echo "place 4 3 3"
    echo "place 3 4 3"
    echo "place 3 3 4"
    echo "camera front"
    echo "zoom 2.0"  # Zoom in
    sleep 1
    echo "screenshot $TEST_OUTPUT/large_voxels_front.ppm"
    echo "camera iso"
    sleep 1
    echo "screenshot $TEST_OUTPUT/large_voxels_iso.ppm"
    sleep 1
    echo "quit"
) | "$VOXEL_CLI"

# Test with filled area
echo -e "\n=== Test with filled area ==="
(
    echo "workspace 5 5 5"
    echo "resolution 32cm"  # 32cm voxels
    echo "fill 5 5 5 10 10 10"  # 6x6x6 cube
    echo "camera iso"
    echo "zoom 1.5"
    sleep 1
    echo "screenshot $TEST_OUTPUT/filled_area.ppm"
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
    
    # Check for non-gray pixels (voxels should be brighter)
    non_gray_pixels = 0
    max_brightness = 0
    
    for i in range(0, len(pixels), 3):
        r, g, b = pixels[i], pixels[i+1], pixels[i+2]
        brightness = max(r, g, b)
        max_brightness = max(max_brightness, brightness)
        
        # Check if not background gray (51)
        if abs(r - 51) > 5 or abs(g - 51) > 5 or abs(b - 51) > 5:
            non_gray_pixels += 1
    
    total_pixels = width * height
    percent_non_gray = 100 * non_gray_pixels / total_pixels
    
    print(f"  Non-gray pixels: {percent_non_gray:.2f}%")
    print(f"  Max brightness: {max_brightness}")
    
    if percent_non_gray > 0.1:
        print("  ✓ Voxels visible!")
    else:
        print("  ✗ No voxels visible")
EOF
    fi
done

echo -e "\n=== Done ==="