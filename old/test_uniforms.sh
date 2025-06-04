#!/bin/bash
# Test uniforms being set

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

echo "=== Testing Uniforms ==="

# Run test and capture output
(
    echo "workspace 2 2 2"
    echo "resolution 32cm"
    echo "place 1 1 1"
    sleep 0.5
    echo "quit"
) | "$VOXEL_CLI" 2>&1 | grep -E "(Setting|camera|uniform|drawElements)" | head -50