#!/bin/bash

# Test script for CLI mesh resolution command
# This tests the new mesh resolution functionality

set -e

echo "Testing CLI mesh resolution command..."
echo "======================================"

# Build the CLI if needed
if [ ! -f ./build_ninja/bin/VoxelEditor_CLI ]; then
    echo "Building CLI..."
    cmake -B build_ninja -G Ninja -DCMAKE_BUILD_TYPE=Debug
    cmake --build build_ninja --target VoxelEditor_CLI
fi

CLI="./execute_command.sh ./build_ninja/bin/VoxelEditor_CLI"

# Test 1: Show current mesh resolution
echo -e "\n1. Show current mesh resolution:"
echo "mesh resolution" | $CLI | grep -A2 "Current mesh resolution"

# Test 2: Set mesh resolution to 16cm
echo -e "\n2. Set mesh resolution to 16cm:"
echo -e "mesh resolution 16cm\nexit" | $CLI | grep "Mesh resolution set to"

# Test 3: Set mesh resolution to 8cm
echo -e "\n3. Set mesh resolution to 8cm:"
echo -e "mesh resolution 8cm\nexit" | $CLI | grep "Mesh resolution set to"

# Test 4: Set mesh resolution to 4cm
echo -e "\n4. Set mesh resolution to 4cm:"
echo -e "mesh resolution 4cm\nexit" | $CLI | grep "Mesh resolution set to"

# Test 5: Invalid resolution
echo -e "\n5. Test invalid resolution:"
echo -e "mesh resolution 3cm\nexit" | $CLI | grep "Invalid resolution"

# Test 6: Test with actual voxel placement and export
echo -e "\n6. Test mesh generation with different resolutions:"

# Test with 16cm resolution (fast)
echo -e "new\nresolution 32cm\nplace 0cm 0cm 0cm\nplace 32cm 0cm 0cm\nsmoothing 0\nmesh resolution 16cm\nexport test_16cm.stl\nexit" | $CLI > /dev/null 2>&1
if [ -f "test_16cm.stl" ]; then
    echo "✓ Successfully exported with 16cm resolution"
    rm -f test_16cm.stl
else
    echo "✗ Failed to export with 16cm resolution"
fi

# Test with 4cm resolution (high quality)
echo -e "new\nresolution 32cm\nplace 0cm 0cm 0cm\nplace 32cm 0cm 0cm\nsmoothing 0\nmesh resolution 4cm\nexport test_4cm.stl\nexit" | $CLI > /dev/null 2>&1
if [ -f "test_4cm.stl" ]; then
    echo "✓ Successfully exported with 4cm resolution"
    rm -f test_4cm.stl
else
    echo "✗ Failed to export with 4cm resolution"
fi

echo -e "\n======================================"
echo "Mesh resolution command tests complete!"