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

# Test 2: PlacementCommands validation
init_test "PlacementCommands Validation"
cat > "$TEST_DIR/placement_validation_cmd.txt" << EOF
workspace 5 5 5
resolution 32cm
place 96cm -10cm 96cm
place 96.5cm 0cm 96cm
place 96cm 0cm 96cm
place 96cm 0cm 96cm
place 128cm 0cm 128cm
status
quit
EOF

if execute_cli_capture "$TEST_DIR/placement_validation_cmd.txt" "$TEST_DIR/placement_output.txt" 10 true; then
    # Should reject: Y<0, non-integer, duplicate
    # Should accept: 100,0,100 and 101,0,101
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
    # 3m workspace = 300cm, so valid positions are 0-284 with 16cm voxels
    if expect_output "$TEST_DIR/boundary_output.txt" "Voxels: 4"; then
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
place 200cm 0cm 200cm
resolution 32cm
place 200cm 0cm 200cm
place 232cm 0cm 200cm
place 200cm 0cm 232cm
place 232cm 0cm 232cm
place 264cm 0cm 200cm
status
quit
EOF

if execute_cli_capture "$TEST_DIR/multi_res_overlap_cmd.txt" "$TEST_DIR/multi_res_output.txt" 10 true; then
    # 64cm voxel at 200,0,200 blocks 32cm voxels in its space
    # Should have: 1x64cm + 2x32cm voxels
    if expect_output "$TEST_DIR/multi_res_output.txt" "Voxels: 3"; then
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
place 100cm 0cm 100cm
place 100cm -10cm 100cm
place 200cm 0cm 200cm
place 200cm 0cm 200cm
place 300cm 0cm 300cm
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
    # Should see: 3 voxels -> 1 voxel -> 3 voxels
    local output=$(cat "$TEST_DIR/undo_failed_output.txt")
    local count1=$(echo "$output" | grep -m1 "Voxels:" | grep -oE '[0-9]+')
    local count2=$(echo "$output" | grep -m2 "Voxels:" | tail -1 | grep -oE '[0-9]+')
    local count3=$(echo "$output" | grep -m3 "Voxels:" | tail -1 | grep -oE '[0-9]+')
    
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
place 0cm 0cm 0cm
place 1cm 0cm 0cm
place 7cm 0cm 0cm
place 8cm 0cm 0cm
place 9cm 0cm 0cm
status
quit
EOF

if execute_cli_capture "$TEST_DIR/snapping_cmd.txt" "$TEST_DIR/snapping_output.txt" 10 true; then
    # All positions are valid 1cm increments
    if expect_output "$TEST_DIR/snapping_output.txt" "Voxels: 5"; then
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
place 200cm 0cm 200cm
resolution 16cm
EOF

# Place 8x8 grid of 16cm voxels on top of 128cm voxel
for x in $(seq 200 16 312); do
    for z in $(seq 200 16 312); do
        echo "place ${x}cm 128cm ${z}cm" >> "$TEST_DIR/subdivision_cmd.txt"
    done
done

echo "status" >> "$TEST_DIR/subdivision_cmd.txt"
echo "quit" >> "$TEST_DIR/subdivision_cmd.txt"

if execute_cli_capture "$TEST_DIR/subdivision_cmd.txt" "$TEST_DIR/subdivision_output.txt" 30 true; then
    # Should have 1 large + 64 small voxels
    if expect_output "$TEST_DIR/subdivision_output.txt" "Voxels: 65"; then
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
    echo "place $((i*8))cm 0cm 100cm" >> "$TEST_DIR/rapid_cmd.txt"
    echo "delete $((i*8))cm 0cm 100cm" >> "$TEST_DIR/rapid_cmd.txt"
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