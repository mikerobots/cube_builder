#!/bin/bash
# Simple test of enhancement features

set -e
source "$(dirname "$0")/test_lib.sh"

TEST_DIR="$OUTPUT_BASE/enhancements"
mkdir -p "$TEST_DIR"

# Test 1: Y < 0 constraint (headless)
init_test "Ground Plane Constraint (Y >= 0)"
cat > "$TEST_DIR/ground_cmd.txt" << EOF
workspace 5 5 5
resolution 64cm
place 0cm -10cm 0cm
place 0cm 0cm 0cm
place 0cm 64cm 0cm
status
quit
EOF

if timeout 10s "$CLI_PATH" --headless < "$TEST_DIR/ground_cmd.txt" > "$TEST_DIR/ground_output.txt" 2>&1; then
    # Count how many times "Voxel placed" appears
    PLACED_COUNT=$(grep -c "Voxel placed" "$TEST_DIR/ground_output.txt" 2>/dev/null || echo 0)
    if [ "$PLACED_COUNT" -eq 2 ]; then
        pass_test "Y < 0 constraint working (2 voxels placed, 1 rejected)"
    else
        fail_test "Y < 0 constraint not working (expected 2 placements, got $PLACED_COUNT)"
        echo "Output:"
        grep -E "placed|failed|error|Total voxels" "$TEST_DIR/ground_output.txt" || true
    fi
else
    fail_test "Ground plane test failed"
fi

# Test 2: Overlap detection (headless)
init_test "Overlap Detection"
cat > "$TEST_DIR/overlap_cmd.txt" << EOF
workspace 5 5 5
resolution 64cm
place 0cm 0cm 0cm
place 0cm 0cm 0cm
place 64cm 0cm 0cm
status
quit
EOF

if timeout 10s "$CLI_PATH" --headless < "$TEST_DIR/overlap_cmd.txt" > "$TEST_DIR/overlap_output.txt" 2>&1; then
    PLACED_COUNT=$(grep -c "Voxel placed" "$TEST_DIR/overlap_output.txt" 2>/dev/null || echo 0)
    if [ "$PLACED_COUNT" -eq 2 ]; then
        pass_test "Overlap detection working (2 voxels placed, duplicate rejected)"
    else
        fail_test "Overlap detection not working (expected 2 placements, got $PLACED_COUNT)"
        echo "Output:"
        grep -E "placed|failed|error|overlap|Total voxels" "$TEST_DIR/overlap_output.txt" || true
    fi
else
    fail_test "Overlap test failed"
fi

# Test 3: Grid-aligned positions (headless)
init_test "Grid-Aligned Positions"
cat > "$TEST_DIR/increment_cmd.txt" << EOF
workspace 5 5 5
resolution 16cm
place 0cm 0cm 0cm
place 16cm 0cm 16cm
place 16cm 0cm 16cm
status
quit
EOF

if timeout 10s "$CLI_PATH" --headless < "$TEST_DIR/increment_cmd.txt" > "$TEST_DIR/increment_output.txt" 2>&1; then
    PLACED_COUNT=$(grep -c "Voxel placed" "$TEST_DIR/increment_output.txt" 2>/dev/null || echo 0)
    if [ "$PLACED_COUNT" -eq 2 ]; then
        pass_test "Grid-aligned positions working (2 voxels placed, duplicate rejected)"
    else
        fail_test "Grid-aligned positions not working (expected 2 placements, got $PLACED_COUNT)"
    fi
else
    fail_test "Increment test failed"
fi

# Test 4: Grid rendering (if display available)
if [ -n "$DISPLAY" ] || [ -n "$WAYLAND_DISPLAY" ] || [ "$(uname)" == "Darwin" ]; then
    init_test "Grid Visibility (Rendering)"
    cat > "$TEST_DIR/grid_render_cmd.txt" << EOF
workspace 5 5 5
view iso
grid on
screenshot $TEST_DIR/grid_on.ppm
grid off
screenshot $TEST_DIR/grid_off.ppm
quit
EOF
    
    if timeout 30s "$CLI_PATH" < "$TEST_DIR/grid_render_cmd.txt" > "$TEST_DIR/grid_render_output.txt" 2>&1; then
        # Check if screenshots were created
        if [ -f "$TEST_DIR/grid_on.ppm" ] || [ -f "$TEST_DIR/grid_on.ppm.ppm" ]; then
            pass_test "Grid rendering test completed (screenshots created)"
        else
            fail_test "Grid rendering failed (no screenshots)"
        fi
    else
        fail_test "Grid rendering test timed out"
    fi
else
    echo "Skipping rendering tests (no display environment)"
fi

# Print summary
print_summary