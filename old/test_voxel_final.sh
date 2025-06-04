#!/bin/bash
cd /Users/michaelhalloran/cube_edit/build/bin

# Create a FIFO for communication
FIFO=/tmp/voxel_test_$$
mkfifo $FIFO

# Clean up on exit
trap "rm -f $FIFO" EXIT

# Start voxel-cli with input from FIFO
./VoxelEditor_CLI < $FIFO &
CLI_PID=$!

# Send commands
{
    sleep 1
    echo "workspace 5 5 5"
    sleep 0.5
    echo "resolution 8cm"
    sleep 0.5
    echo "place 2 2 2"
    sleep 0.5
    echo "screenshot test_final.ppm"
    sleep 3
    echo "quit"
} > $FIFO &

# Wait for CLI to finish
wait $CLI_PID

echo "Test complete! Check test_final.ppm.ppm"