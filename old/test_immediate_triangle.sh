#!/bin/bash
# Test immediate mode triangle

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

echo "=== Testing Immediate Mode Triangle ==="

# Run test
(
    echo "workspace 5 5 5"
    echo "screenshot triangle_test.ppm"
    sleep 0.5
    echo "quit"
) | "$VOXEL_CLI" 2>&1 | grep -v "Setting uniform" | grep -v "drawElements" | tail -10

# Check the screenshot
if [ -f triangle_test.ppm.ppm ]; then
    echo "Checking for red pixels..."
    python3 check_red_pixels.py triangle_test.ppm.ppm
fi