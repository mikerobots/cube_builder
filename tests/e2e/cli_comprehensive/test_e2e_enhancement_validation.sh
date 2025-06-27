#!/bin/bash
# Validate enhancement features in headless mode

set -e
source "$(dirname "$0")/test_lib.sh"

TEST_DIR="$OUTPUT_BASE/headless/enhancements"
mkdir -p "$TEST_DIR"

# Test 1: Grid commands validation
init_test "Grid Commands Validation"
cat > "$TEST_DIR/grid_validation_cmd.txt" << EOF
grid
grid on
grid
grid off
grid
grid toggle
grid
quit
EOF

if execute_cli_capture "$TEST_DIR/grid_validation_cmd.txt" "$TEST_DIR/grid_validation_output.txt" 10 true; then
    if expect_output "$TEST_DIR/grid_validation_output.txt" "Grid command not available in headless mode"; then
        pass_test "Grid commands correctly report headless mode limitation"
    else
        fail_test "Grid commands not reporting headless mode limitation"
    fi
else
    fail_test "Grid commands failed"
fi

# Test 2: PlacementCommands validation - test exact position placement
init_test "PlacementCommands Validation (Exact Positions)"
cat > "$TEST_DIR/placement_validation_cmd.txt" << EOF
workspace 5 5 5
resolution 64cm
place 97cm -10cm 96cm
place 96.5cm 0cm 96cm
place 97cm 3cm 103cm
place 97cm 3cm 103cm
place 133cm 7cm 127cm
status
quit
EOF

if execute_cli_capture "$TEST_DIR/placement_validation_cmd.txt" "$TEST_DIR/placement_output.txt" 10 true; then
    # Should reject: Y<0 (97cm -10cm 96cm), non-1cm-increment (96.5cm), duplicate (97cm 3cm 103cm)
    # Should accept: 97cm 3cm 103cm and 133cm 7cm 127cm (exact 1cm positions)
    if expect_output "$TEST_DIR/placement_output.txt" "Voxels: 2"; then
        pass_test "PlacementCommands validation working"
    else
        fail_test "PlacementCommands validation incorrect"
    fi
else
    fail_test "PlacementCommands test failed"
fi

# Test 3: Workspace boundary validation
init_test "Workspace Boundary Validation"
cat > "$TEST_DIR/boundary_validation_cmd.txt" << EOF
workspace 3 3 3
resolution 16cm
place 0cm 0cm 0cm
place 16cm 0cm 0cm
place 284cm 0cm 0cm
place 300cm 0cm 0cm
place 0cm 0cm 284cm
place 0cm 0cm 300cm
status
quit
EOF

if execute_cli_capture "$TEST_DIR/boundary_validation_cmd.txt" "$TEST_DIR/boundary_output.txt" 10 true; then
    # 3m workspace = -150cm to +150cm from origin, so positions 284cm and 300cm are outside
    if expect_output "$TEST_DIR/boundary_output.txt" "Voxels: 2"; then
        pass_test "Workspace boundary validation correct"
    else
        fail_test "Workspace boundaries not enforced correctly"
    fi
else
    fail_test "Boundary validation test failed"
fi

# Test 4: Multi-resolution overlap detection
init_test "Multi-Resolution Overlap Detection"
cat > "$TEST_DIR/multi_res_overlap_cmd.txt" << EOF
workspace 5 5 5
resolution 64cm
place 64cm 0cm 64cm
resolution 16cm
place 64cm 0cm 64cm
place 80cm 0cm 64cm
place 96cm 0cm 64cm
place 112cm 0cm 64cm
place 128cm 0cm 64cm
status
quit
EOF

if execute_cli_capture "$TEST_DIR/multi_res_overlap_cmd.txt" "$TEST_DIR/multi_res_output.txt" 10 true; then
    # 64cm voxel at 64,0,64 blocks smaller voxels in its space
    # Only the 128cm position should succeed (outside the 64cm voxel)
    if expect_output "$TEST_DIR/multi_res_output.txt" "Voxels: 2"; then
        pass_test "Multi-resolution overlap detection working"
    else
        fail_test "Multi-resolution overlaps not detected correctly"
    fi
else
    fail_test "Multi-resolution overlap test failed"
fi

# Test 5: Enhanced undo/redo with failed commands
init_test "Undo/Redo with Failed Commands"
cat > "$TEST_DIR/undo_failed_cmd.txt" << EOF
workspace 5 5 5
resolution 64cm
place 64cm 0cm 64cm
place 64cm -64cm 64cm
place 128cm 0cm 128cm
place 128cm 0cm 128cm
place 192cm 0cm 192cm
status
undo
undo
status
redo
redo
status
quit
EOF

if execute_cli_capture "$TEST_DIR/undo_failed_cmd.txt" "$TEST_DIR/undo_failed_output.txt" 10 true; then
    # Failed commands shouldn't be in undo history
    # Should see: 2 voxels -> 0 voxels -> 2 voxels
    output=$(cat "$TEST_DIR/undo_failed_output.txt")
    count1=$(echo "$output" | grep -m1 "Voxels:" | grep -oE '[0-9]+')
    count2=$(echo "$output" | grep -m2 "Voxels:" | tail -1 | grep -oE '[0-9]+')
    count3=$(echo "$output" | grep -m3 "Voxels:" | tail -1 | grep -oE '[0-9]+')
    
    if [ "$count1" = "2" ] && [ "$count2" = "0" ] && [ "$count3" = "2" ]; then
        pass_test "Undo/redo correctly skips failed commands"
    else
        fail_test "Undo/redo history includes failed commands (got $count1->$count2->$count3, expected 2->0->2)"
    fi
else
    fail_test "Undo with failed commands test failed"
fi

# Test 6: 1cm grid snapping
init_test "1cm Grid Snapping Validation"
cat > "$TEST_DIR/snapping_cmd.txt" << EOF
workspace 5 5 5
resolution 16cm
place 0cm 0cm 0cm
place 1cm 0cm 0cm
place 7cm 0cm 0cm
place 16cm 0cm 0cm
place 32cm 0cm 0cm
status
quit
EOF

if execute_cli_capture "$TEST_DIR/snapping_cmd.txt" "$TEST_DIR/snapping_output.txt" 10 true; then
    # All positions are valid 1cm increments, but overlaps are prevented
    # 16cm voxels at 0cm, 16cm, and 32cm don't overlap, so 3 voxels total
    if expect_output "$TEST_DIR/snapping_output.txt" "Voxels: 3"; then
        pass_test "1cm grid snapping with overlap prevention working"
    else
        fail_test "1cm snapping rejecting valid positions"
    fi
else
    fail_test "Grid snapping test failed"
fi

# Test 7: Large voxel with small voxel subdivision
init_test "Large/Small Voxel Subdivision"
cat > "$TEST_DIR/subdivision_cmd.txt" << EOF
workspace 5 5 5
resolution 256cm
place 0cm 0cm 0cm
resolution 16cm
EOF

# Place 4x4 grid of 16cm voxels within workspace bounds
for x in $(seq -64 16 48); do
    for z in $(seq -64 16 48); do
        echo "place ${x}cm 16cm ${z}cm" >> "$TEST_DIR/subdivision_cmd.txt"
    done
done

echo "status" >> "$TEST_DIR/subdivision_cmd.txt"
echo "quit" >> "$TEST_DIR/subdivision_cmd.txt"

if execute_cli_capture "$TEST_DIR/subdivision_cmd.txt" "$TEST_DIR/subdivision_output.txt" 30 true; then
    # 256cm voxel fails (outside workspace), only 16cm voxels succeed
    # 8x8 grid of 16cm voxels = 64 total
    if expect_output "$TEST_DIR/subdivision_output.txt" "Voxels: 64"; then
        pass_test "Large/small voxel subdivision working (large voxel rejected)"
    else
        fail_test "Subdivision placement count incorrect"
    fi
else
    fail_test "Subdivision test failed"
fi

# Test 8: Performance - rapid command execution
init_test "Rapid Command Performance"
START_TIME=$(date +%s.%N)
cat > "$TEST_DIR/rapid_cmd.txt" << EOF
workspace 5 5 5
resolution 16cm
EOF

# Generate 100 place/remove pairs
for i in $(seq 0 99); do
    echo "place $((i*16))cm 0cm 96cm" >> "$TEST_DIR/rapid_cmd.txt"
    echo "delete $((i*16))cm 0cm 96cm" >> "$TEST_DIR/rapid_cmd.txt"
done

echo "status" >> "$TEST_DIR/rapid_cmd.txt"
echo "quit" >> "$TEST_DIR/rapid_cmd.txt"

if execute_cli_capture "$TEST_DIR/rapid_cmd.txt" "$TEST_DIR/rapid_output.txt" 30 true; then
    END_TIME=$(date +%s.%N)
    ELAPSED=$(echo "$END_TIME - $START_TIME" | bc)
    
    if expect_output "$TEST_DIR/rapid_output.txt" "Voxels: 0"; then
        pass_test "Rapid command execution completed in ${ELAPSED}s"
        echo "Performance: 200 operations in ${ELAPSED}s" > "$OUTPUT_BASE/metrics/rapid_commands.txt"
    else
        fail_test "Rapid command execution produced incorrect result"
    fi
else
    fail_test "Rapid command test failed or timed out"
fi

# Print summary
print_summary