#!/bin/bash

# Visual Feedback subsystem test runner with optimization
# Usage: ./run_tests.sh [build_dir] [mode]
# Modes: quick (default), full, requirements, all

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
    echo "Visual Feedback subsystem test runner"
    echo "Usage: ./run_tests.sh [build_dir] [mode]"
    echo ""
    echo "Modes:"
    echo "  quick       Run fast unit tests only (default)"
    echo "  full        Run all unit tests including rendering"
    echo "  requirements Run requirement validation tests only"
    echo "  all         Run all tests"
    echo ""
    echo "Typical execution times:"
    echo "  quick:       < 0.1s (basic functionality tests)"
    echo "  full:        < 0.5s (includes rendering tests)"
    echo "  requirements: < 0.2s (requirement validation only)"
    echo "  all:         < 0.5s (everything)"
    echo ""
    echo "Note: Some tests may crash due to OpenGL context issues in headless mode"
    exit 0
fi

echo -e "${GREEN}Running Visual Feedback subsystem tests...${NC}"
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
TEST_EXECUTABLE="$BUILD_DIR/bin/visual_feedback_tests"
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo -e "${YELLOW}Building Visual Feedback tests...${NC}"
    BUILD_START=$(date +%s.%N)
    cmake --build "$BUILD_DIR" --target visual_feedback_tests
    BUILD_END=$(date +%s.%N)
    BUILD_TIME=$(echo "$BUILD_END - $BUILD_START" | bc)
    printf "Build time: %.3f seconds\n\n" $BUILD_TIME
fi

# Define test filters based on mode
# Note: Some rendering tests may crash in headless environments
KNOWN_CRASHY_TESTS="OverlayRendererTest.*:FeedbackRendererTest.*:VisualFeedbackRequirementTest.*"

case "$MODE" in
    quick)
        # Exclude rendering tests and known crashy tests
        FILTER="-${KNOWN_CRASHY_TESTS}:-*Render*:-*Performance*:-*Stress*"
        echo -e "${GREEN}Running QUICK mode - fast unit tests only${NC}"
        echo "- Excludes rendering and potentially crashy tests"
        echo "- All tests complete in <5 seconds"
        TIMEOUT=60
        ;;
    full)
        # Try to run all tests but warn about potential crashes
        FILTER="-*Performance*:-*Stress*"
        echo -e "${YELLOW}Running FULL mode - all unit tests${NC}"
        echo "- Includes rendering tests (may crash in headless mode)"
        echo "- All tests should complete in <5 seconds"
        TIMEOUT=120
        ;;
    requirements)
        # Run only requirement tests
        FILTER="*Requirement*"
        echo -e "${BLUE}Running REQUIREMENTS mode - requirement tests only${NC}"
        echo "- Tests requirement coverage"
        echo "- May crash if tests require OpenGL context"
        TIMEOUT=60
        ;;
    all)
        FILTER="*"
        echo -e "${YELLOW}Running ALL tests${NC}"
        echo "- Complete test coverage"
        echo "- May crash in headless environments"
        TIMEOUT=300
        ;;
    *)
        echo -e "${RED}Error: Unknown mode '$MODE'${NC}"
        echo "Valid modes: quick, full, requirements, all"
        echo "Use --help for more information"
        exit 1
        ;;
esac

echo ""
echo "==========================================="

# Count tests
echo -e "${BLUE}Counting tests...${NC}"
TEST_COUNT=$("$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" --gtest_filter="$FILTER" --gtest_list_tests 2>/dev/null | grep -c "^  " || echo "0")
echo "Total tests to run: $TEST_COUNT"

if [ "$TEST_COUNT" -eq "0" ]; then
    echo -e "${YELLOW}Warning: No tests match the filter${NC}"
fi

# Run tests with timing
echo ""
echo -e "${BLUE}Executing tests...${NC}"
TEST_START=$(date +%s.%N)

# Run with timeout and detailed timing
# Capture both stdout and stderr, and handle potential crashes
set +e
timeout $TIMEOUT "$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" \
    --gtest_filter="$FILTER" \
    --gtest_print_time=1 \
    --gtest_brief=0 2>&1 | tee /tmp/visual_feedback_test_output.log
TEST_RESULT=${PIPESTATUS[0]}
set -e

# Check for segmentation fault
if grep -q "Segmentation fault" /tmp/visual_feedback_test_output.log 2>/dev/null; then
    echo ""
    echo -e "${RED}⚠️  WARNING: Some tests crashed (segmentation fault)${NC}"
    echo "This often happens when running OpenGL/rendering tests without a display"
    echo "Consider running in an environment with OpenGL support or using 'quick' mode"
    
    # Try to identify which test crashed
    LAST_TEST=$(grep "^\[ RUN" /tmp/visual_feedback_test_output.log | tail -1 | sed 's/\[ RUN      \] //')
    if [ -n "$LAST_TEST" ]; then
        echo -e "${YELLOW}Last test before crash: $LAST_TEST${NC}"
    fi
fi

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
if [ "$TEST_RESULT" -eq 124 ]; then
    echo -e "${RED}✗ Tests timed out after $TIMEOUT seconds${NC}"
elif [ "$TEST_RESULT" -ne 0 ]; then
    echo -e "${RED}✗ Tests failed or crashed${NC}"
elif (( $(echo "$TEST_TIME < 0.5" | bc -l) )); then
    echo -e "${GREEN}✓ Excellent performance!${NC}"
elif (( $(echo "$TEST_TIME < 1.0" | bc -l) )); then
    echo -e "${GREEN}✓ Good performance${NC}"
elif (( $(echo "$TEST_TIME < 5.0" | bc -l) )); then
    echo -e "${YELLOW}⚠ Acceptable performance${NC}"
else
    echo -e "${RED}✗ Tests exceed 5-second target${NC}"
    echo "Consider optimizing slow tests"
fi

# Provide mode-specific advice
echo ""
if [[ "$MODE" == "quick" ]]; then
    echo -e "${BLUE}Quick mode completed${NC}"
    echo "To test rendering functionality: ./run_tests.sh $BUILD_DIR full"
    echo "Note: Rendering tests may require OpenGL context"
elif [[ "$MODE" == "full" || "$MODE" == "all" ]] && [ "$TEST_RESULT" -ne 0 ]; then
    echo -e "${YELLOW}Tip: If tests are crashing, try 'quick' mode:${NC}"
    echo "./run_tests.sh $BUILD_DIR quick"
fi

# Clean up
rm -f /tmp/visual_feedback_test_output.log

# Exit with appropriate code
if [ "$TEST_RESULT" -eq 0 ]; then
    echo ""
    echo -e "${GREEN}Visual Feedback tests completed successfully!${NC}"
else
    echo ""
    echo -e "${RED}Visual Feedback tests failed or crashed!${NC}"
fi

exit $TEST_RESULT