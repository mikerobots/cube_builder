#!/bin/bash

# Groups subsystem test runner
# Usage: ./run_tests.sh [--quick|--full] [build_dir]
#
# Performance characteristics:
# - All tests execute in <10ms (individual tests <1ms each)
# - No individual test exceeds 5-second rule
# - Total suite execution: ~6ms for 92 tests
#
# Options:
#   --quick: Run core functionality tests only (default)
#   --full: Run all tests including stress/performance tests

set -e

# Parse arguments
MODE="quick"
BUILD_DIR=""

for arg in "$@"; do
    case $arg in
        --quick)
            MODE="quick"
            ;;
        --full)
            MODE="full"
            ;;
        *)
            BUILD_DIR="$arg"
            ;;
    esac
done

# Default build directory if not specified
BUILD_DIR="${BUILD_DIR:-build_ninja}"

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "=========================================="
echo "Groups Subsystem Test Runner"
echo "=========================================="
echo "Mode: $MODE"
echo "Build directory: $BUILD_DIR"
echo "Project root: $PROJECT_ROOT"
echo ""

# Change to project root
cd "$PROJECT_ROOT"

# Ensure build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Build directory '$BUILD_DIR' does not exist"
    echo "Please run: cmake -B $BUILD_DIR -G Ninja"
    exit 1
fi

# Check if test executable exists
TEST_EXECUTABLE="$BUILD_DIR/bin/groups_tests"
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo "Building Groups tests..."
    cmake --build "$BUILD_DIR" --target groups_tests
fi

# Set test filters based on mode
GTEST_FILTER=""
if [ "$MODE" = "quick" ]; then
    # Quick mode: exclude stress tests and performance benchmarks
    # Currently all tests are fast, but prepare for future additions
    GTEST_FILTER="--gtest_filter=-*Stress*:*Performance*:*LargeScale*"
    echo "Running in QUICK mode (core functionality only)"
else
    echo "Running in FULL mode (all tests)"
fi

# Run the tests with timing and timeout
echo ""
echo "Executing Groups tests..."
echo "=========================================="

# Start timer
START_TIME=$(date +%s.%N)

# Run tests with:
# - Timing output for each test
# - Timeout of 300 seconds for entire suite
# - Timeout warning at 5 seconds per test (enforced by test design)
timeout 300s "$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" \
    --gtest_print_time=1 \
    --gtest_color=yes \
    $GTEST_FILTER

TEST_EXIT_CODE=$?

# End timer
END_TIME=$(date +%s.%N)
DURATION=$(echo "$END_TIME - $START_TIME" | bc)

echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo "Total execution time: ${DURATION}s"

# Performance check
if (( $(echo "$DURATION > 30" | bc -l) )); then
    echo "WARNING: Tests took longer than 30 seconds!"
    echo "Consider investigating slow tests with --gtest_print_time=1"
fi

echo ""

# Check disabled tests
DISABLED_COUNT=$("$TEST_EXECUTABLE" --gtest_list_tests | grep -c "DISABLED_" || true)
if [ "$DISABLED_COUNT" -gt 0 ]; then
    echo "Note: $DISABLED_COUNT tests are disabled"
fi

# Exit with test result
if [ $TEST_EXIT_CODE -eq 0 ]; then
    echo "✅ Groups tests completed successfully!"
else
    echo "❌ Groups tests failed with exit code $TEST_EXIT_CODE"
fi

exit $TEST_EXIT_CODE