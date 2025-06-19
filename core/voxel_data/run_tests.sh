#!/bin/bash

# VoxelData System test runner with standardized options
# Usage: ./run_tests.sh [build_dir] [--quick|--full|--slow]
#
# Test execution modes:
#   --quick  : Run all tests under 1 second (default)
#   --full   : Run all tests under 5 seconds
#   --slow   : Run all tests over 5 seconds (performance tests)
#
# Expected execution times:
#   --quick mode: <1 second total (104 tests, excludes slow VoxelDataStorageLimit test)
#   --full mode:  <5 seconds total (105 tests, includes VoxelDataStorageLimit)
#   --slow mode:  Variable (12 performance tests, may take 40+ seconds)

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
elif [[ "$2" == "--help" ]]; then
    echo "VoxelData System Test Runner"
    echo ""
    echo "Usage: $0 [build_dir] [--quick|--full|--slow|--help]"
    echo ""
    echo "Modes:"
    echo "  --quick  : Fast essential tests (<1s, 104 tests)"
    echo "  --full   : All stable tests (<5s, 105 tests)"
    echo "  --slow   : Performance and stress tests (>5s, 12 tests)"
    echo ""
    echo "Examples:"
    echo "  $0                    # Quick tests with default build"
    echo "  $0 --quick           # Quick tests with default build"
    echo "  $0 build_debug       # Quick tests with debug build"
    echo "  $0 build_debug --full # Full tests with debug build"
    exit 0
fi

# Check if first argument is actually a mode flag
if [[ "$1" == "--quick" || "$1" == "--full" || "$1" == "--slow" || "$1" == "--help" ]]; then
    BUILD_DIR="build_ninja"
    TEST_MODE="${1#--}"
fi

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo -e "${BLUE}Running VoxelData System tests...${NC}"
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
TEST_EXECUTABLE="$BUILD_DIR/bin/VoxelEditor_VoxelData_Tests"
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo "Building VoxelData tests..."
    cmake --build "$BUILD_DIR" --target VoxelEditor_VoxelData_Tests
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
    "quick")
        # Exclude performance tests and the slow VoxelDataStorageLimit test (208ms)
        GTEST_FILTER="--gtest_filter=-*Performance*:*Stress*:*Memory*:VoxelDataRequirementsTest.VoxelDataStorageLimit_REQ_6_3_2"
        TIMEOUT_DURATION="60s"
        echo -e "${GREEN}Running quick tests (excluding performance and slow storage tests)...${NC}"
        echo "Expected: 104 tests in <1 second"
        ;;
    "full")
        # Run all tests except extreme performance tests
        # Include the 208ms VoxelDataStorageLimit test but exclude the 38s collision test
        GTEST_FILTER="--gtest_filter=-*Performance*:*Stress*:*Memory*"
        TIMEOUT_DURATION="300s"
        echo -e "${GREEN}Running all stable tests...${NC}"
        echo "Expected: 105 tests in <5 seconds"
        ;;
    "slow")
        # Run only performance, stress, and memory tests
        GTEST_FILTER="--gtest_filter=*Performance*:*Stress*:*Memory*"
        TIMEOUT_DURATION=""  # No timeout for slow tests
        echo -e "${YELLOW}Running performance and stress tests...${NC}"
        echo "Expected: 12 tests, may take 40+ seconds"
        ;;
esac

# Run the tests with timing and appropriate timeout
echo ""
echo "==========================================="
echo "Executing VoxelData System tests..."
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
    echo -e "${GREEN}VoxelData System tests completed successfully!${NC}"
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
echo "- VoxelTypes: Type definitions and validation (13 tests, <1ms each)"
echo "- SparseOctree: Memory-efficient voxel storage (11 tests, ~1ms total)"
echo "- VoxelGrid: Individual resolution level storage (13 tests, ~1ms total)"
echo "- WorkspaceManager: Workspace bounds and resizing (21 tests, <1ms each)"
echo "- VoxelDataManager: Multi-resolution voxel management (29 tests, ~10ms total)"
echo "- VoxelDataRequirements: Requirement validation (16 tests, includes 208ms storage test)"
echo "- CollisionSimple: Basic collision detection (2 tests, <1ms each)"
echo ""
echo "Performance Tests (--slow mode only):"
echo "- SparseOctree stress tests (3 tests, up to 133ms each)"
echo "- VoxelGrid stress tests (3 tests, up to 261ms each)"
echo "- VoxelDataManager performance tests (3 tests, includes 38.7s collision test)"
echo "- VoxelDataRequirements performance tests (3 tests, up to 19ms each)"
echo ""
echo "Mode Details:"
echo "- --quick: Excludes performance tests and slow storage test (208ms)"
echo "- --full:  Includes all tests except performance/stress tests"
echo "- --slow:  Runs only performance, stress, and memory tests"
echo ""
echo "To run specific test categories:"
echo "  $TEST_EXECUTABLE --gtest_filter='CategoryName*'"
echo ""
echo "To identify slow tests:"
echo "  $TEST_EXECUTABLE --gtest_print_time=1"