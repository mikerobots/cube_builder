#!/bin/bash
# Test script to render yellow voxels

echo "Setting up workspace and placing voxels..."

# Use a FIFO to control timing
FIFO=/tmp/voxel_test_fifo
rm -f $FIFO
mkfifo $FIFO

# Run voxel-cli in background, reading from FIFO
./voxel-cli < $FIFO 2>&1 | tee voxel_test_output.log &
CLI_PID=$!

# Send commands through FIFO
exec 3>$FIFO  # Open file descriptor 3 for writing to FIFO

echo "workspace size 5 5 5" >&3
sleep 0.5
echo "resolution 8cm" >&3
sleep 0.5

# Place some voxels in a visible pattern
echo "place 2 2 2" >&3
sleep 0.5
echo "place 3 2 2" >&3
sleep 0.5
echo "place 2 3 2" >&3
sleep 0.5
echo "place 2 2 3" >&3
sleep 0.5

# Save PPM to check what's being rendered
echo "screenshot test_yellow.ppm" >&3
sleep 1

# Keep window open for observation
echo "Keeping window open for 5 seconds..."
sleep 5

# Clean exit
echo "exit" >&3
exec 3>&-  # Close file descriptor

# Wait for CLI to finish
wait $CLI_PID

# Clean up
rm -f $FIFO

echo "Test complete. Check test_yellow.ppm for output."