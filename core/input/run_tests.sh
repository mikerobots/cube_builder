#!/bin/bash

# Input System test runner with standardized options
# Usage: ./run_tests.sh [build_dir] [--quick|--full|--slow]
#
# Test execution modes:
#   --quick  : Run all tests under 1 second (default)
#   --full   : Run all tests under 5 seconds
#   --slow   : Run all tests over 5 seconds (performance tests)
#
# Expected execution times:
#   --quick mode: <1 second total (147 tests, excludes PlaneDetector)
#   --full mode:  <5 seconds total (158 tests, includes safe PlaneDetector tests)
#   --slow mode:  Variable (disabled tests and stress tests)

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default build directory
BUILD_DIR="${1:-build_ninja}"

# Test mode (default to quick)
TEST_MODE="quick"
if [[ "$2" == "--full" ]]; then
    TEST_MODE="full"
elif [[ "$2" == "--slow" ]]; then
    TEST_MODE="slow"
elif [[ "$2" == "--quick" ]]; then
    TEST_MODE="quick"
fi

# Check if first argument is actually a mode flag
if [[ "$1" == "--quick" || "$1" == "--full" || "$1" == "--slow" || "$1" == "--help" ]]; then
    BUILD_DIR="build_ninja"
    TEST_MODE="${1#--}"
fi

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo -e "${BLUE}Running Input System tests...${NC}"
echo "Build directory: $BUILD_DIR"
echo "Project root: $PROJECT_ROOT"
echo "Test mode: $TEST_MODE"
echo ""

# Change to project root
cd "$PROJECT_ROOT"

# Ensure build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory '$BUILD_DIR' does not exist${NC}"
    echo "Please run: cmake -B $BUILD_DIR -G Ninja"
    exit 1
fi

# Check if test executable exists
TEST_EXECUTABLE="$BUILD_DIR/bin/VoxelEditor_Input_Tests"
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo "Building Input tests..."
    cmake --build "$BUILD_DIR" --target VoxelEditor_Input_Tests
fi

# Verify executable exists after build attempt
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo -e "${RED}Error: Test executable not found: $TEST_EXECUTABLE${NC}"
    echo "Build may have failed. Check build output above."
    exit 1
fi

# Set test filters and timeouts based on mode
GTEST_FILTER=""
TIMEOUT_DURATION=""
case "$TEST_MODE" in
    "help")
        echo "Input System Test Runner"
        echo ""
        echo "Usage: $0 [build_dir] [--quick|--full|--slow|--help]"
        echo ""
        echo "Modes:"
        echo "  --quick  : Fast essential tests (<1s, 147 tests)"
        echo "  --full   : All stable tests (<5s, 147 tests)"
        echo "  --slow   : Disabled and PlaneDetector tests (>5s)"
        echo ""
        echo "Examples:"
        echo "  $0                    # Quick tests with default build"
        echo "  $0 --quick           # Quick tests with default build"
        echo "  $0 build_debug       # Quick tests with debug build"
        echo "  $0 build_debug --full # Full tests with debug build"
        exit 0
        ;;
    "quick")
        # Exclude PlaneDetector tests, stress tests, and performance tests
        # PlaneDetector tests excluded due to potential hanging issues
        GTEST_FILTER="--gtest_filter=-*PlaneDetectorTest*:*Stress*:*Performance*:*Large*:*DISABLED_*"
        TIMEOUT_DURATION="60s"
        echo -e "${GREEN}Running quick tests (excluding PlaneDetector and performance tests)...${NC}"
        echo "Expected: 147 tests in <1 second"
        ;;
    "full")
        # Run all tests except disabled ones and PlaneDetector tests
        # PlaneDetector tests have unpredictable hanging behavior, so moved to slow mode
        GTEST_FILTER="--gtest_filter=-*DISABLED_*:*PlaneDetectorTest*"
        TIMEOUT_DURATION="300s"
        echo -e "${GREEN}Running all stable tests...${NC}"
        echo "Expected: 147 tests in <5 seconds (excludes PlaneDetector tests)"
        ;;
    "slow")
        # Run only disabled tests, performance tests, and PlaneDetector tests
        GTEST_FILTER="--gtest_filter=*DISABLED_*:*Stress*:*Performance*:*Large*:*PlaneDetectorTest*"
        TIMEOUT_DURATION=""  # No timeout for slow tests
        echo -e "${YELLOW}Running slow/disabled tests...${NC}"
        echo "Expected: Variable timing, may take minutes"
        ;;
esac

# Run the tests with timing and appropriate timeout
echo ""
echo "==========================================="
echo "Executing Input System tests..."
echo "==========================================="

# Start timing
START_TIME=$(date +%s)

# Execute tests with timeout based on mode
if [ -n "$TIMEOUT_DURATION" ]; then
    TIMEOUT_CMD="timeout $TIMEOUT_DURATION"
else
    TIMEOUT_CMD=""
fi

# Run tests with timing output
if $TIMEOUT_CMD "$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" \
    --gtest_print_time=1 \
    $GTEST_FILTER \
    --gtest_color=yes; then
    
    # Calculate execution time
    END_TIME=$(date +%s)
    DURATION=$((END_TIME - START_TIME))
    
    echo ""
    echo "==========================================="
    echo -e "${GREEN}Input System tests completed successfully!${NC}"
    echo "Total execution time: ${DURATION} seconds"
    
    # Performance warnings
    if [[ "$TEST_MODE" == "quick" && $DURATION -gt 1 ]]; then
        echo ""
        echo -e "${YELLOW}⚠️  WARNING: Quick tests took longer than expected (${DURATION}s > 1s)${NC}"
        echo "   Consider investigating slow tests"
    elif [[ "$TEST_MODE" == "full" && $DURATION -gt 5 ]]; then
        echo ""
        echo -e "${YELLOW}⚠️  WARNING: Full tests took longer than expected (${DURATION}s > 5s)${NC}"
        echo "   Some tests may need optimization"
    fi
else
    EXIT_CODE=$?
    END_TIME=$(date +%s)
    DURATION=$((END_TIME - START_TIME))
    
    echo ""
    echo "==========================================="
    if [[ $EXIT_CODE -eq 124 ]]; then
        echo -e "${RED}❌ ERROR: Tests timed out after $TIMEOUT_DURATION${NC}"
        echo "   Try running with --slow mode for problematic tests"
    else
        echo -e "${RED}❌ ERROR: Tests failed with exit code $EXIT_CODE${NC}"
    fi
    echo "Execution time before failure: ${DURATION} seconds"
    exit $EXIT_CODE
fi

# Test categories documentation
echo ""
echo "Test Categories and Timing:"
echo "- InputTypes: Type construction and validation (15 tests, <1ms each)"
echo "- InputMapping: Configuration and serialization (18 tests, <1ms each)"  
echo "- MouseHandler: Mouse event processing (13 tests, <1ms each)"
echo "- KeyboardHandler: Keyboard input handling (14 tests, <1ms each)"
echo "- TouchHandler: Touch gesture processing (12 tests, <1ms each)"
echo "- VRInputHandler: VR hand tracking (17 tests, <1ms each)"
echo "- PlacementValidation: Voxel placement rules (9 tests, <1ms each)"
echo "- SmartSnapping: Intelligent snapping logic (8 tests, ~1ms total)"
echo "- ModifierKeyTracking: Modifier key states (7 tests, <1ms each)"
echo "- PlaneDetector: Placement plane detection (11 tests, 1-8ms each, some disabled)"
echo "- InputRequirements: Requirement validation (34 tests, ~4ms total)"
echo ""
echo "Mode Details:"
echo "- --quick: Excludes PlaneDetector and performance tests for fastest execution"
echo "- --full:  Includes all stable tests, excludes PlaneDetector tests"
echo "- --slow:  Runs disabled tests, performance tests, and PlaneDetector tests"
echo ""
echo "To run specific test categories:"
echo "  $TEST_EXECUTABLE --gtest_filter='CategoryName*'"
echo ""
echo "To identify slow tests:"
echo "  $TEST_EXECUTABLE --gtest_print_time=1"