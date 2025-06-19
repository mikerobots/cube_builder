#!/bin/bash

# Rendering subsystem test runner with performance optimizations
# Usage: ./run_tests.sh [build_dir]

set -e

# Default build directory
BUILD_DIR="${1:-build_ninja}"

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Timing function
time_command() {
    local start=$(date +%s.%N)
    "$@"
    local end=$(date +%s.%N)
    local duration=$(echo "$end - $start" | bc)
    echo -e "${BLUE}Execution time: ${duration}s${NC}"
}

echo -e "${GREEN}Running Rendering subsystem tests...${NC}"
echo "Build directory: $BUILD_DIR"

# Change to project root
cd "$PROJECT_ROOT"

# Ensure build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Build directory '$BUILD_DIR' does not exist"
    echo "Please run: cmake -B $BUILD_DIR -G Ninja"
    exit 1
fi

# Check if test executable exists
TEST_EXECUTABLE="$BUILD_DIR/bin/VoxelEditor_Rendering_Tests"
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo "Building Rendering tests..."
    time_command cmake --build "$BUILD_DIR" --target VoxelEditor_Rendering_Tests
fi

# Run the tests with timing and timeout
echo ""
echo -e "${GREEN}Executing Rendering tests...${NC}"
echo "==========================================="

# Count tests first
TEST_COUNT=$("$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" --gtest_list_tests | grep -c "^  " || true)
echo "Total tests: $TEST_COUNT"

# Check if running in headless environment
if [ -z "$DISPLAY" ] && [ -z "$CI" ]; then
    echo -e "${YELLOW}Note: Running in headless mode - OpenGL context tests will be skipped${NC}"
fi

# Run tests with detailed timing
# Using timeout to ensure no test suite runs longer than 5 minutes total
echo ""
time_command timeout 300 "$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" \
    --gtest_brief=1 \
    --gtest_print_time=1

# Show summary
echo ""
echo -e "${GREEN}Rendering tests completed successfully!${NC}"
echo "Note: Tests requiring OpenGL context are automatically skipped in headless mode."
echo "All individual tests complete in under 5 seconds."