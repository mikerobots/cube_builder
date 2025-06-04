#!/bin/bash
# Test debug commands

echo "Testing debug commands..."

# Use a FIFO to control timing
FIFO=/tmp/voxel_debug_fifo
rm -f $FIFO
mkfifo $FIFO

# Run voxel-cli in background, reading from FIFO
./voxel-cli < $FIFO 2>&1 | tee debug_commands_output.log &
CLI_PID=$!

# Send commands through FIFO
exec 3>$FIFO  # Open file descriptor 3 for writing to FIFO

echo "=== Testing debug camera ===" >&3
echo "debug camera" >&3
sleep 1

echo -e "\n=== Setting up workspace ===" >&3
echo "workspace size 5 5 5" >&3
sleep 0.5
echo "resolution 8cm" >&3
sleep 0.5

echo -e "\n=== Placing voxels ===" >&3
echo "place 2 2 2" >&3
sleep 0.5
echo "place 3 2 2" >&3
sleep 0.5

echo -e "\n=== Testing debug voxels ===" >&3
echo "debug voxels" >&3
sleep 1

echo -e "\n=== Testing debug render ===" >&3
echo "debug render" >&3
sleep 1

echo -e "\n=== Testing debug frustum ===" >&3
echo "debug frustum" >&3
sleep 1

echo -e "\n=== Taking screenshot ===" >&3
echo "screenshot debug_test.ppm" >&3
sleep 1

# Clean exit
echo "exit" >&3
exec 3>&-  # Close file descriptor

# Wait for CLI to finish
wait $CLI_PID

# Clean up
rm -f $FIFO

echo -e "\nTest complete. Check debug_commands_output.log for results."

# Extract key debug info
echo -e "\n=== Key Debug Info ==="
grep -A 5 "Camera Debug Info" debug_commands_output.log | head -10
echo "..."
grep -A 5 "Voxel Debug Info" debug_commands_output.log | head -10
echo "..."
grep -A 5 "First voxel" debug_commands_output.log | head -10