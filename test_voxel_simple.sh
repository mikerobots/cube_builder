#!/bin/bash
# Simple test script for voxel placement with 5-second display

echo "Starting Voxel Editor CLI simple test..."

# Function to run commands with timing
run_test() {
    # Initial setup commands
    echo "workspace 5 5 5"
    echo "resolution 8cm"
    echo "view iso"
    
    # Place voxels to form a simple L-shape
    echo "place 0 0 0"
    echo "place 1 0 0"
    echo "place 2 0 0"
    echo "place 0 1 0"
    echo "place 0 2 0"
    
    # Add some height
    echo "place 0 0 1"
    echo "place 0 0 2"
    
    # Show what we've created
    echo "status"
    
    # Save the work
    echo "save test_simple.vxl"
    
    # Sleep for 5 seconds to let user see the window
    sleep 5
    
    # Quit
    echo "quit"
}

# Run the test
echo "Launching CLI with automated commands..."
echo "The window will stay open for 5 seconds to display the voxels..."

run_test | ./voxel-cli

echo ""
echo "Test completed!"
echo "Created files:"
echo "  - test_simple.vxl (saved voxel data)"
echo ""
echo "The test created an L-shaped structure with 7 voxels."