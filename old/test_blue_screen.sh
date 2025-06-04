#!/bin/bash
# Test if we can see blue background

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

echo "=== Testing Blue Background ==="

# Run test without placing voxels
(
    echo "workspace 5 5 5"
    echo "screenshot blue_test.ppm"
    sleep 0.5
    echo "quit"
) | "$VOXEL_CLI" 2>&1 | grep -v "Setting uniform" | grep -v "drawElements"

# Check the screenshot
if [ -f blue_test.ppm ]; then
    echo "Screenshot created!"
    # Use ImageMagick to check the image
    if command -v identify &> /dev/null; then
        identify blue_test.ppm
    fi
    
    # Check file size
    ls -la blue_test.ppm
fi