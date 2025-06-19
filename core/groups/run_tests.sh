#!/bin/bash

# Groups subsystem test runner with standardized options
# Usage: ./run_tests.sh [build_dir] [--quick|--full|--slow]
#
# Test execution modes:
#   --quick  : Run all tests under 1 second (default)
#   --full   : Run all tests under 5 seconds
#   --slow   : Run all tests over 5 seconds (performance tests)
#
# Expected execution times:
#   --quick mode: <1 second total (86 tests, 0-1ms each)
#   --full mode:  <5 seconds total (includes skipped tests)
#   --slow mode:  Variable (currently no slow tests)

set -e

# Parse arguments - standardized format [build_dir] [--quick|--full|--slow]
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
        --slow)
            MODE="slow"
            ;;
        --help)
            echo "Groups subsystem test runner"
            echo "Usage: ./run_tests.sh [build_dir] [--quick|--full|--slow]"
            echo ""
            echo "Test execution modes:"
            echo "  --quick  : Run all tests under 1 second (default)"
            echo "  --full   : Run all tests under 5 seconds"
            echo "  --slow   : Run all tests over 5 seconds (performance tests)"
            echo ""
            echo "Expected execution times:"
            echo "  --quick mode: <1 second total (86 tests, 0-1ms each)"
            echo "  --full mode:  <5 seconds total (includes skipped tests)"
            echo "  --slow mode:  Variable (currently no slow tests)"
            exit 0
            ;;
        --*)
            echo "Error: Unknown option $arg"
            echo "Use --help for usage information"
            exit 1
            ;;
        *)
            if [[ -z "$BUILD_DIR" ]]; then
                BUILD_DIR="$arg"
            else
                echo "Error: Unexpected argument $arg"
                echo "Use --help for usage information"
                exit 1
            fi
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

# Set test filters and timeouts based on mode
GTEST_FILTER=""
TIMEOUT_SECONDS=""

if [ "$MODE" = "quick" ]; then
    # Quick mode: exclude stress tests and performance benchmarks
    # Currently all tests are fast (<1ms each), but prepare for future additions
    GTEST_FILTER="--gtest_filter=-*Stress*:*Performance*:*LargeScale*:*Slow*"
    TIMEOUT_SECONDS="60"
    echo "Running in QUICK mode (tests under 1 second)"
elif [ "$MODE" = "full" ]; then
    # Full mode: run all tests except the slow ones
    GTEST_FILTER="--gtest_filter=-*Slow*"
    TIMEOUT_SECONDS="300"
    echo "Running in FULL mode (all tests under 5 seconds)"
elif [ "$MODE" = "slow" ]; then
    # Slow mode: run only the slow tests (currently none exist)
    GTEST_FILTER="--gtest_filter=*Slow*"
    TIMEOUT_SECONDS=""  # No timeout for slow tests
    echo "Running in SLOW mode (performance tests over 5 seconds)"
    echo "Note: No slow tests currently exist in Groups subsystem"
fi

# Run the tests with timing and timeout
echo ""
echo "Executing Groups tests..."
echo "=========================================="

# Start timer
START_TIME=$(date +%s.%N)

# Run tests with standardized timeout and options
if [ -n "$TIMEOUT_SECONDS" ]; then
    # Run with timeout for quick/full modes
    timeout "${TIMEOUT_SECONDS}s" "$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" \
        --gtest_print_time=1 \
        --gtest_color=yes \
        $GTEST_FILTER
else
    # Run without timeout for slow mode
    "$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" \
        --gtest_print_time=1 \
        --gtest_color=yes \
        $GTEST_FILTER
fi

TEST_EXIT_CODE=$?

# End timer
END_TIME=$(date +%s.%N)
DURATION=$(echo "$END_TIME - $START_TIME" | bc)

echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo "Total execution time: ${DURATION}s"

# Enhanced error handling and timeout detection
if [ $TEST_EXIT_CODE -eq 124 ]; then
    echo "❌ TIMEOUT: Tests exceeded ${TIMEOUT_SECONDS} second limit"
    echo "Consider running with --full or --slow mode for longer timeouts"
    exit $TEST_EXIT_CODE
elif [ $TEST_EXIT_CODE -ne 0 ] && [ $TEST_EXIT_CODE -ne 1 ]; then
    echo "❌ CRITICAL ERROR: Test execution failed with exit code $TEST_EXIT_CODE"
    echo "This may indicate missing dependencies or build issues"
    exit $TEST_EXIT_CODE
fi

# Performance validation based on mode
if [ "$MODE" = "quick" ] && (( $(echo "$DURATION > 1" | bc -l) )); then
    echo "⚠️  WARNING: Quick tests took longer than 1 second (${DURATION}s)"
    echo "Some tests may need to be categorized as full/slow tests"
elif [ "$MODE" = "full" ] && (( $(echo "$DURATION > 5" | bc -l) )); then
    echo "⚠️  WARNING: Full tests took longer than 5 seconds (${DURATION}s)"
    echo "Some tests may need to be categorized as slow tests"
fi

echo ""

# Check for skipped and disabled tests
SKIPPED_COUNT=$("$TEST_EXECUTABLE" --gtest_list_tests 2>/dev/null | grep -c "SKIPPED" || true)
DISABLED_COUNT=$("$TEST_EXECUTABLE" --gtest_list_tests 2>/dev/null | grep -c "DISABLED_" || true)

if [ "$SKIPPED_COUNT" -gt 0 ]; then
    echo "Note: $SKIPPED_COUNT tests are skipped"
fi
if [ "$DISABLED_COUNT" -gt 0 ]; then
    echo "Note: $DISABLED_COUNT tests are disabled"
fi

# Final result with mode-specific validation
if [ $TEST_EXIT_CODE -eq 0 ]; then
    echo "✅ Groups tests ($MODE mode) completed successfully!"
    echo "   Duration: ${DURATION}s"
else
    echo "❌ Groups tests ($MODE mode) failed with exit code $TEST_EXIT_CODE"
    echo "   Duration: ${DURATION}s"
fi

exit $TEST_EXIT_CODE