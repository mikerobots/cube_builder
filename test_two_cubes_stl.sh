#!/bin/bash

# Test STL export with two cubes side by side

echo "Creating two cubes side by side and exporting to STL..."

# Clean up any existing test file
rm -f two_cubes.stl

# Create the STL using the CLI
./execute_command.sh ./build_ninja/bin/VoxelEditor_CLI << EOF
new
resolution 32cm
place 0cm 0cm 0cm
place 32cm 0cm 0cm
export two_cubes.stl
exit
EOF

# Check if STL was created
if [ -f two_cubes.stl ]; then
    echo "STL file created successfully"
    echo "File size: $(ls -lh two_cubes.stl | awk '{print $5}')"
    echo ""
    echo "Checking if binary or ASCII STL:"
    if head -c 5 two_cubes.stl | grep -q "solid"; then
        echo "ASCII STL detected"
        echo "Triangle count: $(grep -c "facet normal" two_cubes.stl)"
    else
        echo "Binary STL detected"
        # Binary STL has 80 byte header + 4 byte triangle count + 50 bytes per triangle
        filesize=$(stat -f%z two_cubes.stl 2>/dev/null || stat -c%s two_cubes.stl)
        if [ $filesize -gt 84 ]; then
            triangle_count=$(( ($filesize - 84) / 50 ))
            echo "Approximate triangle count: $triangle_count"
        fi
    fi
else
    echo "ERROR: STL file was not created"
fi