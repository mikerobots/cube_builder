#!/bin/bash

# Test headless mode

echo "Testing headless mode..."

# Create a FIFO for input
FIFO=/tmp/voxel_cli_headless
rm -f $FIFO
mkfifo $FIFO

# Start the CLI in headless mode, reading from FIFO
./voxel-cli --headless < $FIFO &
CLI_PID=$!

# Function to send command
send_cmd() {
    echo "$1" > $FIFO
    sleep 0.2
}

# Wait for initialization
sleep 1

# Send commands
send_cmd "help"
send_cmd "resolution 128cm"
send_cmd "place 0 0 0"
send_cmd "place 1 0 0"
send_cmd "place 0 1 0"
send_cmd "status"
send_cmd "exit"

# Wait for CLI to exit
wait $CLI_PID

# Cleanup
rm -f $FIFO

echo "Headless test complete"