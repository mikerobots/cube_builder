#!/bin/bash
# Test OpenGL state debugging

echo "Testing OpenGL state debugging..."

# Create a named pipe for input
mkfifo /tmp/voxel_cli_input

# Start the CLI in the background, reading from the pipe
./build/bin/VoxelEditor_CLI < /tmp/voxel_cli_input &
CLI_PID=$!

# Give it time to start up
sleep 1

# Send commands to the pipe
{
    echo "workspace 3 3 3"
    echo "resolution 8cm"
    echo "add 1 1 1"
    sleep 3
    echo "quit"
} > /tmp/voxel_cli_input

# Wait for the CLI to finish
wait $CLI_PID

# Clean up
rm -f /tmp/voxel_cli_input

echo "Test complete!"