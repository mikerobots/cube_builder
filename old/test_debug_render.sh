#!/bin/bash
# Debug render test with detailed output

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

# Create output directory
TEST_OUTPUT="$BUILD_DIR/debug_render_test"
mkdir -p "$TEST_OUTPUT"

echo "=== Debug Render Test ==="

# Test with a single voxel and capture all output
(
    echo "workspace 2 2 2"
    echo "resolution 32cm"
    echo "place 3 3 3"
    echo "camera front"
    echo "zoom 4.0"
    sleep 2
    echo "screenshot $TEST_OUTPUT/debug_test.ppm"
    echo "quit"
) | "$VOXEL_CLI" 2>&1 | tee "$TEST_OUTPUT/full_debug.log"

# Extract relevant information
echo -e "\n=== Shader Information ==="
grep -i "shader\|uniform" "$TEST_OUTPUT/full_debug.log"

echo -e "\n=== Mesh Information ==="
grep -E "mesh|vertices|Mesh" "$TEST_OUTPUT/full_debug.log"

echo -e "\n=== OpenGL Errors ==="
grep -i "error\|warning" "$TEST_OUTPUT/full_debug.log" | grep -v "UNSUPPORTED"

# Analyze the screenshot pixel by pixel
echo -e "\n=== Pixel Analysis ==="
python3 - <<EOF
import os

filename = '$TEST_OUTPUT/debug_test.ppm.ppm'
if os.path.exists(filename):
    with open(filename, 'rb') as f:
        # Read header
        magic = f.readline()
        dimensions = f.readline()
        maxval = f.readline()
        
        width, height = map(int, dimensions.decode('ascii').strip().split())
        
        # Read all pixels
        pixels = f.read(width * height * 3)
        
        # Create a histogram of colors
        color_hist = {}
        for i in range(0, len(pixels), 3):
            color = (pixels[i], pixels[i+1], pixels[i+2])
            color_hist[color] = color_hist.get(color, 0) + 1
        
        # Sort by frequency
        sorted_colors = sorted(color_hist.items(), key=lambda x: x[1], reverse=True)
        
        print(f"Image size: {width}x{height}")
        print(f"Total unique colors: {len(color_hist)}")
        print("\nTop 10 colors by frequency:")
        for color, count in sorted_colors[:10]:
            percentage = 100.0 * count / (width * height)
            print(f"  RGB{color}: {count} pixels ({percentage:.2f}%)")
            
        # Check if image is all one color
        if len(color_hist) == 1:
            print("\nWARNING: Image is entirely one color!")
        
        # Sample specific regions
        print("\nSampling specific regions:")
        regions = [
            ("Center", width//2, height//2),
            ("Top-left", 100, 100),
            ("Top-right", width-100, 100),
            ("Bottom-left", 100, height-100),
            ("Bottom-right", width-100, height-100)
        ]
        
        for name, x, y in regions:
            if 0 <= x < width and 0 <= y < height:
                idx = (y * width + x) * 3
                r, g, b = pixels[idx], pixels[idx+1], pixels[idx+2]
                print(f"  {name} ({x},{y}): RGB({r},{g},{b})")
EOF