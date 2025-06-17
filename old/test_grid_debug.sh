#!/bin/bash
# Debug test for ground plane grid visibility

echo "Testing ground plane grid visibility state..."
echo ""

# Create test commands that query grid state
cat > test_grid_debug.txt << 'EOF'
# Check initial state
debug render

# Turn grid off
grid off
debug render

# Turn grid on
grid on
debug render

# Toggle grid
grid toggle
debug render

quit
EOF

echo "Running debug commands..."
./voxel-cli < test_grid_debug.txt 2>&1 | grep -E "(Ground plane|Grid:|grid visible|render info)"

echo ""
echo "Test complete!"

# Clean up
rm -f test_grid_debug.txt