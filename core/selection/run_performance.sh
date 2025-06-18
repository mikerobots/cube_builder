#!/bin/bash

# Selection subsystem performance test runner
# Usage: ./run_performance.sh [build_dir]

set -e

# Default build directory
BUILD_DIR="${1:-build_ninja}"

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "Running Selection subsystem PERFORMANCE tests..."
echo "Build directory: $BUILD_DIR"
echo "Project root: $PROJECT_ROOT"

# Change to project root
cd "$PROJECT_ROOT"

# Ensure build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Build directory '$BUILD_DIR' does not exist"
    echo "Please run: cmake -B $BUILD_DIR -G Ninja"
    exit 1
fi

# Check if test executable exists
TEST_EXECUTABLE="$BUILD_DIR/bin/VoxelEditor_Selection_Tests"
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo "Building Selection tests..."
    cmake --build "$BUILD_DIR" --target VoxelEditor_Selection_Tests
fi

# Run only performance tests
echo ""
echo "Executing Selection PERFORMANCE tests..."
echo "==========================================="
echo "Note: These tests may take longer to run as they test performance with large datasets"
echo ""

# Run performance-related tests
"$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" \
    --gtest_filter="*Performance*:*LargeSelection*:*VeryLargeRadius*:*Stress*"

echo ""
echo "Selection performance tests completed!"