#!/bin/bash

# Voxel Data subsystem test runner
# Usage: ./run_tests.sh [build_dir] [mode]
#   build_dir: Build directory (default: build_ninja)
#   mode: Test mode - quick, full, performance, or requirements (default: full)
#
# Examples:
#   ./run_tests.sh                    # Run all tests except performance
#   ./run_tests.sh build_ninja quick  # Run quick tests only
#   ./run_tests.sh build_debug performance # Run performance tests

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
print_color $BLUE "VoxelData Subsystem Test Runner"
echo "=========================================="
echo "Build directory: $BUILD_DIR"
echo "Test mode: $TEST_MODE"
echo "Project root: $PROJECT_ROOT"
echo ""

# Change to project root
cd "$PROJECT_ROOT"

# Ensure build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    print_color $RED "Error: Build directory '$BUILD_DIR' does not exist"
    echo "Please run: cmake -B $BUILD_DIR -G Ninja"
    exit 1
fi

# Check if test executable exists
TEST_EXECUTABLE="$BUILD_DIR/bin/VoxelEditor_VoxelData_Tests"
if [ ! -f "$TEST_EXECUTABLE" ]; then
    print_color $YELLOW "Building Voxel Data tests..."
    cmake --build "$BUILD_DIR" --target VoxelEditor_VoxelData_Tests
fi

# Define test filters based on mode
case "$TEST_MODE" in
    quick)
        print_color $YELLOW "Running QUICK tests (excluding performance/stress tests)"
        # Exclude the 56s collision test and other stress tests
        GTEST_FILTER="*:-*Performance*:*Stress*:*Memory*:VoxelDataManagerTest.PerformanceTest_CollisionCheck10000Voxels"
        TIMEOUT=60
        ;;
    performance)
        print_color $YELLOW "Running PERFORMANCE tests only"
        # Run only performance, stress, and memory tests
        GTEST_FILTER="*Performance*:*Stress*:*Memory*"
        TIMEOUT=300
        ;;
    requirements)
        print_color $YELLOW "Running REQUIREMENTS tests only"
        GTEST_FILTER="VoxelDataRequirementsTest.*"
        TIMEOUT=120
        ;;
    full|*)
        print_color $YELLOW "Running ALL tests except extreme performance tests"
        # Exclude only the extremely slow collision test
        GTEST_FILTER="*:-VoxelDataManagerTest.PerformanceTest_CollisionCheck10000Voxels"
        TIMEOUT=180
        ;;
esac

echo ""
print_color $BLUE "Executing Voxel Data tests..."
echo "=========================================="

# Run the tests with timing information
print_color $YELLOW "Test timeout: ${TIMEOUT}s"
print_color $YELLOW "Test filter: ${GTEST_FILTER}"
echo ""

# Execute tests with detailed timing
if timeout $TIMEOUT "$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" \
    --gtest_filter="$GTEST_FILTER" \
    --gtest_print_time=1 \
    --gtest_output=xml:voxel_data_test_results.xml \
    2>&1 | tee test_output.log; then
    
    echo ""
    print_color $GREEN "✓ Voxel Data tests completed successfully!"
    
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
        echo "Tests exceeding 5 seconds:"
        grep -E "\[[[:space:]]+OK[[:space:]]+\].*\([5-9][0-9]{3,} ms\)" test_output.log
        echo ""
        print_color $YELLOW "Consider running these in performance mode: ./run_tests.sh $BUILD_DIR performance"
    else
        print_color $GREEN "✓ All tests within 5-second limit"
    fi
    
    # Memory usage summary for performance tests
    if [ "$TEST_MODE" = "performance" ]; then
        echo ""
        print_color $BLUE "Memory Usage Report:"
        echo "=========================================="
        if grep -i "memory" test_output.log | grep -E "(bytes|KB|MB|GB)" > /dev/null; then
            grep -i "memory" test_output.log | grep -E "(bytes|KB|MB|GB)" | head -10
        else
            echo "No memory usage data found in test output"
        fi
    fi
    
else
    # Check if no tests were found
    if grep -q "No tests were found" test_output.log; then
        print_color $YELLOW "Note: No tests matched the filter."
        echo "Filter used: $GTEST_FILTER"
    else
        print_color $RED "✗ Voxel Data tests failed!"
        
        # Show failed tests
        echo ""
        print_color $RED "Failed tests:"
        grep -E "\[[[:space:]]+FAILED[[:space:]]+\]" test_output.log || echo "  Unable to parse failures"
        
        exit 1
    fi
fi

# Clean up
rm -f test_output.log voxel_data_test_results.xml

echo ""
print_color $BLUE "Test Mode Summary:"
echo "=========================================="
echo "- quick: Excludes performance/stress tests (fastest)"
echo "- full: Runs all tests except extreme performance tests"
echo "- performance: Runs only performance/stress/memory tests"
echo "- requirements: Runs only requirement validation tests"
echo ""
print_color $BLUE "Typical execution times:"
echo "- Quick mode: ~0.5s"
echo "- Requirements mode: ~0.05s"
echo "- Full mode: ~1s"
echo "- Performance mode: ~57s (includes 56s collision test)"
echo ""
print_color $YELLOW "Note: The PerformanceTest_CollisionCheck10000Voxels (56s) is excluded"
print_color $YELLOW "from quick and full modes due to exceeding the 5-second rule."
print_color $YELLOW "Run in performance mode to execute this stress test."