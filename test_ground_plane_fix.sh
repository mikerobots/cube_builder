#!/bin/bash

echo "Ground Plane Outline Fix Test"
echo "============================="
echo ""
echo "Instructions:"
echo "1. Switch to top view (press 't')"
echo "2. Move mouse over empty ground plane"
echo "3. The green outline should now follow your mouse cursor exactly"
echo "4. When hovering over existing voxels, outline still snaps to grid"
echo ""
echo "Debug info will be printed to console showing:"
echo "- World hit position (where mouse hits ground)"
echo "- Box min/max (outline corners)"
echo ""
echo "Starting CLI with debug logging enabled..."

cd "$(dirname "$0")"
./build_ninja/bin/VoxelEditor_CLI --log-level debug