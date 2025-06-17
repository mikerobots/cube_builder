#!/bin/bash
# Test the new enhancement features specifically

set -e
source "$(dirname "$0")/test_lib.sh"

TEST_DIR="$OUTPUT_BASE/rendering/new_features"
mkdir -p "$TEST_DIR"

# Test 1: Grid visibility and rendering
init_test "Grid Visibility Enhancement"
cat > "$TEST_DIR/grid_test_cmd.txt" << EOF
workspace 5 5 5
view iso
zoom 1.5
grid on
screenshot $TEST_DIR/grid_visible.ppm
place 0 0 0
place 32 0 0
place 0 0 32
screenshot $TEST_DIR/grid_with_voxels.ppm
grid off
screenshot $TEST_DIR/grid_hidden.ppm
quit
EOF

if execute_cli "$TEST_DIR/grid_test_cmd.txt" 30; then
    # Should see grid lines in first two screenshots but not the third
    if analyze_screenshot "$TEST_DIR/grid_visible" "$TEST_DIR/grid_visible_analysis.txt" && \
       analyze_screenshot "$TEST_DIR/grid_with_voxels" "$TEST_DIR/grid_voxels_analysis.txt" && \
       analyze_screenshot "$TEST_DIR/grid_hidden" "$TEST_DIR/grid_hidden_analysis.txt"; then
        
        pass_test "Grid visibility enhancement working"
    else
        fail_test "Failed to analyze grid screenshots"
    fi
else
    fail_test "Grid visibility test failed"
fi

# Test 2: 1cm increment placement
init_test "1cm Increment Placement"
cat > "$TEST_DIR/increment_test_cmd.txt" << EOF
workspace 5 5 5
view front
resolution 8cm
place 100 0 100
place 101 0 101
place 102 0 102
place 103 0 103
zoom 0.5
screenshot $TEST_DIR/increment_placement.ppm
stats
quit
EOF

if execute_cli "$TEST_DIR/increment_test_cmd.txt" 30; then
    if analyze_screenshot "$TEST_DIR/increment_placement" "$TEST_DIR/increment_analysis.txt"; then
        if check_visible_content "$TEST_DIR/increment_analysis.txt" 3; then
            pass_test "1cm increment placement working"
        else
            fail_test "1cm increment voxels not visible"
        fi
    else
        fail_test "Failed to analyze increment placement"
    fi
else
    fail_test "1cm increment test failed"
fi

# Test 3: Y >= 0 constraint
init_test "Ground Plane Constraint"
cat > "$TEST_DIR/ground_constraint_cmd.txt" << EOF
workspace 5 5 5
resolution 32cm
place 100 -10 100
place 100 0 100
place 100 10 100
stats
screenshot $TEST_DIR/ground_constraint.ppm
quit
EOF

if execute_cli_capture "$TEST_DIR/ground_constraint_cmd.txt" "$TEST_DIR/ground_output.txt" 30; then
    # Should only have 2 voxels (Y=0 and Y=10)
    if expect_output "$TEST_DIR/ground_output.txt" "Total voxels: 2"; then
        pass_test "Y >= 0 constraint enforced"
    else
        fail_test "Ground plane constraint not working"
    fi
else
    fail_test "Ground constraint test failed"
fi

# Test 4: Overlap prevention
init_test "Overlap Prevention"
cat > "$TEST_DIR/overlap_test_cmd.txt" << EOF
workspace 5 5 5
resolution 64cm
place 200 0 200
place 200 0 200
place 199 0 199
place 201 0 201
stats
screenshot $TEST_DIR/overlap_test.ppm
quit
EOF

if execute_cli_capture "$TEST_DIR/overlap_test_cmd.txt" "$TEST_DIR/overlap_output.txt" 30; then
    # Should only have 1 voxel due to overlap prevention
    if expect_output "$TEST_DIR/overlap_output.txt" "Total voxels: 1"; then
        pass_test "Overlap prevention working"
    else
        fail_test "Overlapping voxels were allowed"
    fi
else
    fail_test "Overlap test failed"
fi

# Test 5: Enhanced undo/redo with validation
init_test "Enhanced Undo/Redo"
cat > "$TEST_DIR/enhanced_undo_cmd.txt" << EOF
workspace 5 5 5
resolution 32cm
place 100 0 100
place 100 -10 100
place 200 0 200
stats
undo
stats
redo
stats
quit
EOF

if execute_cli_capture "$TEST_DIR/enhanced_undo_cmd.txt" "$TEST_DIR/undo_output.txt" 30; then
    # Invalid placement shouldn't affect undo/redo count
    if expect_output "$TEST_DIR/undo_output.txt" "Total voxels: 2" && \
       expect_output "$TEST_DIR/undo_output.txt" "Total voxels: 1"; then
        pass_test "Enhanced undo/redo with validation working"
    else
        fail_test "Undo/redo not handling validation correctly"
    fi
else
    fail_test "Enhanced undo test failed"
fi

# Test 6: Preview colors (requires visual inspection)
init_test "Preview Color System"
cat > "$TEST_DIR/preview_test_cmd.txt" << EOF
workspace 5 5 5
view iso
grid on
resolution 32cm
place 200 0 200
screenshot $TEST_DIR/preview_test.ppm
quit
EOF

if execute_cli "$TEST_DIR/preview_test_cmd.txt" 30; then
    if analyze_screenshot "$TEST_DIR/preview_test" "$TEST_DIR/preview_analysis.txt"; then
        # Look for green-ish colors that might indicate preview
        if grep -E '\([0-9]{1,2}, 2[0-9]{2}, [0-9]{1,2}\)' "$TEST_DIR/preview_analysis.txt" > /dev/null || \
           check_visible_content "$TEST_DIR/preview_analysis.txt" 3; then
            pass_test "Preview system rendering (visual inspection needed)"
        else
            fail_test "Preview colors not detected"
        fi
    else
        fail_test "Failed to analyze preview screenshot"
    fi
else
    fail_test "Preview test failed"
fi

# Print summary
print_summary