#!/bin/bash

# Test script with camera debugging

echo "Testing voxel editor with camera debugging..."

# Create a FIFO for input
FIFO=/tmp/voxel_cli_input
rm -f $FIFO
mkfifo $FIFO

# Start the CLI in background, reading from FIFO
./voxel-cli < $FIFO &
CLI_PID=$!

# Function to send command
send_cmd() {
    echo "$1" > $FIFO
    sleep 0.5
}

# Wait for initialization
sleep 2

# Send commands
send_cmd "resolution 128cm"  # 128cm voxels
send_cmd "place 0 0 0"
sleep 2  # Wait for mesh generation
send_cmd "render"
sleep 2  # Wait for rendering
send_cmd "screenshot test_camera_debug.ppm" 
sleep 1
send_cmd "exit"  # or "q"

# Wait for CLI to exit
wait $CLI_PID

# Cleanup
rm -f $FIFO

echo "Test complete, check test_camera_debug.ppm"