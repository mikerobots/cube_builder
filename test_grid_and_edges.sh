#!/bin/bash
# Test script for both grid and edges commands

echo "Testing grid and edges commands..."
echo ""

# Create test commands
cat > test_commands.txt << 'EOF'
workspace 5 5 5
resolution 16cm

# Place some voxels to see edges
place 0cm 0cm 0cm
place 16cm 0cm 0cm
place 0cm 16cm 0cm
place 16cm 16cm 0cm

# Test edge commands
edges
edges off
edges on
edges toggle

# Test grid commands
grid
grid off
grid on
grid toggle

quit
EOF

echo "Running test commands..."
./voxel-cli < test_commands.txt 2>&1 | grep -E "(edges|grid|Ground plane|Voxel edges)" | grep -v "Generated ground"

echo ""
echo "Test complete!"

# Clean up
rm -f test_commands.txt