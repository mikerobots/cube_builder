#!/bin/bash

# Test voxel rendering with debug output

echo "Testing voxel rendering..."

# Create a test script to send commands
cat > test_commands.txt << 'EOF'
workspace 5 5 5
resolution 8cm
place 2 2 2
debug camera
debug voxels
debug render
screenshot test_debug.ppm
quit
EOF

# Run the CLI with commands
echo "Running voxel-cli with test commands..."
cd build/bin
./VoxelEditor_CLI < ../../test_commands.txt > ../../voxel_test_debug.log 2>&1

# Check if screenshot was created
if [ -f "test_debug.ppm" ]; then
    echo "Screenshot created successfully"
    
    # Count non-black pixels
    python3 << 'PYTHON'
import sys

try:
    with open('test_debug.ppm', 'rb') as f:
        # Skip PPM header
        header = f.readline()
        dims = f.readline()
        maxval = f.readline()
        
        # Read pixel data
        data = f.read()
        
    # Count non-black pixels (assuming RGB format)
    non_black = 0
    total_pixels = len(data) // 3
    
    for i in range(0, len(data), 3):
        r, g, b = data[i], data[i+1], data[i+2]
        if r > 0 or g > 0 or b > 0:
            non_black += 1
    
    print(f"Total pixels: {total_pixels}")
    print(f"Non-black pixels: {non_black}")
    print(f"Percentage: {non_black/total_pixels*100:.2f}%")
    
    # Check if any yellow pixels (high R and G, low B)
    yellow_pixels = 0
    for i in range(0, len(data), 3):
        r, g, b = data[i], data[i+1], data[i+2]
        if r > 200 and g > 200 and b < 50:
            yellow_pixels += 1
    
    print(f"Yellow pixels: {yellow_pixels}")
    
except Exception as e:
    print(f"Error analyzing image: {e}")
    sys.exit(1)
PYTHON
    
    # Move screenshot to parent directory for inspection
    mv test_debug.ppm ../../
else
    echo "No screenshot created - check log for errors"
fi

# Clean up
cd ../..
rm -f test_commands.txt

echo "Check voxel_test_debug.log for output"