#!/bin/bash
# Test script for voxel placement in CLI application

echo "Starting Voxel Editor CLI test..."

# Create a command file with test commands
cat << 'EOF' > test_commands.txt
# Set up workspace
workspace 5 5 5
resolution 8cm
view iso

# Place some voxels to create a simple structure
place 0 0 0
place 1 0 0
place 2 0 0
place 0 1 0
place 1 1 0
place 2 1 0
place 0 2 0
place 1 2 0
place 2 2 0

# Create a second layer
place 0 0 1
place 1 0 1
place 2 0 1
place 0 1 1
place 2 1 1
place 0 2 1
place 1 2 1
place 2 2 1

# Create a top
place 1 1 2

# Save the creation
save test_structure.vxl

# Wait a moment then quit
status
EOF

# Run the CLI with commands piped in, with a timeout
echo "Running voxel placement commands..."
(
    # Send commands from file
    cat test_commands.txt
    # Wait 5 seconds
    sleep 5
    # Send quit command
    echo "quit"
) | ./voxel-cli

echo "Test completed!"
echo "Check test_structure.vxl for the saved voxel data"

# Clean up
rm -f test_commands.txt