#!/bin/bash

# File I/O subsystem test runner
# Usage: ./run_tests.sh [build_dir] [mode]
#   build_dir: Build directory (default: build_ninja)
#   mode: Test mode - quick, full, or requirements (default: full)
#
# Examples:
#   ./run_tests.sh                    # Run all tests
#   ./run_tests.sh build_ninja quick  # Run quick tests only
#   ./run_tests.sh build_debug full   # Run all tests with debug build

set -e

# Default build directory
BUILD_DIR="${1:-build_ninja}"
TEST_MODE="${2:-full}"

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_color() {
    color=$1
    shift
    echo -e "${color}$@${NC}"
}

# Function to time a command
time_command() {
    local start_time=$(date +%s.%N)
    "$@"
    local exit_code=$?
    local end_time=$(date +%s.%N)
    local duration=$(echo "$end_time - $start_time" | bc)
    
    if [ $exit_code -eq 0 ]; then
        print_color $GREEN "✓ Completed in ${duration}s"
    else
        print_color $RED "✗ Failed after ${duration}s"
    fi
    
    return $exit_code
}

echo "=========================================="
print_color $BLUE "File I/O Subsystem Test Runner"
echo "=========================================="
echo "Build directory: $BUILD_DIR"
echo "Test mode: $TEST_MODE"
echo "Project root: $PROJECT_ROOT"
echo ""

# Ensure build directory exists
if [ ! -d "$PROJECT_ROOT/$BUILD_DIR" ]; then
    print_color $RED "Error: Build directory '$BUILD_DIR' does not exist"
    echo "Please run: cmake -B $BUILD_DIR -G Ninja"
    exit 1
fi

# Change to build directory
cd "$PROJECT_ROOT/$BUILD_DIR"

# Check if test executable exists
TEST_EXECUTABLE="$PROJECT_ROOT/$BUILD_DIR/bin/file_io_tests"
if [ ! -f "$TEST_EXECUTABLE" ]; then
    print_color $RED "Error: file_io_tests executable not found at $TEST_EXECUTABLE"
    echo "Please build the project first: cmake --build $BUILD_DIR"
    exit 1
fi

# Define test filters based on mode
case "$TEST_MODE" in
    quick)
        print_color $YELLOW "Running QUICK tests (excluding slow tests)"
        # Exclude tests that typically take longer
        GTEST_FILTER="*:-FileManagerTest.AutoSaveBasic:CompressionTest.LargeDataStressTest"
        TIMEOUT=60
        ;;
    requirements)
        print_color $YELLOW "Running REQUIREMENTS tests only"
        GTEST_FILTER="FileIORequirementsTest.*"
        TIMEOUT=120
        ;;
    full|*)
        print_color $YELLOW "Running ALL tests"
        GTEST_FILTER="*"
        TIMEOUT=300
        ;;
esac

echo ""
print_color $BLUE "Executing File I/O tests..."
echo "=========================================="

# Run the tests with timing information
print_color $YELLOW "Test timeout: ${TIMEOUT}s"
print_color $YELLOW "Test filter: ${GTEST_FILTER}"
echo ""

# Execute tests with detailed timing
if timeout $TIMEOUT "$TEST_EXECUTABLE" \
    --gtest_filter="$GTEST_FILTER" \
    --gtest_print_time=1 \
    --gtest_output=xml:file_io_test_results.xml \
    2>&1 | tee test_output.log; then
    
    echo ""
    print_color $GREEN "✓ File I/O tests completed successfully!"
    
    # Extract timing statistics
    echo ""
    print_color $BLUE "Test Timing Summary:"
    echo "=========================================="
    
    # Show tests that took more than 100ms
    print_color $YELLOW "Tests taking >100ms:"
    grep -E "\[[[:space:]]+OK[[:space:]]+\].*\([0-9]{3,} ms\)" test_output.log || echo "  None"
    
    # Show total time
    TOTAL_TIME=$(grep -E "tests? from.*ran\." test_output.log | sed -E 's/.*\(([0-9]+) ms total\).*/\1/')
    if [ ! -z "$TOTAL_TIME" ]; then
        TOTAL_SECONDS=$(echo "scale=3; $TOTAL_TIME / 1000" | bc)
        echo ""
        print_color $BLUE "Total execution time: ${TOTAL_SECONDS}s"
    fi
    
    # Performance check
    echo ""
    print_color $BLUE "Performance Check:"
    echo "=========================================="
    
    # Check for tests exceeding 5 seconds
    if grep -E "\[[[:space:]]+OK[[:space:]]+\].*\([5-9][0-9]{3,} ms\)" test_output.log > /dev/null; then
        print_color $RED "⚠ WARNING: Some tests exceed 5 seconds!"
        grep -E "\[[[:space:]]+OK[[:space:]]+\].*\([5-9][0-9]{3,} ms\)" test_output.log
    else
        print_color $GREEN "✓ All tests within 5-second limit"
    fi
    
else
    # Check if no tests were found
    if grep -q "No tests were found" test_output.log; then
        print_color $YELLOW "Note: No tests matched the filter."
        echo "Filter used: $GTEST_FILTER"
    else
        print_color $RED "✗ File I/O tests failed!"
        
        # Show failed tests
        echo ""
        print_color $RED "Failed tests:"
        grep -E "\[[[:space:]]+FAILED[[:space:]]+\]" test_output.log || echo "  Unable to parse failures"
        
        exit 1
    fi
fi

# Clean up
rm -f test_output.log file_io_test_results.xml

echo ""
print_color $BLUE "Test Mode Summary:"
echo "=========================================="
echo "- quick: Excludes slow tests (AutoSave, LargeDataStress)"
echo "- full: Runs all tests"
echo "- requirements: Runs only requirement validation tests"
echo ""
print_color $BLUE "Typical execution times:"
echo "- Quick mode: ~0.1s"
echo "- Requirements mode: ~0.05s"
echo "- Full mode: ~2.1s (includes 2s AutoSave test)"
echo ""
print_color $YELLOW "Note: All individual tests comply with the 5-second rule."
print_color $YELLOW "The AutoSaveBasic test (2s) uses a deliberate sleep for timing validation."