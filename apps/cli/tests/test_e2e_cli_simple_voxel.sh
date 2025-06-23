#!/bin/bash
# Simple voxel rendering test

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/../../.."
BUILD_DIR="$PROJECT_ROOT/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

# Create a test output directory
TEST_OUTPUT="$BUILD_DIR/simple_voxel_test"
mkdir -p "$TEST_OUTPUT"

echo "Simple voxel rendering test..."

# Test with a single voxel in the middle of the viewport
(
    echo "workspace 4 4 4"    # Smaller workspace
    echo "resolution 32cm"    # Larger voxels (32cm)
    echo "place 0cm 0cm 0cm"  # Place at origin (centered coordinate system)
    echo "camera front"
    echo "zoom 3.0"           # Zoom in closer
    sleep 2                   # Give more time to render
    echo "screenshot $TEST_OUTPUT/single_voxel.ppm"
    sleep 1
    echo "quit"
) | timeout 10 "$VOXEL_CLI" 2>&1 | grep -E "(Created shader|vertices|ERROR|Warning|Voxel placed)"

# Check if screenshot exists and analyze it
SCREENSHOT="$TEST_OUTPUT/single_voxel.ppm.ppm"
if [ -f "$SCREENSHOT" ]; then
    echo -e "\nAnalyzing screenshot..."
    python3 - <<EOF
with open('$SCREENSHOT', 'rb') as f:
    # Skip header
    f.readline(); f.readline(); f.readline()
    
    # Read some pixels from center
    width, height = 2560, 1440
    center_start = (height//2 * width + width//2 - 50) * 3
    f.seek(center_start + 13)  # Skip header and go to center
    
    center_pixels = f.read(300)  # Read 100 pixels from center
    
    # Check if any pixels are not gray
    non_gray = False
    for i in range(0, len(center_pixels), 3):
        r, g, b = center_pixels[i], center_pixels[i+1], center_pixels[i+2]
        if abs(r - 51) > 10 or abs(g - 51) > 10 or abs(b - 51) > 10:
            non_gray = True
            print(f"Found non-gray pixel: R={r} G={g} B={b}")
            break
    
    if not non_gray:
        print("Center pixels are all gray (no voxel visible)")
        exit(1)
    else:
        print("✓ Voxel detected!")
        exit(0)
EOF
    exit $?
else
    echo "Screenshot not created!"
    exit 1
fi