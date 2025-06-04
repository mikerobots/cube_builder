#!/bin/bash
# Simple render test with debug output

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

# Create output directory
TEST_OUTPUT="$BUILD_DIR/render_simple_test"
mkdir -p "$TEST_OUTPUT"

echo "=== Simple Render Test ==="

# Run test with debug output
(
    echo "workspace 4 4 4"
    echo "resolution 64cm"
    echo "place 3 3 3"  # Big voxel in center
    echo "camera front"
    echo "zoom 2.0"
    sleep 1
    echo "screenshot $TEST_OUTPUT/center_voxel.ppm"
    sleep 1
    echo "quit"
) | "$VOXEL_CLI" 2>&1 | tee "$TEST_OUTPUT/full_output.log"

# Extract key information
echo -e "\n=== Key Debug Info ==="
echo "Voxel placement:"
grep -E "Voxel.*at grid pos|world pos" "$TEST_OUTPUT/full_output.log"

echo -e "\nMesh generation:"
grep -E "Generated mesh|Created render mesh" "$TEST_OUTPUT/full_output.log"

echo -e "\nCamera info:"
grep -E "Camera.*view|Setting up camera" "$TEST_OUTPUT/full_output.log"

echo -e "\nOpenGL errors:"
grep -i "error\|warning" "$TEST_OUTPUT/full_output.log" | grep -v "UNSUPPORTED"

# Analyze the screenshot
echo -e "\n=== Screenshot Analysis ==="
python3 - <<EOF
import os

filename = '$TEST_OUTPUT/center_voxel.ppm.ppm'
if os.path.exists(filename):
    with open(filename, 'rb') as f:
        # Read PPM header
        magic = f.readline()
        dimensions = f.readline()
        maxval = f.readline()
        
        width, height = map(int, dimensions.decode('ascii').strip().split())
        
        # Sample center pixels
        center_x = width // 2
        center_y = height // 2
        
        # Read center area (100x100 pixels)
        pixels = f.read(width * height * 3)
        
        # Check center area
        center_colors = set()
        for dy in range(-50, 50):
            for dx in range(-50, 50):
                x = center_x + dx
                y = center_y + dy
                if 0 <= x < width and 0 <= y < height:
                    idx = (y * width + x) * 3
                    if idx + 2 < len(pixels):
                        color = (pixels[idx], pixels[idx+1], pixels[idx+2])
                        center_colors.add(color)
        
        print(f"Image size: {width}x{height}")
        print(f"Colors in center 100x100 area: {len(center_colors)}")
        
        if len(center_colors) == 1:
            print("WARNING: Center area is uniform color - voxel not visible")
            print(f"  Color: RGB{list(center_colors)[0]}")
        else:
            print("SUCCESS: Multiple colors in center - voxel may be visible")
            for color in list(center_colors)[:5]:
                print(f"  Color: RGB{color}")
EOF