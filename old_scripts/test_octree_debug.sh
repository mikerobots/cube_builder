#!/bin/bash

echo "Testing octree position mapping..."

./build/bin/VoxelEditor_CLI << 'EOF' 2>&1 | grep -E "(VoxelGrid::setVoxel|SparseOctree:|Depth|Octree voxel)"
resolution 128cm
place 0 0 0
place 1 0 0
place 0 1 0
place 0 0 1
quit
EOF