#!/bin/bash

# Voxel Data Performance Test Runner
# Runs performance tests with longer timeouts

set -euo pipefail

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# Script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Default build directory
BUILD_DIR="${BUILD_DIR:-build_ninja}"

# Function to print colored output
print_color() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

# Function to print section header
print_header() {
    local title=$1
    echo
    print_color "$BOLD$BLUE" "=========================================="
    print_color "$BOLD$BLUE" "$title"
    print_color "$BOLD$BLUE" "=========================================="
    echo
}

# Check if performance test executables exist
print_header "Voxel Data Performance Tests"

cd "$PROJECT_ROOT"

# Build performance tests if needed
print_color "$CYAN" "Building performance tests..."
cmake --build "$BUILD_DIR" --target test_uperf_core_voxel_data_manager test_uperf_core_voxel_data_requirements 2>/dev/null || {
    print_color "$YELLOW" "Performance tests not built yet. Building now..."
    cmake --build "$BUILD_DIR"
}

# Run performance tests with longer timeout
print_color "$CYAN" "Running performance tests (this may take a few minutes)..."
echo

# Test 1: Manager performance
print_color "$YELLOW" "Running: test_uperf_core_voxel_data_manager"
if timeout 300 "$PROJECT_ROOT/execute_command.sh" "$PROJECT_ROOT/$BUILD_DIR/bin/test_uperf_core_voxel_data_manager"; then
    print_color "$GREEN" "✓ Manager performance tests passed"
else
    print_color "$RED" "✗ Manager performance tests failed"
fi

echo

# Test 2: Requirements performance  
print_color "$YELLOW" "Running: test_uperf_core_voxel_data_requirements"
if timeout 300 "$PROJECT_ROOT/execute_command.sh" "$PROJECT_ROOT/$BUILD_DIR/bin/test_uperf_core_voxel_data_requirements"; then
    print_color "$GREEN" "✓ Requirements performance tests passed"
else
    print_color "$RED" "✗ Requirements performance tests failed"
fi

print_header "Performance Test Summary"
print_color "$CYAN" "Performance tests completed. Check output above for timing details."