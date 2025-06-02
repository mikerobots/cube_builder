#!/bin/bash
# Timed test script for voxel placement in CLI application

echo "Starting Voxel Editor CLI timed test..."

# Use expect if available, otherwise use a simpler approach
if command -v expect &> /dev/null; then
    echo "Using expect for precise timing control..."
    
    expect << 'EOF'
    spawn ./voxel-cli
    
    # Wait for prompt
    expect "voxel> "
    
    # Set up workspace
    send "workspace 5 5 5\r"
    expect "voxel> "
    
    send "resolution 8cm\r"
    expect "voxel> "
    
    send "view iso\r"
    expect "voxel> "
    
    # Place voxels in a pattern
    send "place 0 0 0\r"
    expect "voxel> "
    send "place 1 0 0\r"
    expect "voxel> "
    send "place 2 0 0\r"
    expect "voxel> "
    send "place 1 1 0\r"
    expect "voxel> "
    send "place 1 1 1\r"
    expect "voxel> "
    send "place 1 1 2\r"
    expect "voxel> "
    
    # Show status
    send "status\r"
    expect "voxel> "
    
    # Wait 5 seconds to see the result
    sleep 5
    
    # Save and quit
    send "save test_timed.vxl\r"
    expect "voxel> "
    
    send "quit\r"
    expect eof
EOF
    
else
    echo "Expect not found, using simple piped approach..."
    
    # Create a temporary FIFO for controlled input
    FIFO=$(mktemp -u)
    mkfifo "$FIFO"
    
    # Start the CLI reading from the FIFO
    ./voxel-cli < "$FIFO" &
    CLI_PID=$!
    
    # Open the FIFO for writing
    exec 3>"$FIFO"
    
    # Send commands with delays
    echo "workspace 5 5 5" >&3
    sleep 0.1
    echo "resolution 8cm" >&3
    sleep 0.1
    echo "view iso" >&3
    sleep 0.1
    
    # Place voxels
    echo "place 0 0 0" >&3
    sleep 0.1
    echo "place 1 0 0" >&3
    sleep 0.1
    echo "place 2 0 0" >&3
    sleep 0.1
    echo "place 1 1 0" >&3
    sleep 0.1
    echo "place 1 1 1" >&3
    sleep 0.1
    echo "place 1 1 2" >&3
    sleep 0.1
    
    echo "status" >&3
    
    # Wait 5 seconds
    echo "Waiting 5 seconds to observe the voxels..."
    sleep 5
    
    # Save and quit
    echo "save test_timed.vxl" >&3
    sleep 0.1
    echo "quit" >&3
    
    # Close the FIFO
    exec 3>&-
    
    # Wait for CLI to exit
    wait $CLI_PID
    
    # Clean up
    rm -f "$FIFO"
fi

echo "Test completed!"
echo "Check test_timed.vxl for the saved voxel data"