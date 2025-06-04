#!/bin/bash
# Test voxel placement and mesh generation

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_ROOT/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

# Create output directory
TEST_OUTPUT="$BUILD_DIR/voxel_placement_test"
mkdir -p "$TEST_OUTPUT"

echo "=== Testing Voxel Placement and Mesh Generation ==="

# Test with explicit place commands
echo -e "\nTest: Place individual voxels"
(
    echo "workspace 3 3 3"
    echo "resolution 32cm"
    echo "place 1 1 1"
    sleep 0.5
    echo "place 2 1 1"
    sleep 0.5
    echo "place 3 1 1"
    sleep 0.5
    echo "place 1 2 1"
    sleep 0.5
    echo "camera front"
    echo "zoom 2.0"
    sleep 1
    echo "screenshot $TEST_OUTPUT/individual_voxels.ppm"
    sleep 1
    echo "quit"
) | "$VOXEL_CLI" 2>&1 | grep -E "(Found|vertices|Mesh|voxels to render|Voxel placed)"

echo -e "\n=== Analyzing Screenshot ==="
if [ -f "$TEST_OUTPUT/individual_voxels.ppm.ppm" ]; then
    python3 - <<EOF
with open('$TEST_OUTPUT/individual_voxels.ppm.ppm', 'rb') as f:
    # Skip header
    f.readline(); f.readline(); f.readline()
    
    # Read pixels
    pixels = f.read(2560 * 1440 * 3)
    
    # Count unique colors
    unique_colors = set()
    for i in range(0, len(pixels), 3):
        unique_colors.add((pixels[i], pixels[i+1], pixels[i+2]))
    
    print(f"Unique colors: {len(unique_colors)}")
    if len(unique_colors) == 1:
        print("WARNING: Only background color - no voxels visible!")
    else:
        print("SUCCESS: Multiple colors found - voxels may be visible")
        # Show non-background colors
        for color in list(unique_colors)[:5]:
            if color != (51, 51, 51):
                print(f"  Non-background color: RGB{color}")
EOF
fi