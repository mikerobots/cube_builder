#!/bin/bash

# File I/O subsystem test runner
# Usage: ./run_tests.sh [build_dir]

set -e

# Default build directory
BUILD_DIR="${1:-build_ninja}"

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "Running File I/O subsystem tests..."
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
TEST_EXECUTABLE="$BUILD_DIR/bin/file_io_tests"
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo "Note: File I/O tests are not yet implemented (CMakeLists.txt has commented out test target)"
    echo "Please check core/file_io/tests/CMakeLists.txt and implement the test files"
    exit 0
fi

# Run the tests
echo ""
echo "Executing File I/O tests..."
echo "==========================================="
"$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE"

echo ""
echo "File I/O tests completed successfully!"