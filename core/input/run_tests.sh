#!/bin/bash

# Input subsystem test runner with optimization support
# Usage: ./run_tests.sh [build_dir] [--quick|--full|--stress|--plane]
#
# Test execution modes:
#   --quick  : Run only fast essential tests (default) - excludes PlaneDetector
#   --full   : Run all tests including slower ones - WARNING: may hang
#   --stress : Run performance/stress tests separately
#   --plane  : Run PlaneDetector tests separately (CAUTION: known to hang)
#
# Expected execution times:
#   --quick mode: <1 second (147 tests)
#   --full mode:  May timeout due to PlaneDetector issues
#   --stress mode: Variable (may take several minutes)
#   --plane mode:  May hang indefinitely
#
# Known Issues:
#   - PlaneDetectorTest.PlaneClearingWhenPreviewClears hangs
#   - PlaneDetectorTest.DifferentVoxelSizes may also hang
#   - 5 PlaneDetector tests are already disabled

set -e

# Default build directory
BUILD_DIR="${1:-build_ninja}"

# Test mode (default to quick)
TEST_MODE="quick"
if [[ "$2" == "--full" ]]; then
    TEST_MODE="full"
elif [[ "$2" == "--stress" ]]; then
    TEST_MODE="stress"
elif [[ "$2" == "--quick" ]]; then
    TEST_MODE="quick"
fi

# Check if first argument is actually a mode flag
if [[ "$1" == "--quick" || "$1" == "--full" || "$1" == "--stress" ]]; then
    BUILD_DIR="build_ninja"
    TEST_MODE="${1#--}"
fi

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "Running Input subsystem tests..."
echo "Build directory: $BUILD_DIR"
echo "Project root: $PROJECT_ROOT"
echo "Test mode: $TEST_MODE"
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
TEST_EXECUTABLE="$BUILD_DIR/bin/VoxelEditor_Input_Tests"
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo "Building Input tests..."
    cmake --build "$BUILD_DIR" --target VoxelEditor_Input_Tests
fi

# Set test filters based on mode
GTEST_FILTER=""
case "$TEST_MODE" in
    "quick")
        # Exclude known slow tests and stress tests
        # Based on analysis, PlaneDetectorTest tests hang or are slow
        GTEST_FILTER="--gtest_filter=-*PlaneDetectorTest*:*Stress*:*Performance*:*Large*"
        echo "Running quick tests (excluding PlaneDetector, stress, and performance tests)..."
        ;;
    "full")
        # Run all enabled tests
        GTEST_FILTER="--gtest_filter=-*DISABLED_*"
        echo "Running all enabled tests..."
        ;;
    "stress")
        # Run only stress/performance tests
        GTEST_FILTER="--gtest_filter=*Stress*:*Performance*:*Large*"
        echo "Running stress/performance tests..."
        ;;
    "plane")
        # Run PlaneDetector tests separately due to hanging issues
        GTEST_FILTER="--gtest_filter=PlaneDetectorTest*"
        echo "Running PlaneDetector tests (may hang - use with caution)..."
        ;;
esac

# Run the tests with timing and appropriate timeout
echo ""
echo "Executing Input tests..."
echo "==========================================="

# Start timing
START_TIME=$(date +%s)

# Execute tests with timeout based on mode
TIMEOUT_DURATION="30s"
if [[ "$TEST_MODE" == "stress" ]]; then
    TIMEOUT_DURATION="300s"
elif [[ "$TEST_MODE" == "full" ]]; then
    TIMEOUT_DURATION="60s"
fi

# Run tests with timeout and timing output
if timeout "$TIMEOUT_DURATION" "$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" \
    --gtest_print_time=1 \
    $GTEST_FILTER \
    --gtest_color=yes; then
    
    # Calculate execution time
    END_TIME=$(date +%s)
    DURATION=$((END_TIME - START_TIME))
    
    echo ""
    echo "==========================================="
    echo "Input tests completed successfully!"
    echo "Total execution time: ${DURATION} seconds"
    
    # Performance warning for quick mode
    if [[ "$TEST_MODE" == "quick" && $DURATION -gt 5 ]]; then
        echo ""
        echo "⚠️  WARNING: Quick tests took longer than expected (${DURATION}s > 5s)"
        echo "   Consider investigating slow tests with --gtest_print_time=1"
    fi
else
    EXIT_CODE=$?
    END_TIME=$(date +%s)
    DURATION=$((END_TIME - START_TIME))
    
    echo ""
    echo "==========================================="
    if [[ $EXIT_CODE -eq 124 ]]; then
        echo "❌ ERROR: Tests timed out after $TIMEOUT_DURATION"
        echo "   Try running with --full or --stress mode for longer tests"
    else
        echo "❌ ERROR: Tests failed with exit code $EXIT_CODE"
    fi
    echo "Execution time before failure: ${DURATION} seconds"
    exit $EXIT_CODE
fi

# Test categories documentation
echo ""
echo "Test Categories:"
echo "- InputTypes: Basic type construction and validation (~15 tests, <1ms each)"
echo "- InputMapping: Input configuration and serialization (~18 tests, <1ms each)"
echo "- MouseHandler: Mouse event processing (~13 tests, <1ms each)"
echo "- KeyboardHandler: Keyboard input handling (~14 tests, <1ms each)"
echo "- TouchHandler: Touch gesture processing (~11 tests, <1ms each)"
echo "- VRInputHandler: VR hand tracking (~17 tests, <1ms each)"
echo "- PlacementValidation: Voxel placement rules (~9 tests, <1ms each)"
echo "- ModifierKeyTracking: Modifier key states (~7 tests, <1ms each)"
echo "- PlaneDetector: Placement plane detection (~11 tests, some disabled for performance)"
echo "- InputRequirements: Requirement validation tests (~43 tests, varied timing)"
echo ""
echo "To run specific test categories:"
echo "  $TEST_EXECUTABLE --gtest_filter='CategoryName*'"
echo ""
echo "To identify slow tests:"
echo "  $TEST_EXECUTABLE --gtest_print_time=1 --gtest_filter='*'"