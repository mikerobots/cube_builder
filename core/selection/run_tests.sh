#!/bin/bash

# Selection subsystem test runner with optimization
# Usage: ./run_tests.sh [build_dir] [mode]
# Modes: quick (default), full, performance, all

set -e

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Default values
BUILD_DIR="${1:-build_ninja}"
MODE="${2:-quick}"

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Timing function
time_command() {
    local start=$(date +%s.%N)
    "$@"
    local result=$?
    local end=$(date +%s.%N)
    local duration=$(echo "$end - $start" | bc)
    printf "${BLUE}Execution time: %.3f seconds${NC}\n" $duration
    return $result
}

# Display help
if [[ "$1" == "--help" || "$1" == "-h" ]]; then
    echo "Selection subsystem test runner"
    echo "Usage: ./run_tests.sh [build_dir] [mode]"
    echo ""
    echo "Modes:"
    echo "  quick       Run fast unit tests only (default)"
    echo "  full        Run all unit tests except performance"
    echo "  performance Run performance/stress tests only"
    echo "  all         Run all tests including performance"
    echo ""
    echo "Typical execution times:"
    echo "  quick:       < 0.1s (unit tests only)"
    echo "  full:        < 0.1s (all non-performance tests)"
    echo "  performance: ~45s (stress tests with large datasets)"
    echo "  all:         ~45s (everything)"
    echo ""
    echo "Note: Performance tests may exceed 5-second limit by design"
    exit 0
fi

echo -e "${GREEN}Running Selection subsystem tests...${NC}"
echo "Build directory: $BUILD_DIR"
echo "Mode: $MODE"
echo ""

# Start total timer
TOTAL_START=$(date +%s.%N)

# Change to project root
cd "$PROJECT_ROOT"

# Ensure build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory '$BUILD_DIR' does not exist${NC}"
    echo "Please run: cmake -B $BUILD_DIR -G Ninja"
    exit 1
fi

# Check if test executable exists
TEST_EXECUTABLE="$BUILD_DIR/bin/VoxelEditor_Selection_Tests"
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo -e "${YELLOW}Building Selection tests...${NC}"
    BUILD_START=$(date +%s.%N)
    cmake --build "$BUILD_DIR" --target VoxelEditor_Selection_Tests
    BUILD_END=$(date +%s.%N)
    BUILD_TIME=$(echo "$BUILD_END - $BUILD_START" | bc)
    printf "Build time: %.3f seconds\n\n" $BUILD_TIME
fi

# Define test filters based on mode
case "$MODE" in
    quick)
        FILTER="-*Performance*:-*LargeSelection*:-*VeryLargeRadius*:-*Stress*:-*HighResolution*"
        echo -e "${GREEN}Running QUICK mode - fast unit tests only${NC}"
        echo "- Excludes performance and stress tests"
        echo "- All tests complete in <5 seconds"
        TIMEOUT=60
        ;;
    full)
        FILTER="-*Performance*:-*LargeSelection*:-*VeryLargeRadius*:-*Stress*:-*HighResolution*"
        echo -e "${YELLOW}Running FULL mode - all unit tests${NC}"
        echo "- Excludes only performance tests"
        echo "- All tests complete in <5 seconds"
        TIMEOUT=120
        ;;
    performance)
        FILTER="*Performance*:*LargeSelection*:*VeryLargeRadius*:*Stress*:*HighResolution*"
        echo -e "${RED}Running PERFORMANCE mode - stress tests only${NC}"
        echo "- Tests with large datasets (may exceed 5s limit)"
        echo "- Validates performance characteristics"
        echo ""
        echo -e "${YELLOW}WARNING: Some tests intentionally exceed 5-second limit:${NC}"
        echo "  - LargeSelectionAddRemove: ~27s"
        echo "  - BoxSelectorHighResolution: ~5.4s"
        echo "  - UndoRedoLargeSelections: ~12s"
        TIMEOUT=300
        ;;
    all)
        FILTER="*"
        echo -e "${YELLOW}Running ALL tests including performance${NC}"
        echo "- Complete test coverage"
        echo "- Performance tests may take significant time"
        TIMEOUT=300
        ;;
    *)
        echo -e "${RED}Error: Unknown mode '$MODE'${NC}"
        echo "Valid modes: quick, full, performance, all"
        echo "Use --help for more information"
        exit 1
        ;;
esac

echo ""
echo "==========================================="

# Count tests
echo -e "${BLUE}Counting tests...${NC}"
TEST_COUNT=$("$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" --gtest_filter="$FILTER" --gtest_list_tests | grep -c "^  " || true)
echo "Total tests to run: $TEST_COUNT"

# Run tests with timing
echo ""
echo -e "${BLUE}Executing tests...${NC}"
TEST_START=$(date +%s.%N)

# Run with timeout and detailed timing
timeout $TIMEOUT "$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" \
    --gtest_filter="$FILTER" \
    --gtest_print_time=1 \
    --gtest_brief=0
TEST_RESULT=$?

TEST_END=$(date +%s.%N)
TEST_TIME=$(echo "$TEST_END - $TEST_START" | bc)

# Calculate total time
TOTAL_END=$(date +%s.%N)
TOTAL_TIME=$(echo "$TOTAL_END - $TOTAL_START" | bc)

echo ""
echo "==========================================="
echo -e "${BLUE}Timing Summary:${NC}"
printf "Test execution: %.3f seconds\n" $TEST_TIME
printf "Total time: %.3f seconds\n" $TOTAL_TIME

# Performance analysis
echo ""
if [[ "$MODE" == "performance" || "$MODE" == "all" ]]; then
    echo -e "${YELLOW}Performance test notes:${NC}"
    echo "- Some tests exceed 5s by design to stress the system"
    echo "- Focus on optimization opportunities in:"
    echo "  * Large selection operations (currently ~27s)"
    echo "  * High-resolution box selection (currently ~5.4s)"
    echo "  * Undo/redo with large datasets (currently ~12s)"
elif (( $(echo "$TEST_TIME < 0.5" | bc -l) )); then
    echo -e "${GREEN}✓ Excellent performance!${NC}"
elif (( $(echo "$TEST_TIME < 1.0" | bc -l) )); then
    echo -e "${GREEN}✓ Good performance${NC}"
elif (( $(echo "$TEST_TIME < 5.0" | bc -l) )); then
    echo -e "${YELLOW}⚠ Acceptable performance${NC}"
else
    echo -e "${RED}✗ Tests exceed 5-second target${NC}"
    echo "Consider optimizing slow tests or moving them to performance mode"
fi

# Exit with test result
if [ $TEST_RESULT -eq 0 ]; then
    echo ""
    echo -e "${GREEN}Selection tests completed successfully!${NC}"
    
    if [[ "$MODE" == "quick" || "$MODE" == "full" ]]; then
        echo ""
        echo "To run performance tests: ./run_tests.sh $BUILD_DIR performance"
    fi
else
    echo ""
    echo -e "${RED}Selection tests failed!${NC}"
    
    if [ $TEST_RESULT -eq 124 ]; then
        echo "Tests were terminated due to timeout ($TIMEOUT seconds)"
    fi
fi

exit $TEST_RESULT