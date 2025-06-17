#!/bin/bash

# Test the ground plane grid rendering

echo "Testing ground plane grid rendering..."

# Build first
echo "Building project..."
cmake --build build_ninja

# Test 1: Show grid (default)
echo -e "\n=== Test 1: Grid visible (default) ==="
./build_ninja/bin/VoxelEditor_CLI << EOF
grid on
screenshot test_grid_visible.ppm
sleep 0.5
quit
EOF

# Test 2: Hide grid
echo -e "\n=== Test 2: Grid hidden ==="
./build_ninja/bin/VoxelEditor_CLI << EOF
grid off
screenshot test_grid_hidden.ppm
sleep 0.5
quit
EOF

# Test 3: Grid with voxels
echo -e "\n=== Test 3: Grid with voxels ==="
./build_ninja/bin/VoxelEditor_CLI << EOF
grid on
place 0cm 0cm 0cm
place 32cm 0cm 0cm
place 64cm 0cm 0cm
screenshot test_grid_with_voxels.ppm
sleep 0.5
quit
EOF

# Test 4: Toggle grid
echo -e "\n=== Test 4: Grid toggle ==="
./build_ninja/bin/VoxelEditor_CLI << EOF
grid off
screenshot test_grid_off.ppm
grid toggle
screenshot test_grid_toggle_on.ppm
sleep 0.5
quit
EOF

# Analyze the screenshots
echo -e "\n=== Analyzing screenshots ==="
if [ -f "test_grid_visible.ppm" ]; then
    echo "Grid visible analysis:"
    python3 tools/analyze_ppm_colors.py test_grid_visible.ppm | head -20
fi

if [ -f "test_grid_hidden.ppm" ]; then
    echo -e "\nGrid hidden analysis:"
    python3 tools/analyze_ppm_colors.py test_grid_hidden.ppm | head -20
fi

echo -e "\nTest complete!"