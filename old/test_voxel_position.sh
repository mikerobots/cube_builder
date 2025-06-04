#!/bin/bash
# Test voxel world positions

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

echo "=== Testing Voxel Positions ==="

# Run test and capture output
(
    echo "workspace 5 5 5"
    echo "resolution 32cm"
    echo "place 2 2 2"
    sleep 0.5
    echo "quit"
) | "$VOXEL_CLI" 2>&1 | grep -E "(workspace|Voxel|world pos|Camera)" | head -20