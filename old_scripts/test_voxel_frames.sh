#!/bin/bash

# Test with multiple render frames before screenshot

echo "Testing voxel rendering with multiple frames..."

# Create a FIFO for input
FIFO=/tmp/voxel_cli_frames
rm -f $FIFO
mkfifo $FIFO

# Start the CLI in background
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
send_cmd "resolution 128cm"
send_cmd "place 0 0 0"

# Force multiple render frames
echo "Waiting for multiple render frames..."
sleep 3  # Let it render for 3 seconds (should be ~180 frames at 60fps)

send_cmd "screenshot test_after_frames.ppm" 
sleep 1
send_cmd "exit"

# Wait for CLI to exit
wait $CLI_PID

# Cleanup
rm -f $FIFO

# Analyze the screenshot
echo ""
echo "Analyzing screenshot..."
python3 << 'EOF'
try:
    with open('test_after_frames.ppm.ppm', 'rb') as f:
        # Read PPM header
        magic = f.readline().decode().strip()
        line = f.readline().decode().strip()
        while line.startswith('#'):
            line = f.readline().decode().strip()
        width, height = map(int, line.split())
        max_val = int(f.readline().decode().strip())
        
        # Read pixels
        red_count = 0
        total_pixels = width * height
        pixels = f.read(total_pixels * 3)
        
        for i in range(0, len(pixels), 3):
            r, g, b = pixels[i], pixels[i+1], pixels[i+2]
            if r > 200 and g < 50 and b < 50:
                red_count += 1
        
        print(f'Image size: {width}x{height}')
        print(f'Red pixels found: {red_count}/{total_pixels} ({red_count/total_pixels*100:.2f}%)')
        
        if red_count == 0:
            print('NO VOXEL VISIBLE!')
        else:
            print('VOXEL IS VISIBLE!')
            
except Exception as e:
    print(f'Error: {e}')
EOF