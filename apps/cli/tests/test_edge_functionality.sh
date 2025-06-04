#!/bin/bash

# Functional test for edge rendering
echo "Edge Rendering Functionality Test"
echo "================================="

PASSED=0
FAILED=0

# Function to run a test
run_test() {
    local test_name="$1"
    local commands="$2"
    local expected="$3"
    
    echo -n "Testing $test_name... "
    
    OUTPUT=$(timeout 3 ./build_ninja/bin/VoxelEditor_CLI --headless 2>&1 <<EOF
$commands
quit
EOF
)
    
    # Check for crashes
    if echo "$OUTPUT" | grep -qi "segmentation\|abort\|crash\|exception\|core dumped"; then
        echo "✗ CRASHED"
        FAILED=$((FAILED + 1))
        return 1
    fi
    
    # Check for expected output
    if echo "$OUTPUT" | grep -q "$expected"; then
        echo "✓ PASSED"
        PASSED=$((PASSED + 1))
        return 0
    else
        echo "✗ FAILED (expected: $expected)"
        FAILED=$((FAILED + 1))
        return 1
    fi
}

# Test 1: Basic edge rendering
run_test "basic edge rendering" \
"edges on
place 0 0 0" \
"Edge rendering enabled"

# Test 2: Edge toggle
run_test "edge toggle" \
"edges on
edges off
edges toggle" \
"Edge rendering enabled"

# Test 3: Multiple voxels with edges
run_test "multiple voxels" \
"edges on
place 0 0 0
place 1 0 0
place 0 1 0
place 0 0 1" \
"Voxel placed at (0, 0, 1)"

# Test 4: Shader switching
run_test "shader switching" \
"place 0 0 0
shader basic
shader enhanced
shader flat" \
"Shader mode set to"

# Test 5: Edges with different shaders
run_test "edges with shaders" \
"edges on
shader basic
place 0 0 0
shader enhanced
place 1 0 0
shader flat
place 2 0 0" \
"Voxel placed at (2, 0, 0)"

# Test 6: Large scene with edges
run_test "large scene" \
"edges on
place 0 0 0
place 1 0 0
place 2 0 0
place 0 1 0
place 1 1 0
place 2 1 0
place 0 2 0
place 1 2 0
place 2 2 0
place 1 1 1
info" \
"Voxels: 10"

# Summary
echo ""
echo "Summary:"
echo "========"
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ $FAILED -eq 0 ]; then
    echo ""
    echo "✅ All edge rendering functionality tests passed!"
    echo "Note: GL_INVALID_ENUM warnings are known and don't affect functionality."
    exit 0
else
    echo ""
    echo "❌ Some tests failed!"
    exit 1
fi