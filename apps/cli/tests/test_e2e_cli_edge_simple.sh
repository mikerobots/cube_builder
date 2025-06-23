#!/bin/bash

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

# Change to project root
cd "$PROJECT_ROOT"

# Simple edge rendering test
echo "Testing edge rendering..."
echo "========================="

PASSED=0
FAILED=0

# Function to run a test
run_test() {
    local test_name="$1"
    local commands="$2"
    local expected="$3"
    local headless="$4"
    
    echo -n "Testing $test_name... "
    
    local flags=""
    if [ "$headless" = "true" ]; then
        flags="--headless"
    fi
    
    OUTPUT=$(./execute_command.sh timeout 3 ./build_ninja/bin/VoxelEditor_CLI $flags 2>&1 <<EOF || true
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

# Test 1: Basic placement without edges
run_test "basic placement" \
"place 0cm 0cm 0cm" \
"Voxel placed at (0, 0, 0)" \
"false"

# Test 2: Edge rendering
run_test "edge rendering" \
"edges on
place 0cm 0cm 0cm" \
"Edge rendering enabled" \
"false"

# Test 3: Shader commands in headless mode
run_test "shader in headless" \
"shader basic" \
"Shader command not available in headless mode" \
"true"

# Test 4: Centered coordinate system
run_test "centered coordinates" \
"resolution 16cm
place -16cm 0cm -16cm
place 16cm 0cm 16cm" \
"Voxel placed at (16, 0, 16)" \
"true"

# Test 5: Multiple resolutions
run_test "resolution switching" \
"resolution 16cm
place 0cm 0cm 0cm
resolution 32cm
place 32cm 0cm 32cm" \
"Voxel placed at (32, 0, 32)" \
"true"

# Summary
echo ""
echo "Summary:"
echo "========"
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ $FAILED -eq 0 ]; then
    echo ""
    echo "✅ All edge simple tests passed!"
    exit 0
else
    echo ""
    echo "❌ Some tests failed!"
    exit 1
fi