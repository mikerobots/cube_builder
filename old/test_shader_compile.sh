#!/bin/bash
# Test shader compilation

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

echo "=== Testing Shader Compilation ==="

# Run and immediately quit
echo "quit" | "$VOXEL_CLI" 2>&1 | head -50