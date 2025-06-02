#!/bin/bash

# Quick demo script for the Voxel Editor CLI
# This creates a simple structure and saves it

echo "=== Voxel Editor CLI Quick Demo ==="
echo ""
echo "Creating a simple house-like structure..."
echo ""

./voxel-cli << EOF
new house_demo

# Set workspace
workspace 20 20 20

# Create foundation (5x5x1)
fill 0 0 0 4 4 0

# Create walls
# Front wall with door gap
fill 0 0 1 0 4 3
fill 4 0 1 4 4 3
fill 0 4 1 4 4 3
# Back wall
fill 0 0 1 4 0 3

# Create roof (pyramid-like)
fill 1 1 4 3 3 4
place 2 2 5

# Group the structure
selectall
group House

# Show status
status
groups

# Save the house
save house_demo.vxl
echo "House saved to house_demo.vxl"

# Export to STL
export house_demo.stl
echo "House exported to house_demo.stl"

exit
EOF

echo ""
echo "Demo complete! Files created:"
echo "  - house_demo.vxl (voxel file)"
echo "  - house_demo.stl (3D model)"
echo ""
echo "To reload the house, run: ./voxel-cli"
echo "Then type: open house_demo.vxl"