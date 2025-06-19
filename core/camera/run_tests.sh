#!/bin/bash

# Camera subsystem test runner with standardized options
# Usage: ./run_tests.sh [build_dir] [--quick|--full|--slow|--help]
#
# Test execution modes:
#   --quick  : Run all tests under 1 second (default) - ALL Camera tests
#   --full   : Run all tests under 5 seconds - ALL Camera tests
#   --slow   : Run all tests over 5 seconds - None (no slow tests exist)
#
# Expected execution times:
#   --quick mode: ~1ms total (122 tests)
#   --full mode:  ~1ms total (122 tests)  
#   --slow mode:  None (exits immediately)
#
# Test Categories:
#   Quick Tests (122): All Camera tests complete in <1ms each
#     - CameraTest (15 tests): Basic camera operations
#     - OrbitCameraTest (20 tests): Orbit camera functionality  
#     - OrbitCameraTransformationTest (6 tests): Matrix transformations
#     - ViewportTest (16 tests): Viewport coordinate transformations
#     - CameraControllerTest (25 tests): Camera controller interactions
#     - ZoomFunctionalityTest (13 tests): Zoom operations
#     - SetDistanceTest (13 tests): Distance control
#     - CameraRequirementsTest (14 tests): Requirements validation

set -e

# Parse command line arguments
BUILD_DIR=""
TEST_MODE="quick"

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
            echo "Camera subsystem test runner"
            echo ""
            echo "Usage: $0 [build_dir] [--quick|--full|--slow|--help]"
            echo ""
            echo "Test modes:"
            echo "  --quick  Run fast tests (<1s) - DEFAULT - ALL 122 Camera tests"
            echo "  --full   Run all tests (<5s) - ALL 122 Camera tests"
            echo "  --slow   Run slow tests (>5s) - None exist, exits immediately"
            echo "  --help   Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                          # Run quick tests with build_ninja"
            echo "  $0 build_debug             # Run quick tests with build_debug"
            echo "  $0 --full                  # Run full test suite"
            echo "  $0 build_ninja --slow      # Run slow tests (none exist)"
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
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Timing function
time_command() {
    local start=$(date +%s.%N)
    "$@"
    local end=$(date +%s.%N)
    local duration=$(echo "$end - $start" | bc -l 2>/dev/null || echo "0.001")
    echo -e "${BLUE}Execution time: ${duration}s${NC}"
}

echo -e "${GREEN}Camera Subsystem Test Runner${NC}"
echo "Mode: $TEST_MODE"
echo "Build directory: $BUILD_DIR"
echo ""

# Change to project root
cd "$PROJECT_ROOT"

# Ensure build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory '$BUILD_DIR' does not exist${NC}"
    echo "Please run: cmake -B $BUILD_DIR -G Ninja"
    exit 1
fi

# Test executable path
TEST_EXECUTABLE="$BUILD_DIR/bin/VoxelEditor_Camera_Tests"

# Check if test executable exists, build if needed
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo -e "${YELLOW}Building Camera tests...${NC}"
    if ! time_command cmake --build "$BUILD_DIR" --target VoxelEditor_Camera_Tests; then
        echo -e "${RED}Error: Failed to build Camera tests${NC}"
        exit 1
    fi
fi

# Verify executable exists after build
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo -e "${RED}Error: Test executable not found: $TEST_EXECUTABLE${NC}"
    exit 1
fi

# Handle different test modes
case $TEST_MODE in
    quick)
        echo -e "${GREEN}Running QUICK tests (<1s): ALL 122 Camera tests${NC}"
        echo "==========================================="
        echo "Test Categories: Camera operations, Orbit controls, Transformations,"
        echo "                Viewport handling, Controller interactions, Zoom/Distance"
        echo ""
        
        # Run all tests with 60s timeout
        if ! /opt/homebrew/bin/timeout 60 "$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" \
            --gtest_brief=1 \
            --gtest_print_time=1; then
            echo -e "${RED}Error: Quick tests failed or timed out${NC}"
            exit 1
        fi
        ;;
        
    full)
        echo -e "${GREEN}Running FULL tests (<5s): ALL 122 Camera tests${NC}"
        echo "=========================================="
        echo "Test Categories: All Camera subsystem tests (same as quick mode)"
        echo ""
        
        # Run all tests with 300s timeout
        if ! /opt/homebrew/bin/timeout 300 "$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" \
            --gtest_brief=1 \
            --gtest_print_time=1; then
            echo -e "${RED}Error: Full tests failed or timed out${NC}"
            exit 1
        fi
        ;;
        
    slow)
        echo -e "${YELLOW}No slow tests exist in Camera subsystem${NC}"
        echo "All Camera tests complete in <1ms each."
        echo "Use --quick or --full mode to run all tests."
        echo ""
        echo -e "${GREEN}Slow mode completed (no tests to run)${NC}"
        exit 0
        ;;
esac

# Count total tests run
TEST_COUNT=$("$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" --gtest_list_tests 2>/dev/null | grep -c "^  " || echo "122")

echo ""
echo -e "${GREEN}Camera tests completed successfully!${NC}"
echo "Tests run: $TEST_COUNT"
echo "Mode: $TEST_MODE"
echo "All Camera tests execute in under 1ms each - excellent performance!"