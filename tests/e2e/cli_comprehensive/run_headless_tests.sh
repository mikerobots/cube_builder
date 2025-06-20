#!/bin/bash
# Run all headless tests

set -e

TEST_DIR="$(dirname "$0")"
cd "$TEST_DIR"

echo "========================================"
echo "Running Headless CLI Tests"
echo "========================================"
echo ""

# Ensure output directories exist
mkdir -p output/headless
mkdir -p output/logs
mkdir -p output/metrics

# List of headless test scripts
HEADLESS_TESTS=(
    "test_commands_headless.sh"
    "test_error_handling.sh"
    "test_enhancement_validation.sh"
)

# Track overall results
TOTAL_PASSED=0
TOTAL_FAILED=0
FAILED_TESTS=()

# Run each headless test
for test in "${HEADLESS_TESTS[@]}"; do
    if [ -f "$test" ]; then
        echo "----------------------------------------"
        echo "Running: $test"
        echo "----------------------------------------"
        
        # Run test and capture output
        if ./"$test" > "output/logs/${test%.sh}.log" 2>&1; then
            echo "✅ PASSED: $test"
            # Extract pass/fail counts from test output
            PASSED=$(grep -oE 'Passed: [0-9]+' "output/logs/${test%.sh}.log" | grep -oE '[0-9]+' || echo 0)
            FAILED=$(grep -oE 'Failed: [0-9]+' "output/logs/${test%.sh}.log" | grep -oE '[0-9]+' || echo 0)
            TOTAL_PASSED=$((TOTAL_PASSED + PASSED))
            TOTAL_FAILED=$((TOTAL_FAILED + FAILED))
        else
            echo "❌ FAILED: $test"
            FAILED_TESTS+=("$test")
            # Try to extract counts even from failed test
            PASSED=$(grep -oE 'Passed: [0-9]+' "output/logs/${test%.sh}.log" | grep -oE '[0-9]+' || echo 0)
            FAILED=$(grep -oE 'Failed: [0-9]+' "output/logs/${test%.sh}.log" | grep -oE '[0-9]+' || echo 0)
            TOTAL_PASSED=$((TOTAL_PASSED + PASSED))
            TOTAL_FAILED=$((TOTAL_FAILED + FAILED + 1))
        fi
        
        # Show test summary
        tail -n 20 "output/logs/${test%.sh}.log" | grep -E "PASS|FAIL|Total|Summary" || true
        echo ""
    else
        echo "⚠️  WARNING: Test file $test not found"
        echo ""
    fi
done

# Overall summary
echo "========================================"
echo "HEADLESS TEST SUMMARY"
echo "========================================"
echo "Total tests passed: $TOTAL_PASSED"
echo "Total tests failed: $TOTAL_FAILED"
echo ""

if [ ${#FAILED_TESTS[@]} -eq 0 ] && [ $TOTAL_FAILED -eq 0 ]; then
    echo "🎉 ALL HEADLESS TESTS PASSED!"
    exit 0
else
    echo "💥 SOME TESTS FAILED:"
    for failed_test in "${FAILED_TESTS[@]}"; do
        echo "  - $failed_test"
    done
    echo ""
    echo "Check output/logs/ for detailed test output."
    exit 1
fi