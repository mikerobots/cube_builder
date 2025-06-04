#!/bin/bash
# Test for OpenGL errors

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

echo "=== Testing for GL Errors ==="

# Run test
(
    echo "workspace 5 5 5"
    echo "resolution 8cm"
    echo "place 31 31 31"
    echo "screenshot gl_errors.ppm"
    sleep 0.5
    echo "quit"
) | "$VOXEL_CLI" 2>&1 | grep -E "(GL Error|drawElements)" | head -20