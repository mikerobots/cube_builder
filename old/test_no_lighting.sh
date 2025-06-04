#!/bin/bash
# Test rendering without lighting

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

# Create output directory
TEST_OUTPUT="$BUILD_DIR/no_lighting_test"
mkdir -p "$TEST_OUTPUT"

echo "=== Testing Without Lighting ==="

# Temporarily patch the RenderEngine to disable lighting
RENDER_ENGINE="$SCRIPT_DIR/core/rendering/RenderEngine.cpp"

# Create a simple test that should show voxels
echo -e "\nPlacing voxels and capturing screenshot..."
(
    echo "workspace 2 2 2"
    echo "resolution 32cm"
    echo "place 3 3 3"    # Center voxel
    echo "camera front"
    echo "zoom 3.0"
    sleep 1
    echo "screenshot $TEST_OUTPUT/test_voxel.ppm"
    echo "info"  # Show some debug info
    sleep 1
    echo "quit"
) | "$VOXEL_CLI" 2>&1 | tee "$TEST_OUTPUT/output.log"

# Check the screenshot
echo -e "\n=== Analyzing Screenshot ==="
python3 - <<EOF
import os

filename = '$TEST_OUTPUT/test_voxel.ppm.ppm'
if os.path.exists(filename):
    with open(filename, 'rb') as f:
        # Read PPM header
        magic = f.readline()
        dimensions = f.readline()
        maxval = f.readline()
        
        width, height = map(int, dimensions.decode('ascii').strip().split())
        
        # Read all pixels
        pixels = f.read(width * height * 3)
        
        # Count unique colors
        unique_colors = set()
        bright_pixels = 0
        
        for i in range(0, len(pixels), 3):
            r, g, b = pixels[i], pixels[i+1], pixels[i+2]
            unique_colors.add((r, g, b))
            
            # Count pixels brighter than background
            if r > 60 or g > 60 or b > 60:
                bright_pixels += 1
        
        print(f"Image size: {width}x{height}")
        print(f"Unique colors: {len(unique_colors)}")
        print(f"Bright pixels: {bright_pixels} ({100.0 * bright_pixels / (width * height):.2f}%)")
        
        # Show top colors
        color_count = {}
        for i in range(0, len(pixels), 3):
            color = (pixels[i], pixels[i+1], pixels[i+2])
            color_count[color] = color_count.get(color, 0) + 1
        
        print("\nTop 5 colors:")
        for color, count in sorted(color_count.items(), key=lambda x: x[1], reverse=True)[:5]:
            print(f"  RGB{color}: {count} pixels")
else:
    print(f"Screenshot not found: {filename}")
EOF

# Check logs for shader info
echo -e "\n=== Shader Information ==="
grep -i "shader\|uniform\|lighting" "$TEST_OUTPUT/output.log" | tail -20

echo -e "\n=== Mesh Information ==="
grep -E "vertices|Mesh|Found.*voxels" "$TEST_OUTPUT/output.log" | tail -10