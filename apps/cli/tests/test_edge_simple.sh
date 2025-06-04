#!/bin/bash

# Simple edge rendering test focusing on the error
echo "Testing edge rendering..."

# Test with edges disabled (baseline)
echo -e "\n1. Testing without edges (baseline):"
timeout 2 ./build_ninja/bin/VoxelEditor_CLI 2>&1 <<EOF | grep -E "(Error|error|GL_|place)"
place 0 0 0
quit
EOF

# Test with edges enabled  
echo -e "\n2. Testing with edges enabled:"
timeout 2 ./build_ninja/bin/VoxelEditor_CLI 2>&1 <<EOF | grep -E "(Error|error|GL_|edge|place)"
edges on
place 0 0 0
quit
EOF

# Test shader switching
echo -e "\n3. Testing shader switching:"
timeout 2 ./build_ninja/bin/VoxelEditor_CLI --headless 2>&1 <<EOF | grep -E "(shader|Shader)"
shader list
shader enhanced
shader basic
shader flat
quit
EOF

echo -e "\nDone!"