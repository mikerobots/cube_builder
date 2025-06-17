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
    if expect_output "$TEST_DIR/grid_validation_output.txt" "Grid: " && \
       expect_output "$TEST_DIR/grid_validation_output.txt" "enabled\|on\|true\|disabled\|off\|false"; then
        pass_test "Grid commands function correctly"
    else
        fail_test "Grid commands not reporting status"
    fi
else
    fail_test "Grid commands failed"
fi

# Test 2: PlacementCommands validation
init_test "PlacementCommands Validation"
cat > "$TEST_DIR/placement_validation_cmd.txt" << EOF
workspace 5 5 5
resolution 32cm
place 100 -10 100
place 100.5 0 100
place 100 0 100
place 100 0 100
place 101 0 101
stats
quit
EOF

if execute_cli_capture "$TEST_DIR/placement_validation_cmd.txt" "$TEST_DIR/placement_output.txt" 10 true; then
    # Should reject: Y<0, non-integer, duplicate
    # Should accept: 100,0,100 and 101,0,101
    if expect_output "$TEST_DIR/placement_output.txt" "Total voxels: 2"; then
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
place 0 0 0
place 16 0 0
place 284 0 0
place 300 0 0
place 0 0 284
place 0 0 300
stats
quit
EOF

if execute_cli_capture "$TEST_DIR/boundary_validation_cmd.txt" "$TEST_DIR/boundary_output.txt" 10 true; then
    # 3m workspace = 300cm, so valid positions are 0-284 with 16cm voxels
    if expect_output "$TEST_DIR/boundary_output.txt" "Total voxels: 4"; then
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
place 200 0 200
resolution 32cm
place 200 0 200
place 232 0 200
place 200 0 232
place 232 0 232
place 264 0 200
stats
quit
EOF

if execute_cli_capture "$TEST_DIR/multi_res_overlap_cmd.txt" "$TEST_DIR/multi_res_output.txt" 10 true; then
    # 64cm voxel at 200,0,200 blocks 32cm voxels in its space
    # Should have: 1x64cm + 2x32cm voxels
    if expect_output "$TEST_DIR/multi_res_output.txt" "Total voxels: 3"; then
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
resolution 32cm
place 100 0 100
place 100 -10 100
place 200 0 200
place 200 0 200
place 300 0 300
stats
undo
undo
stats
redo
redo
stats
quit
EOF

if execute_cli_capture "$TEST_DIR/undo_failed_cmd.txt" "$TEST_DIR/undo_failed_output.txt" 10 true; then
    # Failed commands shouldn't be in undo history
    # Should see: 3 voxels -> 1 voxel -> 3 voxels
    local output=$(cat "$TEST_DIR/undo_failed_output.txt")
    local count1=$(echo "$output" | grep -m1 "Total voxels:" | grep -oE '[0-9]+')
    local count2=$(echo "$output" | grep -m2 "Total voxels:" | tail -1 | grep -oE '[0-9]+')
    local count3=$(echo "$output" | grep -m3 "Total voxels:" | tail -1 | grep -oE '[0-9]+')
    
    if [ "$count1" = "3" ] && [ "$count2" = "1" ] && [ "$count3" = "3" ]; then
        pass_test "Undo/redo correctly skips failed commands"
    else
        fail_test "Undo/redo history includes failed commands"
    fi
else
    fail_test "Undo with failed commands test failed"
fi

# Test 6: 1cm grid snapping
init_test "1cm Grid Snapping Validation"
cat > "$TEST_DIR/snapping_cmd.txt" << EOF
workspace 5 5 5
resolution 8cm
place 0 0 0
place 1 0 0
place 7 0 0
place 8 0 0
place 9 0 0
stats
quit
EOF

if execute_cli_capture "$TEST_DIR/snapping_cmd.txt" "$TEST_DIR/snapping_output.txt" 10 true; then
    # All positions are valid 1cm increments
    if expect_output "$TEST_DIR/snapping_output.txt" "Total voxels: 5"; then
        pass_test "1cm grid snapping allows all integer positions"
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
resolution 128cm
place 200 0 200
resolution 16cm
EOF

# Place 8x8 grid of 16cm voxels on top of 128cm voxel
for x in $(seq 200 16 312); do
    for z in $(seq 200 16 312); do
        echo "place $x 128 $z" >> "$TEST_DIR/subdivision_cmd.txt"
    done
done

echo "stats" >> "$TEST_DIR/subdivision_cmd.txt"
echo "quit" >> "$TEST_DIR/subdivision_cmd.txt"

if execute_cli_capture "$TEST_DIR/subdivision_cmd.txt" "$TEST_DIR/subdivision_output.txt" 30 true; then
    # Should have 1 large + 64 small voxels
    if expect_output "$TEST_DIR/subdivision_output.txt" "Total voxels: 65"; then
        pass_test "Large/small voxel subdivision working"
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
resolution 8cm
EOF

# Generate 100 place/remove pairs
for i in $(seq 0 99); do
    echo "place $((i*8)) 0 100" >> "$TEST_DIR/rapid_cmd.txt"
    echo "remove $((i*8)) 0 100" >> "$TEST_DIR/rapid_cmd.txt"
done

echo "stats" >> "$TEST_DIR/rapid_cmd.txt"
echo "quit" >> "$TEST_DIR/rapid_cmd.txt"

if execute_cli_capture "$TEST_DIR/rapid_cmd.txt" "$TEST_DIR/rapid_output.txt" 30 true; then
    END_TIME=$(date +%s.%N)
    ELAPSED=$(echo "$END_TIME - $START_TIME" | bc)
    
    if expect_output "$TEST_DIR/rapid_output.txt" "Total voxels: 0"; then
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