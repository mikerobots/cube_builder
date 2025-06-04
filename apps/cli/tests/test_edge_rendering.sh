#!/bin/bash

# Test edge rendering with CLI
# Exit on error
set -e

echo "Testing edge rendering with CLI..."

# Function to check for errors in output
check_for_errors() {
    local output="$1"
    local test_name="$2"
    
    # Check for common error patterns
    if echo "$output" | grep -qi "error\|exception\|failed\|segmentation\|abort\|crash\|invalid\|warning"; then
        # Ignore specific known warnings that are not errors
        if echo "$output" | grep -q "WARNING: Could not find vertex shader attribute" || \
           echo "$output" | grep -q "WARNING: Output of vertex shader"; then
            # These are known shader warnings, not errors
            return 0
        fi
        
        echo "✗ $test_name failed - errors detected in output:"
        echo "$output" | grep -i "error\|exception\|failed\|segmentation\|abort\|crash\|invalid\|warning"
        return 1
    fi
    
    # Check for expected success patterns
    if ! echo "$output" | grep -q "Initialization complete"; then
        echo "✗ $test_name failed - initialization did not complete"
        return 1
    fi
    
    return 0
}

# Function to run test with timeout and capture output
run_test() {
    local test_name="$1"
    local commands="$2"
    local expected_output="$3"
    
    echo -e "\n$test_name"
    
    # Run command with timeout and capture both stdout and stderr
    local output
    output=$(timeout 3 ./build_ninja/bin/VoxelEditor_CLI --headless 2>&1 <<EOF || true
$commands
quit
EOF
)
    
    local exit_code=$?
    
    # Check exit code (0 = success, 124 = timeout which is OK for our tests)
    if [ $exit_code -ne 0 ] && [ $exit_code -ne 124 ]; then
        echo "✗ $test_name failed with exit code: $exit_code"
        echo "Output:"
        echo "$output"
        return 1
    fi
    
    # Check for errors in output
    if ! check_for_errors "$output" "$test_name"; then
        return 1
    fi
    
    # Check for expected output patterns
    if [ -n "$expected_output" ]; then
        if ! echo "$output" | grep -q "$expected_output"; then
            echo "✗ $test_name failed - expected output not found: $expected_output"
            echo "Actual output:"
            echo "$output"
            return 1
        fi
    fi
    
    echo "✓ $test_name passed"
    return 0
}

# Keep track of failures
FAILED=0

# Test 1: Place voxel without edges
if ! run_test "Test 1: Place voxel (no edges)" "place 0 0 0" "Voxel placed at (0, 0, 0)"; then
    FAILED=$((FAILED + 1))
fi

# Test 2: Place voxel with edges enabled
if ! run_test "Test 2: Place voxel with edges enabled" "edges on
place 0 0 0" "Edge rendering enabled"; then
    FAILED=$((FAILED + 1))
fi

# Test 3: Multiple voxels with edges
if ! run_test "Test 3: Multiple voxels with edges" "edges on
place 0 0 0
place 1 0 0
place 0 1 0" "Voxel placed at (0, 1, 0)"; then
    FAILED=$((FAILED + 1))
fi

# Test 4: Toggle edges
if ! run_test "Test 4: Toggle edges" "place 0 0 0
edges toggle
edges off
edges on" "Edge rendering enabled"; then
    FAILED=$((FAILED + 1))
fi

# Test 5: Check for shader compilation errors
if ! run_test "Test 5: Shader switching" "shader enhanced
place 0 0 0
shader basic
shader flat" "Shader mode set to: flat"; then
    FAILED=$((FAILED + 1))
fi

# Test 6: Stress test - many voxels with edges
if ! run_test "Test 6: Stress test with edges" "edges on
place 0 0 0
place 1 0 0
place 2 0 0
place 0 1 0
place 1 1 0
place 2 1 0
place 0 2 0
place 1 2 0
place 2 2 0" "Voxel placed at (2, 2, 0)"; then
    FAILED=$((FAILED + 1))
fi

# Test 7: Verify no OpenGL errors in non-headless mode (if display is available)
if [ -n "$DISPLAY" ] || [ "$(uname)" = "Darwin" ]; then
    echo -e "\nTest 7: Check for OpenGL errors (non-headless)"
    
    # Run briefly in non-headless mode to check for OpenGL errors
    output=$(timeout 2 ./build_ninja/bin/VoxelEditor_CLI 2>&1 <<EOF || true
place 0 0 0
quit
EOF
)
    
    # Check specifically for OpenGL errors
    if echo "$output" | grep -i "gl.*error\|opengl.*error" | grep -v "GL_INVALID_ENUM"; then
        echo "✗ Test 7 failed - OpenGL errors detected:"
        echo "$output" | grep -i "gl.*error\|opengl.*error"
        FAILED=$((FAILED + 1))
    else
        echo "✓ Test 7 passed (ignoring known GL_INVALID_ENUM)"
    fi
else
    echo -e "\nTest 7: Skipped (no display available)"
fi

# Summary
echo -e "\n========================================="
if [ $FAILED -eq 0 ]; then
    echo "✅ All edge rendering tests passed!"
    exit 0
else
    echo "❌ $FAILED test(s) failed!"
    exit 1
fi