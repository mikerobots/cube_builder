#!/bin/bash

# Test STL export with a single cube

echo "Creating single cube and exporting to STL..."

# Clean up any existing test file
rm -f single_cube.stl

# Create the STL using the CLI
./execute_command.sh ./build_ninja/bin/VoxelEditor_CLI << EOF
new
resolution 32cm
place 0cm 0cm 0cm
export single_cube.stl
exit
EOF

# Check if STL was created
if [ -f single_cube.stl ]; then
    echo "STL file created successfully"
    echo "File size: $(ls -lh single_cube.stl | awk '{print $5}')"
    echo ""
    echo "First 20 lines of STL file:"
    head -20 single_cube.stl
    echo ""
    echo "Checking if binary or ASCII STL:"
    if head -c 5 single_cube.stl | grep -q "solid"; then
        echo "ASCII STL detected"
        echo "Triangle count: $(grep -c "facet normal" single_cube.stl)"
    else
        echo "Binary STL detected"
        # Binary STL has 80 byte header + 4 byte triangle count + 50 bytes per triangle
        filesize=$(stat -f%z single_cube.stl 2>/dev/null || stat -c%s single_cube.stl)
        if [ $filesize -gt 84 ]; then
            triangle_count=$(( ($filesize - 84) / 50 ))
            echo "Approximate triangle count: $triangle_count"
        fi
    fi
else
    echo "ERROR: STL file was not created"
fi