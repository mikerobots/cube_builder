#!/bin/bash
# Minimal render test to debug voxel visibility

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

# Create output directory
TEST_OUTPUT="$BUILD_DIR/minimal_render_test"
mkdir -p "$TEST_OUTPUT"

echo "=== Minimal Render Test ==="

# Test 1: Single voxel after workspace command
echo -e "\nTest 1: Immediate voxel placement"
(
    echo "workspace 2 2 2"
    echo "resolution 64cm"
    echo "place 1 1 1"
    echo "camera front"
    echo "zoom 5.0"  # Very close zoom
    sleep 2
    echo "screenshot $TEST_OUTPUT/test1.ppm"
    echo "quit"
) | "$VOXEL_CLI" 2>&1 | grep -E "(Camera set|world pos|vertices|shader)" | tail -20

# Check if anything changed
echo -e "\nChecking screenshot colors..."
python3 - <<EOF
filename = '$TEST_OUTPUT/test1.ppm.ppm'
try:
    with open(filename, 'rb') as f:
        # Skip header
        f.readline(); f.readline(); f.readline()
        
        # Read all pixels
        pixels = f.read()
        
        # Find unique colors
        colors = set()
        for i in range(0, len(pixels), 3):
            colors.add((pixels[i], pixels[i+1], pixels[i+2]))
        
        print(f"Unique colors found: {len(colors)}")
        for color in list(colors)[:10]:
            print(f"  RGB{color}")
            
        # Check specific areas
        width, height = 2560, 1440
        
        # Check corners
        corners = [
            (0, 0, "top-left"),
            (width-1, 0, "top-right"),
            (0, height-1, "bottom-left"),
            (width-1, height-1, "bottom-right"),
            (width//2, height//2, "center")
        ]
        
        print("\nPixel samples:")
        for x, y, name in corners:
            idx = (y * width + x) * 3
            if idx + 2 < len(pixels):
                r, g, b = pixels[idx], pixels[idx+1], pixels[idx+2]
                print(f"  {name}: RGB({r}, {g}, {b})")
                
except Exception as e:
    print(f"Error: {e}")
EOF

# Test 2: Check OpenGL state during rendering
echo -e "\n\nTest 2: OpenGL state check"
# Add temporary debug output to shader
(
    echo "workspace 1 1 1"
    echo "resolution 64cm"  # Big voxel in small space
    echo "place 0 0 0"
    echo "camera front"
    echo "zoom 10.0"  # Extremely close
    sleep 1
    echo "screenshot $TEST_OUTPUT/test2.ppm"
    echo "quit"
) | "$VOXEL_CLI" 2>&1 > "$TEST_OUTPUT/full_log.txt"

# Check for any OpenGL errors
echo -e "\nOpenGL-related output:"
grep -i "gl\|shader\|texture\|error" "$TEST_OUTPUT/full_log.txt" | grep -v "GLFW" | tail -20