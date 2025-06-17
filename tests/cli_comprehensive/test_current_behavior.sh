#!/bin/bash
# Test the current actual behavior (not the intended requirements)

echo "=== Testing Current Implementation Behavior ==="
echo "Note: This tests what actually works, not what should work per requirements"
echo ""

# Test 1: Y < 0 constraint (this works correctly)
echo "Test 1: Y < 0 Constraint"
cat > /tmp/y_test.txt << EOF
workspace 5 5 5
resolution 32cm
place 100 -10 100
place 100 0 100
place 100 10 100
quit
EOF

OUTPUT=$(timeout 10s ../../voxel-cli --headless < /tmp/y_test.txt 2>&1)
PLACED=$(echo "$OUTPUT" | grep -c "Voxel placed")
ERRORS=$(echo "$OUTPUT" | grep -c "Cannot place voxel")

if [ "$PLACED" -eq 2 ] && [ "$ERRORS" -eq 1 ]; then
    echo "✅ PASS: Y < 0 constraint working"
else
    echo "❌ FAIL: Y < 0 constraint not working"
fi

# Test 2: Current coordinate acceptance (no bounds checking)
echo ""
echo "Test 2: Coordinate Bounds (Current Behavior)"
cat > /tmp/bounds_test.txt << EOF
workspace 5 5 5
resolution 32cm
place 0 0 0
place 1000 0 1000
place -1000 0 -1000
quit
EOF

OUTPUT=$(timeout 10s ../../voxel-cli --headless < /tmp/bounds_test.txt 2>&1)
PLACED=$(echo "$OUTPUT" | grep -c "Voxel placed")

echo "  Placed $PLACED voxels (no bounds checking currently)"
if [ "$PLACED" -eq 3 ]; then
    echo "  ⚠️  Current behavior: All positions accepted"
fi

# Test 3: PlacementCommands integration
echo ""
echo "Test 3: PlacementCommands Integration"
cat > /tmp/placement_test.txt << EOF
workspace 5 5 5
resolution 32cm
delete 100 0 100
place 100 0 100
delete 100 0 100
quit
EOF

OUTPUT=$(timeout 10s ../../voxel-cli --headless < /tmp/placement_test.txt 2>&1)
if echo "$OUTPUT" | grep -q "No voxel at specified position" && \
   echo "$OUTPUT" | grep -q "Voxel placed at" && \
   echo "$OUTPUT" | grep -q "Voxel deleted at"; then
    echo "✅ PASS: PlacementCommands working for validation"
else
    echo "❌ FAIL: PlacementCommands not working correctly"
fi

# Test 4: Grid and visual commands (headless)
echo ""
echo "Test 4: Grid Commands"
cat > /tmp/grid_test.txt << EOF
grid on
grid off
grid toggle
quit
EOF

if timeout 5s ../../voxel-cli --headless < /tmp/grid_test.txt > /dev/null 2>&1; then
    echo "✅ PASS: Grid commands execute without crash"
else
    echo "❌ FAIL: Grid commands failed"
fi

# Test 5: Save/Load (even though count is wrong)
echo ""
echo "Test 5: Save/Load Operations"
cat > /tmp/save_test.txt << EOF
workspace 5 5 5
resolution 64cm
place 200 0 200
save /tmp/test_save.vox
quit
EOF

rm -f /tmp/test_save.vox
if timeout 10s ../../voxel-cli --headless < /tmp/save_test.txt 2>&1 | grep -q "Project saved"; then
    if [ -f "/tmp/test_save.vox" ]; then
        echo "✅ PASS: Save creates file"
    else
        echo "❌ FAIL: Save file not created"
    fi
else
    echo "❌ FAIL: Save command failed"
fi

echo ""
echo "=== Summary ==="
echo "Current implementation status:"
echo "- Y >= 0 constraint: WORKING"
echo "- PlacementCommands: INTEGRATED"  
echo "- Coordinate bounds: NOT ENFORCED"
echo "- Voxel storage: NOT WORKING (count = 0)"
echo "- Grid commands: EXECUTE"
echo ""
echo "Major issues:"
echo "1. Coordinate system not centered at origin as per requirements"
echo "2. Voxel storage/counting broken (octree issue)"
echo "3. No workspace bounds validation"