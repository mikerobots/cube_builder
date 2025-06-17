#!/bin/bash
# Test boundary validation in Voxel Editor CLI

echo "Testing boundary validation for voxel placement..."

# Test 1: Test workspace bounds for 3x3x3 meter workspace
echo -e "\n=== Test 1: 3x3x3 meter workspace (bounds should be -150cm to +150cm in X/Z, 0 to 300cm in Y) ==="
./voxel-cli --headless << 'EOF'
workspace 3 3 3
resolution 16cm
place -160cm 0cm 0cm
place 160cm 0cm 0cm
place 0cm 0cm -160cm
place 0cm 0cm 160cm
place 0cm -10cm 0cm
place 0cm 310cm 0cm
stats
quit
EOF

# Test 2: Test grid alignment for 16cm voxels
echo -e "\n=== Test 2: Grid alignment for 16cm voxels (positions must be multiples of 16cm) ==="
./voxel-cli --headless << 'EOF'
workspace 3 3 3
resolution 16cm
place 0cm 0cm 0cm
place 16cm 0cm 0cm
place 32cm 0cm 0cm
place 15cm 0cm 0cm
place 17cm 0cm 0cm
place 152cm 0cm 0cm
stats
quit
EOF

# Test 3: Check the actual workspace bounds being used
echo -e "\n=== Test 3: Verify workspace bounds calculation ==="
./voxel-cli --headless << 'EOF'
workspace 3 3 3
resolution 1cm
place -150cm 0cm 0cm
place 150cm 0cm 0cm
place 0cm 0cm -150cm
place 0cm 0cm 150cm
place -151cm 0cm 0cm
place 151cm 0cm 0cm
stats
quit
EOF

echo -e "\nBoundary validation test complete."