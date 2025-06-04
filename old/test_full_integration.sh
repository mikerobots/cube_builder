#!/bin/bash

# Test full CLI integration
echo "Testing full CLI integration..."

# Launch CLI with comprehensive test commands
{
    echo "workspace 5 5 5"
    sleep 0.5
    echo "resolution 8cm"
    sleep 0.5
    echo "place 2 2 2"
    sleep 0.5
    echo "place 3 2 2"
    sleep 0.5 
    echo "select 2 2 2"
    sleep 0.5
    echo "status"
    sleep 0.5
    echo "save test_project.vxl"
    sleep 0.5
    echo "undo"
    sleep 0.5
    echo "redo"
    sleep 0.5
    echo "status"
    sleep 0.5
    echo "quit"
} | ./build_ninja/bin/VoxelEditor_CLI

echo "Full integration test completed"