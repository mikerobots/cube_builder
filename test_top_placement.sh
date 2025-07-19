#!/bin/bash

# Test script to verify top face placement alignment

echo "Testing top face placement alignment..."
echo ""

# Create a test scene with the CLI
./execute_command.sh ./build_ninja/bin/VoxelEditor_CLI << EOF
place 0cm 0cm 0cm
resolution 2cm
place 2cm 32cm 0cm
screenshot test_top_alignment.ppm
place 0cm 32cm 0cm
screenshot test_top_alignment2.ppm
place -2cm 32cm 0cm
screenshot test_top_alignment3.ppm
exit
EOF

echo ""
echo "Screenshots saved:"
echo "- test_top_alignment.ppm: 2cm voxel at (2cm, 32cm, 0cm) - should be offset"
echo "- test_top_alignment2.ppm: 2cm voxel at (0cm, 32cm, 0cm) - should be aligned"
echo "- test_top_alignment3.ppm: 2cm voxel at (-2cm, 32cm, 0cm) - should be offset"