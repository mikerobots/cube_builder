#!/bin/bash
# Test script to see shader errors when placing a cube

echo "Testing cube placement and shader errors..."
echo ""

# Create test commands
cat > test_cube_commands.txt << 'EOF'
workspace 5 5 5
resolution 32cm

# Turn off edges to see shader issues more clearly
edges off

# Place a simple cube
place 0cm 0cm 0cm
place 32cm 0cm 0cm
place 0cm 32cm 0cm
place 32cm 32cm 0cm
place 0cm 0cm 32cm
place 32cm 0cm 32cm
place 0cm 32cm 32cm
place 32cm 32cm 32cm

# View from isometric angle
camera iso

# Debug render info
debug render

quit
EOF

echo "Running cube placement test..."
./voxel-cli < test_cube_commands.txt 2>&1 | grep -E "(shader|Shader|ERROR|Warning|warning|uniform|attribute|linking)"

echo ""
echo "Test complete!"

# Clean up
rm -f test_cube_commands.txt