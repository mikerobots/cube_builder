#!/bin/bash
# Visual test to demonstrate the difference between grid and edges

echo "Testing grid vs edges visual difference..."
echo ""

mkdir -p test_output

# Test 1: Both on (default)
cat > test_output/both_on.txt << 'EOF'
workspace 5 5 5
resolution 32cm
place 0cm 0cm 0cm
place 32cm 0cm 0cm
place 0cm 0cm 32cm
place 32cm 0cm 32cm
camera iso
screenshot test_output/both_on.ppm
quit
EOF

# Test 2: Edges off, grid on
cat > test_output/edges_off_grid_on.txt << 'EOF'
workspace 5 5 5
resolution 32cm
edges off
place 0cm 0cm 0cm
place 32cm 0cm 0cm
place 0cm 0cm 32cm
place 32cm 0cm 32cm
camera iso
screenshot test_output/edges_off_grid_on.ppm
quit
EOF

# Test 3: Edges on, grid off
cat > test_output/edges_on_grid_off.txt << 'EOF'
workspace 5 5 5
resolution 32cm
grid off
place 0cm 0cm 0cm
place 32cm 0cm 0cm
place 0cm 0cm 32cm
place 32cm 0cm 32cm
camera iso
screenshot test_output/edges_on_grid_off.ppm
quit
EOF

# Test 4: Both off
cat > test_output/both_off.txt << 'EOF'
workspace 5 5 5
resolution 32cm
edges off
grid off
place 0cm 0cm 0cm
place 32cm 0cm 0cm
place 0cm 0cm 32cm
place 32cm 0cm 32cm
camera iso
screenshot test_output/both_off.ppm
quit
EOF

echo "Creating screenshots..."
echo "1. Both edges and grid ON (default)..."
./voxel-cli < test_output/both_on.txt > /dev/null 2>&1

echo "2. Edges OFF, grid ON..."
./voxel-cli < test_output/edges_off_grid_on.txt > /dev/null 2>&1

echo "3. Edges ON, grid OFF..."
./voxel-cli < test_output/edges_on_grid_off.txt > /dev/null 2>&1

echo "4. Both OFF..."
./voxel-cli < test_output/both_off.txt > /dev/null 2>&1

echo ""
echo "Screenshots created in test_output/:"
ls -la test_output/*.ppm* 2>/dev/null | grep -v ".txt"

echo ""
echo "Visual comparison summary:"
echo "- both_on.ppm: Shows voxel edges AND ground plane grid"
echo "- edges_off_grid_on.ppm: Shows only ground plane grid"
echo "- edges_on_grid_off.ppm: Shows only voxel edges"
echo "- both_off.ppm: Shows only solid voxels"

# Clean up command files
rm -f test_output/*.txt