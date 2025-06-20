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
place 0cm 0cm 0cm" \
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
place 0cm 0cm 0cm
place 1cm 0cm 0cm
place 0cm 1cm 0cm
place 0cm 0cm 1cm" \
"Voxel placed at (0cm, 0cm, 1cm)"

# Test 4: Shader switching
run_test "shader switching" \
"place 0cm 0cm 0cm
shader basic
shader enhanced
shader flat" \
"Shader mode set to"

# Test 5: Edges with different shaders
run_test "edges with shaders" \
"edges on
shader basic
place 0cm 0cm 0cm
shader enhanced
place 1cm 0cm 0cm
shader flat
place 2cm 0cm 0cm" \
"Voxel placed at (2cm, 0cm, 0cm)"

# Test 6: Large scene with edges
run_test "large scene" \
"edges on
place 0cm 0cm 0cm
place 1cm 0cm 0cm
place 2cm 0cm 0cm
place 0cm 1cm 0cm
place 1cm 1cm 0cm
place 2cm 1cm 0cm
place 0cm 2cm 0cm
place 1cm 2cm 0cm
place 2cm 2cm 0cm
place 1cm 1cm 1cm
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