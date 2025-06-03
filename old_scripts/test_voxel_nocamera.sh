#!/bin/bash

echo "Testing voxel rendering without camera movement..."

rm -f test_nocamera.ppm

./build/bin/VoxelEditor_CLI << 'EOF'
resolution 128cm
place 0 0 0
place 1 0 0
place 0 1 0
place 0 0 1
screenshot test_nocamera.png
quit
EOF

echo ""
if [ -f test_nocamera.ppm ]; then
    echo "Screenshot created successfully"
    # Check if the image has non-blue pixels
    xxd -l 300 -s 17 test_nocamera.ppm | grep -v "334d 80" > /dev/null
    if [ $? -eq 0 ]; then
        echo "Image appears to contain rendered content!"
    else
        echo "Image appears to be all blue background"
    fi
else
    echo "Screenshot failed"
fi