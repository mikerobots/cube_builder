#!/bin/bash

# Simple edge rendering test focusing on the error
echo "Testing edge rendering..."

# Test with edges disabled (baseline)
echo -e "\n1. Testing without edges (baseline):"
timeout 2 ./build_ninja/bin/VoxelEditor_CLI 2>&1 <<EOF | grep -E "(Error|error|GL_|place)"
place 0cm 0cm 0cm
quit
EOF

# Test with edges enabled  
echo -e "\n2. Testing with edges enabled:"
timeout 2 ./build_ninja/bin/VoxelEditor_CLI 2>&1 <<EOF | grep -E "(Error|error|GL_|edge|place)"
edges on
place 0cm 0cm 0cm
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

# Test edge coordinates with centered system
echo -e "\n4. Testing edge coordinates (centered system):"
timeout 2 ./build_ninja/bin/VoxelEditor_CLI --headless 2>&1 <<EOF | grep -E "(place|Error|error|bounds)"
# Test negative coordinates (should work with centered system)
place -10cm 0cm -10cm
place -20cm 0cm -20cm
# Test positive coordinates
place 10cm 0cm 10cm  
place 20cm 0cm 20cm
# Test Y boundary (Y must be >= 0)
place 0cm 0cm 0cm
# Test mixed coordinates
place -5cm 0cm 5cm
place 5cm 0cm -5cm
quit
EOF

echo -e "\nDone!"