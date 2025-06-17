#!/bin/bash
# Test script for ground plane grid command

echo "Testing ground plane grid command..."
echo ""

# Create test commands
cat > test_grid_commands.txt << 'EOF'
workspace 5 5 5
resolution 16cm

# Test grid toggle (should work with default state)
grid

# Test explicit states
grid off
grid on
grid toggle

# Place some voxels to see the grid effect
place 0cm 0cm 0cm
place 50cm 16cm 50cm
place -50cm 32cm -50cm

# Turn grid off and on while voxels are visible
grid off
grid on

# Invalid state test
grid invalid

quit
EOF

echo "Running test commands..."
./bin/VoxelEditor_CLI < test_grid_commands.txt 2>&1 | grep -E "(Ground plane grid|Invalid state|Error)"

echo ""
echo "Test complete!"

# Clean up
rm -f test_grid_commands.txt