#!/bin/bash

echo "Testing single voxel rendering..."

rm -f single_voxel.ppm

./build/bin/VoxelEditor_CLI << 'EOF'
resolution 128cm
place 0 0 0
camera iso
screenshot single_voxel.png
quit
EOF

echo ""
if [ -f single_voxel.ppm ]; then
    echo "Screenshot created successfully"
    open single_voxel.ppm
else
    echo "Screenshot failed"
fi