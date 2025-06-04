#!/bin/bash
# Run All CLI Validation Tests
# Executes all test scripts and provides a summary report

set -e

TEST_DIR="$(dirname "$0")"
cd "$TEST_DIR"

echo "========================================"
echo "CLI Validation Test Suite"
echo "========================================"
echo ""

# List of test scripts
TESTS=(
    "test_basic_voxel_placement.sh"
    "test_camera_views.sh"
    "test_resolution_switching.sh"
    "test_multiple_voxels.sh"
    "test_render_modes.sh"
)

# Track results
PASSED=0
FAILED=0
FAILED_TESTS=()

# Clean up previous test output including all PPM files
echo "Cleaning up previous test results..."
rm -rf test_output
mkdir -p test_output

echo ""
echo "Running all tests..."
echo ""

# Run each test
for test in "${TESTS[@]}"; do
    if [ -f "$test" ]; then
        echo "----------------------------------------"
        echo "Running: $test"
        echo "----------------------------------------"
        
        if ./"$test"; then
            echo "✅ PASSED: $test"
            ((PASSED++))
        else
            echo "❌ FAILED: $test"
            ((FAILED++))
            FAILED_TESTS+=("$test")
        fi
        echo ""
    else
        echo "⚠️  WARNING: Test file $test not found"
        echo ""
    fi
done

# Summary report
echo "========================================"
echo "TEST SUMMARY"
echo "========================================"
echo "Total tests run: $((PASSED + FAILED))"
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo ""

if [ $FAILED -eq 0 ]; then
    echo "🎉 ALL TESTS PASSED!"
    echo ""
    echo "Test artifacts saved in: test_output/"
    echo "View screenshots and analysis files for detailed results."
    exit 0
else
    echo "💥 SOME TESTS FAILED:"
    for failed_test in "${FAILED_TESTS[@]}"; do
        echo "  - $failed_test"
    done
    echo ""
    echo "Check test_output/ for debugging information."
    echo "See individual test output above for failure details."
    exit 1
fi