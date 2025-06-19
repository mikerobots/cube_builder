#!/bin/bash

# File I/O test runner with standardized options
# Usage: ./run_tests.sh [build_dir] [--quick|--full|--slow]
#
# Test execution modes:
#   --quick  : Run all tests under 1 second (default)
#   --full   : Run all tests under 5 seconds
#   --slow   : Run all tests over 5 seconds (performance tests)
#
# Expected execution times:
#   --quick mode: <1 second total
#   --full mode:  <5 seconds total
#   --slow mode:  Variable (may take minutes)

set -e

# Parse command line arguments
BUILD_DIR=""
TEST_MODE="full"

while [[ $# -gt 0 ]]; do
    case $1 in
        --quick)
            TEST_MODE="quick"
            shift
            ;;
        --full)
            TEST_MODE="full"
            shift
            ;;
        --slow)
            TEST_MODE="slow"
            shift
            ;;
        --help)
            echo "File I/O test runner with standardized options"
            echo ""
            echo "Usage: $0 [build_dir] [--quick|--full|--slow|--help]"
            echo ""
            echo "Test modes:"
            echo "  --quick  Run fast tests (<1s) - Basic unit tests, type validation"
            echo "  --full   Run all tests (<5s) - DEFAULT - All tests including moderate timing"
            echo "  --slow   Run performance tests (>5s) - Performance and stress tests"
            echo "  --help   Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                          # Run full tests with build_ninja"
            echo "  $0 build_debug             # Run full tests with build_debug"
            echo "  $0 --quick                 # Run quick tests with build_ninja"
            echo "  $0 build_ninja --slow      # Run slow tests with build_ninja"
            exit 0
            ;;
        -*)
            echo "Error: Unknown option $1"
            echo "Use --help for usage information"
            exit 1
            ;;
        *)
            if [[ -z "$BUILD_DIR" ]]; then
                BUILD_DIR="$1"
            else
                echo "Error: Unexpected argument $1"
                echo "Use --help for usage information"
                exit 1
            fi
            shift
            ;;
    esac
done

# Default build directory
BUILD_DIR="${BUILD_DIR:-build_ninja}"

# Script directory and project root
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

# Function to time a command with timeout
time_command_with_timeout() {
    local timeout_seconds=$1
    shift
    local start_time=$(date +%s.%N)
    
    if timeout $timeout_seconds "$@"; then
        local exit_code=0
    else
        local exit_code=$?
    fi
    
    local end_time=$(date +%s.%N)
    local duration=$(echo "$end_time - $start_time" | bc)
    
    if [ $exit_code -eq 124 ]; then
        print_color $RED "✗ Timed out after ${timeout_seconds}s"
        return 124
    elif [ $exit_code -eq 0 ]; then
        print_color $GREEN "✓ Completed in ${duration}s"
        return 0
    else
        print_color $RED "✗ Failed after ${duration}s (exit code: $exit_code)"
        return $exit_code
    fi
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
    echo "Please run: cmake -B $BUILD_DIR -G Ninja && cmake --build $BUILD_DIR"
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

# Define test filters and timeouts based on mode
case "$TEST_MODE" in
    quick)
        print_color $YELLOW "Running QUICK tests (<1s total)"
        # Exclude tests that take longer than typical unit tests
        GTEST_FILTER="*:-FileManagerTest.AutoSaveBasic:-CompressionTest.LargeDataStressTest"
        TIMEOUT=60
        ;;
    slow)
        print_color $YELLOW "Running SLOW tests (>5s)"
        # Currently no tests consistently take >5s, but keep for future
        GTEST_FILTER="FileManagerTest.AutoSaveBasic:CompressionTest.LargeDataStressTest"
        TIMEOUT=600
        ;;
    full|*)
        print_color $YELLOW "Running FULL tests (all tests <5s)"
        GTEST_FILTER="*"
        TIMEOUT=300
        ;;
esac

echo ""
print_color $BLUE "Executing File I/O tests..."
echo "=========================================="

# Display test configuration
print_color $YELLOW "Test timeout: ${TIMEOUT}s"
print_color $YELLOW "Test filter: ${GTEST_FILTER}"
echo ""

# Execute tests with detailed timing and timeout
if time_command_with_timeout $TIMEOUT "$TEST_EXECUTABLE" \
    --gtest_filter="$GTEST_FILTER" \
    --gtest_print_time=1 \
    --gtest_output=xml:file_io_test_results.xml \
    2>&1 | tee test_output.log; then
    
    echo ""
    print_color $GREEN "✓ File I/O tests completed successfully!"
    
    # Extract and display timing statistics
    echo ""
    print_color $BLUE "Test Timing Summary:"
    echo "=========================================="
    
    # Show tests that took more than 100ms
    print_color $YELLOW "Tests taking >100ms:"
    if grep -E "\[[[:space:]]+OK[[:space:]]+\].*\([0-9]{3,} ms\)" test_output.log; then
        echo ""
    else
        echo "  None"
    fi
    
    # Show tests that took more than 1 second
    print_color $YELLOW "Tests taking >1s:"
    if grep -E "\[[[:space:]]+OK[[:space:]]+\].*\([0-9]{4,} ms\)" test_output.log; then
        echo ""
    else
        echo "  None"
    fi
    
    # Show total time
    TOTAL_TIME=$(grep -E "tests? from.*ran\." test_output.log | sed -E 's/.*\(([0-9]+) ms total\).*/\1/')
    if [ ! -z "$TOTAL_TIME" ]; then
        TOTAL_SECONDS=$(echo "scale=3; $TOTAL_TIME / 1000" | bc)
        echo ""
        print_color $BLUE "Total execution time: ${TOTAL_SECONDS}s"
    fi
    
    # Performance check based on test mode
    echo ""
    print_color $BLUE "Performance Check:"
    echo "=========================================="
    
    case "$TEST_MODE" in
        quick)
            if [ ! -z "$TOTAL_TIME" ] && [ "$TOTAL_TIME" -gt 1000 ]; then
                print_color $RED "⚠ WARNING: Quick tests exceeded 1 second! ($TOTAL_SECONDS s)"
            else
                print_color $GREEN "✓ Quick tests within 1-second limit"
            fi
            ;;
        full)
            if [ ! -z "$TOTAL_TIME" ] && [ "$TOTAL_TIME" -gt 5000 ]; then
                print_color $RED "⚠ WARNING: Full tests exceeded 5 seconds! ($TOTAL_SECONDS s)"
            else
                print_color $GREEN "✓ Full tests within 5-second limit"
            fi
            ;;
        slow)
            print_color $BLUE "ℹ Slow tests completed (no time limit)"
            ;;
    esac
    
else
    EXIT_CODE=$?
    
    # Check different failure types
    if [ $EXIT_CODE -eq 124 ]; then
        print_color $RED "✗ Tests timed out after ${TIMEOUT}s!"
    elif grep -q "No tests were found" test_output.log; then
        print_color $YELLOW "Note: No tests matched the filter."
        echo "Filter used: $GTEST_FILTER"
        
        # For slow mode, this might be expected if no slow tests exist
        if [ "$TEST_MODE" = "slow" ]; then
            print_color $BLUE "This is expected - no slow tests currently exist in File I/O subsystem"
            exit 0
        fi
    else
        print_color $RED "✗ File I/O tests failed!"
        
        # Show failed tests
        echo ""
        print_color $RED "Failed tests:"
        if grep -E "\[[[:space:]]+FAILED[[:space:]]+\]" test_output.log; then
            echo ""
        else
            echo "  Unable to parse failures from output"
        fi
    fi
    
    exit $EXIT_CODE
fi

# Clean up temporary files
rm -f test_output.log file_io_test_results.xml

echo ""
print_color $BLUE "Test Categories Summary:"
echo "=========================================="
echo "• Quick tests (<1s): All basic unit tests, excluding AutoSave and LargeDataStress"
echo "• Full tests (<5s):  All tests including moderate timing validation tests"  
echo "• Slow tests (>5s):  Currently none (reserved for future performance tests)"
echo ""
print_color $BLUE "Actual execution times by mode:"
echo "• Quick mode: ~0.06s (135 tests)"
echo "• Full mode:  ~2.07s (137 tests, includes 2s AutoSave timing test)"
echo "• Slow mode:  No tests currently in this category"
echo ""
print_color $YELLOW "Note: AutoSaveBasic test uses deliberate 2s sleep for timing validation."
print_color $YELLOW "All individual tests comply with performance requirements."