#!/bin/bash
# Run all comprehensive CLI tests

set -e

TEST_DIR="$(dirname "$0")"
cd "$TEST_DIR"

echo "========================================"
echo "Comprehensive CLI Test Suite"
echo "========================================"
echo ""
echo "This will run both headless and rendering tests."
echo "Rendering tests require a display environment."
echo ""

# Initialize results
HEADLESS_RESULT=0
RENDERING_RESULT=0

# Ensure all test scripts are executable
chmod +x *.sh

# Create output directories
mkdir -p output/headless
mkdir -p output/rendering  
mkdir -p output/logs
mkdir -p output/metrics

# Clean up previous results
echo "Cleaning up previous test results..."
rm -rf output/*
mkdir -p output/headless output/rendering output/logs output/metrics

# Build the CLI if needed
if [ ! -f "../../voxel-cli" ]; then
    echo "Building voxel-cli..."
    cd ../..
    cmake --build build
    cd tests/cli_comprehensive
fi

# Check CLI exists
if [ ! -f "../../voxel-cli" ]; then
    echo "ERROR: voxel-cli not found. Please build the project first."
    exit 1
fi

# Check tools exist
if [ ! -f "../../tools/analyze_ppm_colors.py" ]; then
    echo "ERROR: PPM analysis tool not found at ../../tools/analyze_ppm_colors.py"
    exit 1
fi

echo ""
echo "========================================"
echo "PHASE 1: Headless Tests"
echo "========================================"

if ./run_headless_tests.sh; then
    echo "✅ Headless tests passed"
    HEADLESS_RESULT=0
else
    echo "❌ Headless tests failed"
    HEADLESS_RESULT=1
fi

echo ""
echo "========================================"
echo "PHASE 2: Rendering Tests"  
echo "========================================"

if ./run_rendering_tests.sh; then
    echo "✅ Rendering tests passed"
    RENDERING_RESULT=0
else
    echo "❌ Rendering tests failed"
    RENDERING_RESULT=1
fi

echo ""
echo "========================================"
echo "FINAL SUMMARY"
echo "========================================"

# Collect metrics
TOTAL_TESTS=$(find output/logs -name "*.log" | wc -l)
SCREENSHOTS=$(find output -name "*.ppm*" 2>/dev/null | wc -l)

echo "Test suites run: 2"
echo "Total test files executed: $TOTAL_TESTS"
echo "Screenshots generated: $SCREENSHOTS"
echo ""

if [ $HEADLESS_RESULT -eq 0 ]; then
    echo "✅ Headless tests: PASSED"
else
    echo "❌ Headless tests: FAILED"
fi

if [ $RENDERING_RESULT -eq 0 ]; then
    echo "✅ Rendering tests: PASSED"
else
    echo "❌ Rendering tests: FAILED"
fi

echo ""
echo "Test artifacts:"
echo "  - Logs: output/logs/"
echo "  - Screenshots: output/rendering/"
echo "  - Headless results: output/headless/"
echo "  - Performance metrics: output/metrics/"

# Generate summary report
echo ""
echo "Generating summary report..."
cat > output/test_summary.txt << EOF
Comprehensive CLI Test Summary
Generated: $(date)

Headless Tests: $([ $HEADLESS_RESULT -eq 0 ] && echo "PASSED" || echo "FAILED")
Rendering Tests: $([ $RENDERING_RESULT -eq 0 ] && echo "PASSED" || echo "FAILED")

Total test files: $TOTAL_TESTS
Screenshots generated: $SCREENSHOTS

Detailed logs available in output/logs/
EOF

echo "Summary saved to: output/test_summary.txt"

# Exit with appropriate code
if [ $HEADLESS_RESULT -eq 0 ] && [ $RENDERING_RESULT -eq 0 ]; then
    echo ""
    echo "🎉 ALL TESTS PASSED!"
    exit 0
else
    echo ""
    echo "💥 SOME TESTS FAILED"
    echo "See output/logs/ for details"
    exit 1
fi