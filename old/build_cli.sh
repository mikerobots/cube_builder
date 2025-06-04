#!/bin/bash
# Build script for VoxelEditor CLI

# Get the number of CPU cores for parallel build
CORES=$(sysctl -n hw.ncpu)

echo "Building VoxelEditor CLI with $CORES parallel jobs..."

# Build the CLI target
cmake --build build_ninja --target VoxelEditor_CLI -j$CORES

# Check if build succeeded
if [ $? -eq 0 ]; then
    echo "Build completed successfully!"
else
    echo "Build failed!"
    exit 1
fi