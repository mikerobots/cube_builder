#!/bin/bash
# Run all rendering tests

set -e

TEST_DIR="$(dirname "$0")"
cd "$TEST_DIR"

echo "========================================"
echo "Running Rendering CLI Tests"
echo "========================================"
echo ""

# Check if we can run rendering tests
if [ -n "$DISPLAY" ] || [ -n "$WAYLAND_DISPLAY" ] || [ "$(uname)" == "Darwin" ]; then
    echo "Display environment detected, proceeding with rendering tests..."
else
    echo "WARNING: No display environment detected. Rendering tests may fail."
    echo "Set DISPLAY or run with a graphical environment."
    read -p "Continue anyway? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Ensure output directories exist
mkdir -p output/rendering
mkdir -p output/logs
mkdir -p output/metrics

# List of rendering test scripts
RENDERING_TESTS=(
    "test_new_features.sh"
    "test_visual_enhancements.sh"
    "test_integration.sh"
)

# Track overall results
TOTAL_PASSED=0
TOTAL_FAILED=0
FAILED_TESTS=()

# Clean up any leftover PPM files
echo "Cleaning up previous screenshots..."
find output/rendering -name "*.ppm*" -delete 2>/dev/null || true

# Run each rendering test
for test in "${RENDERING_TESTS[@]}"; do
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

# Count screenshots generated
SCREENSHOT_COUNT=$(find output/rendering -name "*.ppm*" | wc -l)

# Overall summary
echo "========================================"
echo "RENDERING TEST SUMMARY"
echo "========================================"
echo "Total tests passed: $TOTAL_PASSED"
echo "Total tests failed: $TOTAL_FAILED"
echo "Screenshots generated: $SCREENSHOT_COUNT"
echo ""

if [ ${#FAILED_TESTS[@]} -eq 0 ] && [ $TOTAL_FAILED -eq 0 ]; then
    echo "🎉 ALL RENDERING TESTS PASSED!"
    echo ""
    echo "Screenshots saved in: output/rendering/"
    echo "Logs saved in: output/logs/"
    exit 0
else
    echo "💥 SOME TESTS FAILED:"
    for failed_test in "${FAILED_TESTS[@]}"; do
        echo "  - $failed_test"
    done
    echo ""
    echo "Check output/logs/ for detailed test output."
    echo "Screenshots (if any) saved in: output/rendering/"
    exit 1
fi