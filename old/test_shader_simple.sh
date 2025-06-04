#!/bin/bash
# Simple shader test

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

echo "=== Testing Simple Shader ==="

# Run test and look at uniform locations
(
    echo "workspace 5 5 5"
    echo "resolution 8cm"
    echo "place 31 31 31"
    sleep 0.5
    echo "quit"
) | "$VOXEL_CLI" 2>&1 | grep -E "(uniform|location|Shader)" | head -30