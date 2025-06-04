#!/bin/bash
# Test placing voxels at the center of the workspace

echo "Testing voxels at workspace center..."

# Use a FIFO to control timing
FIFO=/tmp/voxel_center_fifo
rm -f $FIFO
mkfifo $FIFO

# Run voxel-cli in background, reading from FIFO
./voxel-cli < $FIFO 2>&1 | tee center_voxels_output.log &
CLI_PID=$!

# Send commands through FIFO
exec 3>$FIFO  # Open file descriptor 3 for writing to FIFO

echo "workspace size 5 5 5" >&3
sleep 0.5
echo "resolution 8cm" >&3
sleep 0.5

# Place voxels near the center of the 5x5x5 workspace
# With 8cm voxels, we need grid positions around 31 (2.5m / 0.08m = 31.25)
echo "place 30 30 30" >&3
sleep 0.5
echo "place 31 30 30" >&3
sleep 0.5
echo "place 32 30 30" >&3
sleep 0.5
echo "place 30 31 30" >&3
sleep 0.5
echo "place 31 31 30" >&3
sleep 0.5
echo "place 32 31 30" >&3
sleep 0.5
echo "place 30 32 30" >&3
sleep 0.5
echo "place 31 32 30" >&3
sleep 0.5
echo "place 32 32 30" >&3
sleep 0.5

# Debug commands
echo "debug camera" >&3
sleep 0.5
echo "debug voxels" >&3
sleep 0.5
echo "debug frustum" >&3
sleep 0.5

# Take screenshot
echo "screenshot center_voxels.ppm" >&3
sleep 1

# Keep window open for observation
echo "Keeping window open for 3 seconds..."
sleep 3

# Clean exit
echo "exit" >&3
exec 3>&-  # Close file descriptor

# Wait for CLI to finish
wait $CLI_PID

# Clean up
rm -f $FIFO

echo "Test complete. Check center_voxels.ppm for output."