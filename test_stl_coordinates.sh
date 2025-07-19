#!/bin/bash

# Create test STL with a 32cm voxel at origin
echo "Creating test STL with 32cm voxel at origin..."

# Create temporary test script
cat << 'EOF' > test_commands.txt
# Set to 32cm resolution
resolution 32cm

# Place voxel at origin
place 0cm 0cm 0cm

# Export to STL
export test_voxel.stl

# Exit
exit
EOF

# Run the CLI with test commands
./execute_command.sh ./build_debug/bin/VoxelEditor_CLI < test_commands.txt

# Check if STL was created
if [ -f "test_voxel.stl" ]; then
    echo "STL file created successfully"
    echo "Checking STL contents..."
    
    # Display first few lines of STL (should show vertex coordinates)
    echo "First 20 lines of STL file:"
    head -n 20 test_voxel.stl
    
    echo ""
    echo "Checking for vertex coordinates in expected range..."
    # Use grep to find vertex lines and check coordinates
    grep "vertex" test_voxel.stl | head -n 10
else
    echo "ERROR: STL file was not created"
fi

# Cleanup
rm -f test_commands.txt test_voxel.stl