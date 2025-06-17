#!/bin/bash
# Test script to see if shader issues are fixed when placing a cube

echo "Testing cube placement with fixed shader attributes..."
echo ""

# Create test commands
cat > test_cube_commands.txt << 'EOF'
workspace 5 5 5
resolution 32cm

# Turn off edges to see solid rendering
edges off

# Place a cube
place 0cm 0cm 0cm

# View from isometric angle
camera iso

# Take screenshot
screenshot test_output/cube_test.ppm

quit
EOF

echo "Running cube placement test..."
./voxel-cli < test_cube_commands.txt 2>&1 | tail -30

echo ""
echo "Checking if screenshot was created..."
ls -la test_output/cube_test* 2>/dev/null

# Clean up
rm -f test_cube_commands.txt