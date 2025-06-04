#!/bin/bash
# Test with yellow shader and capture screenshot

echo "Testing with yellow shader..."

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
    echo "add 2 1 1"
    echo "add 1 2 1"
    echo "add 1 1 2"
    sleep 2
    echo "screenshot yellow_test.ppm"
    sleep 1
    echo "quit"
} > /tmp/voxel_cli_input

# Wait for the CLI to finish
wait $CLI_PID

# Clean up
rm -f /tmp/voxel_cli_input

# Check if screenshot was created
if [ -f yellow_test.ppm ]; then
    echo "Screenshot saved to yellow_test.ppm"
    python3 find_yellow_pixels.py
else
    echo "Failed to create screenshot"
fi

echo "Test complete!"