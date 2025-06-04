#!/bin/bash

# Test zoom behavior in CLI
echo "Testing zoom behavior in CLI app"

# Create a temporary command file
cat > /tmp/zoom_test_commands.txt << EOF
# Add a voxel first
voxel 100 100 100

# Test zoom commands
zoom 1.5
zoom 1.5
zoom 1.5
zoom 0.8
zoom 0.8

# Exit
quit
EOF

# Run the CLI with the commands
echo "Running CLI with zoom test commands..."
./build_ninja/bin/VoxelEditor_CLI < /tmp/zoom_test_commands.txt

echo "Test complete"