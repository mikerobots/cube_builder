#!/bin/bash
# Test with voxel at center of workspace

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

echo "=== Testing Centered Voxel ==="

# Run test and capture output, then save a screenshot
(
    echo "workspace 5 5 5"
    echo "resolution 8cm"
    echo "place 31 31 31"  # Center of 64x64x64 grid at 8cm resolution
    echo "screenshot centered_voxel.ppm"
    sleep 1
    echo "quit"
) | "$VOXEL_CLI"

# Check if screenshot was created
if [ -f centered_voxel.ppm ]; then
    echo "Screenshot saved to centered_voxel.ppm"
    ls -la centered_voxel.ppm
else
    echo "No screenshot created"
fi