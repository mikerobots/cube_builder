#!/bin/bash

echo "===================================="
echo "Voxel Editor - Headless Mode Demo"
echo "===================================="
echo ""
echo "This demonstrates running the voxel editor without rendering."
echo "Useful for batch operations, automated testing, and CI/CD."
echo ""

# Run some commands in headless mode
echo "Running batch voxel operations in headless mode..."
echo ""

./voxel-cli --headless << 'EOF'
resolution 128cm
place 0 0 0
place 1 0 0
place 2 0 0
place 0 1 0
place 1 1 0
place 2 1 0
status
save test_structure.vxl
export test_structure.stl
quit
EOF

echo ""
echo "Headless operations completed!"
echo ""
echo "To run with rendering window, use: ./voxel-cli"
echo "To run in headless mode, use: ./voxel-cli --headless"