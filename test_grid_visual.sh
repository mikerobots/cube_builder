#!/bin/bash
# Visual test for ground plane grid command

echo "Testing ground plane grid visibility..."
echo ""

# Create output directory
mkdir -p test_output

# Test 1: With grid on (default)
cat > test_output/grid_on_commands.txt << 'EOF'
workspace 5 5 5
resolution 16cm
place 0cm 0cm 0cm
place 1m 0cm 0cm
place -1m 0cm 0cm
camera iso
screenshot test_output/grid_on.ppm
quit
EOF

echo "Test 1: Default grid (should be on)..."
./voxel-cli < test_output/grid_on_commands.txt > /dev/null 2>&1

# Test 2: With grid off
cat > test_output/grid_off_commands.txt << 'EOF'
workspace 5 5 5
resolution 16cm
grid off
place 0cm 0cm 0cm
place 1m 0cm 0cm
place -1m 0cm 0cm
camera iso
screenshot test_output/grid_off.ppm
quit
EOF

echo "Test 2: Grid turned off..."
./voxel-cli < test_output/grid_off_commands.txt > /dev/null 2>&1

# Find actual screenshot files
GRID_ON_FILE=""
GRID_OFF_FILE=""

for ext in ".ppm" ".ppm.ppm" ".ppm.ppm.ppm"; do
    if [ -f "test_output/grid_on${ext}" ]; then
        GRID_ON_FILE="test_output/grid_on${ext}"
    fi
    if [ -f "test_output/grid_off${ext}" ]; then
        GRID_OFF_FILE="test_output/grid_off${ext}"
    fi
done

if [ -n "$GRID_ON_FILE" ] && [ -n "$GRID_OFF_FILE" ]; then
    echo ""
    echo "Screenshots created:"
    echo "- Grid ON:  $GRID_ON_FILE"
    echo "- Grid OFF: $GRID_OFF_FILE"
    
    # Analyze color differences
    echo ""
    echo "Analyzing color differences..."
    echo "Grid ON colors:"
    ./tools/analyze_ppm_colors.py "$GRID_ON_FILE" | head -10
    
    echo ""
    echo "Grid OFF colors:"
    ./tools/analyze_ppm_colors.py "$GRID_OFF_FILE" | head -10
else
    echo "ERROR: Screenshots not created properly"
fi

echo ""
echo "Test complete!"
echo "You can visually compare the screenshots to see the grid difference."

# Clean up command files
rm -f test_output/grid_on_commands.txt test_output/grid_off_commands.txt