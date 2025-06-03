#!/bin/bash

echo "Testing with debug output..."

./build/bin/VoxelEditor_CLI << 'EOF' 2>&1 | tee debug_output.log
resolution 32cm
place 0 0 0
status
screenshot test_debug.png
quit
EOF

echo ""
echo "Check debug_output.log for shader and camera info"