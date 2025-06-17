#!/bin/bash
# Test script for new coordinate unit system

echo "Testing Voxel Editor coordinate units..."
echo ""

# Create test commands showing different unit formats
cat > test_units_commands.txt << 'EOF'
workspace 5 5 5
resolution 16cm

# Place voxels using centimeter units
place 0cm 0cm 0cm
place 50cm 16cm 50cm
place -50cm 32cm -50cm

# Place voxels using meter units
place 1m 48cm 0m
place -1m 64cm 0m
place 0.5m 80cm 0.5m

# Mixed units
place -0.75m 96cm 75cm
place 1.25m 1.12m -1.25m

stats
quit
EOF

echo "Running test commands..."
./bin/VoxelEditor_CLI < test_units_commands.txt 2>&1 | grep -E "(placed|Total voxels:|Error|Invalid)"

echo ""
echo "Test complete!"

# Clean up
rm -f test_units_commands.txt