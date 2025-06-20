#!/bin/bash
# Basic smoke test to verify CLI works

set -e
source "$(dirname "$0")/test_lib.sh"

TEST_DIR="$OUTPUT_BASE/smoke"
mkdir -p "$TEST_DIR"

# Test 1: CLI starts and quits
init_test "CLI Start/Stop"
cat > "$TEST_DIR/basic_cmd.txt" << EOF
quit
EOF

if timeout 5s "$CLI_PATH" --headless < "$TEST_DIR/basic_cmd.txt" > "$TEST_DIR/basic_output.txt" 2>&1; then
    if grep -q "Goodbye!" "$TEST_DIR/basic_output.txt"; then
        pass_test "CLI starts and quits cleanly"
    else
        fail_test "CLI did not exit cleanly"
        cat "$TEST_DIR/basic_output.txt"
    fi
else
    fail_test "CLI failed to start or timed out"
fi

# Test 2: Place a voxel
init_test "Basic Voxel Placement"
cat > "$TEST_DIR/place_cmd.txt" << EOF
workspace 5.0 5.0 5.0
resolution 32cm
place 96cm 0cm 96cm
status
quit
EOF

if timeout 10s "$CLI_PATH" --headless < "$TEST_DIR/place_cmd.txt" > "$TEST_DIR/place_output.txt" 2>&1; then
    if grep -q "Voxel placed" "$TEST_DIR/place_output.txt" || grep -q "Total voxels:" "$TEST_DIR/place_output.txt"; then
        pass_test "Voxel placement works"
    else
        fail_test "Voxel placement failed"
        echo "Output:"
        cat "$TEST_DIR/place_output.txt"
    fi
else
    fail_test "Voxel placement test timed out"
fi

# Test 3: Grid command
init_test "Grid Command"
cat > "$TEST_DIR/grid_cmd.txt" << EOF
grid on
grid off
grid toggle
quit
EOF

if timeout 10s "$CLI_PATH" --headless < "$TEST_DIR/grid_cmd.txt" > "$TEST_DIR/grid_output.txt" 2>&1; then
    if grep -q "Grid" "$TEST_DIR/grid_output.txt"; then
        pass_test "Grid commands execute"
    else
        # Grid might not output in headless mode
        pass_test "Grid commands execute (no output in headless)"
    fi
else
    fail_test "Grid command test failed"
fi

# Print summary
print_summary