#!/bin/bash

# Test voxel placement in CLI
echo "Testing voxel placement..."

# Launch CLI with some commands
{
    echo "help"
    sleep 1
    echo "workspace 5 5 5"  
    sleep 1
    echo "resolution 8cm"
    sleep 1
    echo "quit"
} | ./build_ninja/bin/VoxelEditor_CLI

echo "CLI test completed"