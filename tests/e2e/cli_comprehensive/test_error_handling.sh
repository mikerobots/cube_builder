#!/bin/bash
# Test error handling and edge cases in headless mode

set -e
source "$(dirname "$0")/test_lib.sh"

TEST_DIR="$OUTPUT_BASE/headless/errors"
mkdir -p "$TEST_DIR"

# Test 1: Workspace boundary violations
init_test "Workspace Boundary Violations"
cat > "$TEST_DIR/boundary_cmd.txt" << EOF
workspace 5.0 5.0 5.0
resolution 8cm
place -300cm 0cm 0cm
place 0cm -300cm 0cm
place 0cm 0cm -300cm
place 600cm 0cm 0cm
place 0cm 600cm 0cm
place 0cm 0cm 600cm
status
quit
EOF

if execute_cli_capture "$TEST_DIR/boundary_cmd.txt" "$TEST_DIR/boundary_output.txt" 10 true; then
    if expect_output "$TEST_DIR/boundary_output.txt" "Voxels: 0"; then
        pass_test "Out-of-bounds placements correctly rejected"
    else
        fail_test "Out-of-bounds placements were not rejected"
    fi
else
    fail_test "Boundary violation test failed to execute"
fi

# Test 2: Y < 0 constraint
init_test "Ground Plane Constraint (Y >= 0)"
cat > "$TEST_DIR/ground_plane_cmd.txt" << EOF
workspace 5.0 5.0 5.0
resolution 16cm
place 96cm -1cm 96cm
place 96cm -50cm 96cm
place 96cm 0cm 96cm
place 96cm 48cm 96cm
status
quit
EOF

if execute_cli_capture "$TEST_DIR/ground_plane_cmd.txt" "$TEST_DIR/ground_plane_output.txt" 10 true; then
    if expect_output "$TEST_DIR/ground_plane_output.txt" "Voxels: 2"; then
        pass_test "Y < 0 placements correctly rejected"
    else
        fail_test "Negative Y placements were not rejected"
    fi
else
    fail_test "Ground plane constraint test failed"
fi

# Test 3: Overlap detection
init_test "Voxel Overlap Detection"
cat > "$TEST_DIR/overlap_cmd.txt" << EOF
workspace 5.0 5.0 5.0
resolution 32cm
place 96cm 96cm 96cm
place 96cm 96cm 96cm
place 96cm 96cm 96cm
place 192cm 192cm 192cm
status
quit
EOF

if execute_cli_capture "$TEST_DIR/overlap_cmd.txt" "$TEST_DIR/overlap_output.txt" 10 true; then
    # Should have only 2 voxels (first at 96,96,96 and one at 192,192,192)
    if expect_output "$TEST_DIR/overlap_output.txt" "Voxels: 2"; then
        pass_test "Overlapping placements correctly rejected"
    else
        fail_test "Overlap detection not working"
    fi
else
    fail_test "Overlap detection test failed"
fi

# Test 4: 1cm increment validation
init_test "1cm Increment Validation"
cat > "$TEST_DIR/increment_cmd.txt" << EOF
workspace 5.0 5.0 5.0
resolution 8cm
place 0cm 0cm 0cm
place 8cm 8cm 8cm
place 4.5cm 4.5cm 4.5cm
place 16cm 16cm 16cm
status
quit
EOF

if execute_cli_capture "$TEST_DIR/increment_cmd.txt" "$TEST_DIR/increment_output.txt" 10 true; then
    # Should have 3 voxels (0,0,0), (8,8,8), and (16,16,16)
    # The 4.5cm position should be rejected
    if expect_output "$TEST_DIR/increment_output.txt" "Voxels: 3"; then
        pass_test "Non-1cm increment placements rejected"
    else
        fail_test "1cm increment validation not working"
    fi
else
    fail_test "Increment validation test failed"
fi

# Test 5: Maximum undo/redo depth
init_test "Undo/Redo Limits"
cat > "$TEST_DIR/undo_limit_cmd.txt" << EOF
workspace 5.0 5.0 5.0
resolution 16cm
EOF

# Add many operations
for i in {0..150}; do
    echo "place $((i*10))cm 0cm $((i*10))cm" >> "$TEST_DIR/undo_limit_cmd.txt"
done

# Try to undo more than the limit
for i in {0..150}; do
    echo "undo" >> "$TEST_DIR/undo_limit_cmd.txt"
done

echo "status" >> "$TEST_DIR/undo_limit_cmd.txt"
echo "quit" >> "$TEST_DIR/undo_limit_cmd.txt"

if execute_cli_capture "$TEST_DIR/undo_limit_cmd.txt" "$TEST_DIR/undo_limit_output.txt" 30 true; then
    # Check that we don't crash and handle limits gracefully
    if expect_no_output "$TEST_DIR/undo_limit_output.txt" "Segmentation fault"; then
        pass_test "Undo/redo limits handled gracefully"
    else
        fail_test "Undo/redo limits caused crash"
    fi
else
    fail_test "Undo limit test failed"
fi

# Test 6: Invalid file operations
init_test "Invalid File Operations"
cat > "$TEST_DIR/invalid_file_cmd.txt" << EOF
load /nonexistent/path/file.vox
save /root/cannot_write_here.vox
export /dev/null/cannot_write.stl
workspace 5.0 5.0 5.0
place 0cm 0cm 0cm
save $TEST_DIR/valid_save.vox
quit
EOF

if execute_cli_capture "$TEST_DIR/invalid_file_cmd.txt" "$TEST_DIR/invalid_file_output.txt" 10 true; then
    if expect_output "$TEST_DIR/invalid_file_output.txt" "Error" || \
       expect_output "$TEST_DIR/invalid_file_output.txt" "Failed" || \
       expect_output "$TEST_DIR/invalid_file_output.txt" "Could not"; then
        pass_test "Invalid file operations handled gracefully"
    else
        fail_test "Invalid file operations not properly handled"
    fi
else
    fail_test "Invalid file operations test failed"
fi

# Test 7: Memory stress test (many voxels)
init_test "Memory Stress Test"
cat > "$TEST_DIR/memory_stress_cmd.txt" << EOF
workspace 8.0 8.0 8.0
resolution 1cm
EOF

# Place many voxels in a pattern (within 8m workspace bounds: -400cm to +400cm)
for x in {0..200..10}; do
    for y in {0..200..10}; do
        for z in {0..200..10}; do
            echo "place ${x}cm ${y}cm ${z}cm" >> "$TEST_DIR/memory_stress_cmd.txt"
        done
    done
done

echo "status" >> "$TEST_DIR/memory_stress_cmd.txt"
echo "quit" >> "$TEST_DIR/memory_stress_cmd.txt"

if execute_cli_capture "$TEST_DIR/memory_stress_cmd.txt" "$TEST_DIR/memory_stress_output.txt" 60 true; then
    if expect_output "$TEST_DIR/memory_stress_output.txt" "Voxels:"; then
        pass_test "Memory stress test completed without crash"
    else
        fail_test "Memory stress test output incorrect"
    fi
else
    fail_test "Memory stress test failed or timed out"
fi

# Test 8: Resolution switching with existing voxels
init_test "Resolution Switching Safety"
cat > "$TEST_DIR/resolution_switch_cmd.txt" << EOF
workspace 5.0 5.0 5.0
resolution 32cm
place 96cm 96cm 96cm
resolution 8cm
place 96cm 96cm 96cm
resolution 64cm
place 128cm 128cm 128cm
status
quit
EOF

if execute_cli_capture "$TEST_DIR/resolution_switch_cmd.txt" "$TEST_DIR/resolution_switch_output.txt" 10 true; then
    # Should handle resolution switches gracefully
    if expect_output "$TEST_DIR/resolution_switch_output.txt" "Voxels:"; then
        pass_test "Resolution switching handled safely"
    else
        fail_test "Resolution switching caused issues"
    fi
else
    fail_test "Resolution switching test failed"
fi

# Print summary
print_summary