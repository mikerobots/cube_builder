#!/bin/bash

# Input subsystem test runner
# Usage: ./run_tests.sh [build_dir]

set -e

# Default build directory
BUILD_DIR="${1:-build_ninja}"

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "Running Input subsystem tests..."
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
TEST_EXECUTABLE="$BUILD_DIR/bin/VoxelEditor_Input_Tests"
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo "Building Input tests..."
    cmake --build "$BUILD_DIR" --target VoxelEditor_Input_Tests
fi

# Run the tests
echo ""
echo "Executing Input tests..."
echo "==========================================="
"$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE"

echo ""
echo "Input tests completed successfully!"