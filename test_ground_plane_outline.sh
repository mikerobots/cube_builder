#!/bin/bash

# Test script to verify ground plane outline follows mouse cursor

echo "Testing ground plane outline positioning..."
echo "1. Launch the CLI"
echo "2. Move your mouse over empty space (ground plane)"
echo "3. The green outline should follow your mouse cursor exactly"
echo "4. When hovering over existing voxels, the outline should snap to grid positions"
echo ""
echo "Starting CLI..."

cd "$(dirname "$0")"
./build_ninja/bin/VoxelEditor_CLI