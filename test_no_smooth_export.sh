#!/bin/bash

echo "Testing STL export with no smoothing..."

# Clean up
rm -f test_no_smooth.stl

# Create STL with smoothing level 0
./execute_command.sh ./build_ninja/bin/VoxelEditor_CLI << EOF
new
resolution 32cm
place 0cm 0cm 0cm
smooth 0
export test_no_smooth.stl
exit
EOF

# Check the file
if [ -f test_no_smooth.stl ]; then
    echo "STL created. Parsing..."
    python3 parse_binary_stl.py test_no_smooth.stl | head -50
else
    echo "ERROR: STL file was not created"
fi