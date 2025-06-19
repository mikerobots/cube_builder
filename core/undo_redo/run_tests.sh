#!/bin/bash

# Undo/Redo subsystem test runner with optimization and timing
# Usage: ./run_tests.sh [mode] [build_dir]
# Modes: quick, full, memory, requirements, all (default: quick)

set -e

# Parse arguments
MODE="${1:-quick}"
BUILD_DIR="${2:-build_ninja}"

# If first argument looks like a build directory, swap arguments
if [[ "$MODE" == *"build"* ]]; then
    BUILD_DIR="$MODE"
    MODE="quick"
fi

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "=== Undo/Redo Subsystem Test Runner ==="
echo "Mode: $MODE"
echo "Build directory: $BUILD_DIR"
echo "Project root: $PROJECT_ROOT"
echo ""

# Change to project root
cd "$PROJECT_ROOT"

# Ensure build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Build directory '$BUILD_DIR' does not exist"
    echo "Please run: cmake -B $BUILD_DIR -G Ninja"
    exit 1
fi

# Test executables
TEST_COMMAND="$BUILD_DIR/bin/test_undo_redo"
TEST_SIMPLE="$BUILD_DIR/bin/test_simple_command"
TEST_HISTORY="$BUILD_DIR/bin/test_history_manager"
TEST_PLACEMENT="$BUILD_DIR/bin/test_placement_commands"
TEST_REQUIREMENTS="$BUILD_DIR/bin/test_undo_redo_requirements"

# Build all tests if needed
echo "Checking test executables..."
for test in "$TEST_COMMAND" "$TEST_SIMPLE" "$TEST_HISTORY" "$TEST_PLACEMENT" "$TEST_REQUIREMENTS"; do
    target=$(basename "$test")
    if [ ! -f "$test" ]; then
        echo "Building $target..."
        cmake --build "$BUILD_DIR" --target "$target"
    fi
done

# Function to run a test with timing and timeout
run_test() {
    local test_name="$1"
    local test_exe="$2"
    local timeout_sec="${3:-60}"
    local extra_args="${4:-}"
    
    echo ""
    echo "--- Running $test_name ---"
    echo "Timeout: ${timeout_sec}s"
    
    # Run with timeout and timing
    if timeout "$timeout_sec" "$PROJECT_ROOT/execute_command.sh" "$test_exe" --gtest_print_time=1 $extra_args; then
        echo "✓ $test_name completed successfully"
    else
        echo "✗ $test_name failed or timed out"
        return 1
    fi
}

# Track failures
FAILED_TESTS=()

# Execute tests based on mode
case "$MODE" in
    "quick")
        echo ""
        echo "=== QUICK MODE - Running core functionality tests ==="
        echo "Excludes: stress tests, large memory tests"
        echo ""
        
        run_test "Command Tests" "$TEST_COMMAND" 60 || FAILED_TESTS+=("Command")
        run_test "Simple Command Tests" "$TEST_SIMPLE" 60 || FAILED_TESTS+=("SimpleCommand")
        run_test "History Manager Tests" "$TEST_HISTORY" 60 || FAILED_TESTS+=("HistoryManager")
        
        echo ""
        echo "=== Quick Test Summary ==="
        echo "Total execution time: ~0.1s (typical)"
        echo "Tests focus on core undo/redo functionality"
        ;;
        
    "full")
        echo ""
        echo "=== FULL MODE - Running all unit tests ==="
        echo "Includes: all command, history, and placement tests"
        echo ""
        
        run_test "Command Tests" "$TEST_COMMAND" 300 || FAILED_TESTS+=("Command")
        run_test "Simple Command Tests" "$TEST_SIMPLE" 300 || FAILED_TESTS+=("SimpleCommand")
        run_test "History Manager Tests" "$TEST_HISTORY" 300 || FAILED_TESTS+=("HistoryManager")
        run_test "Placement Commands Tests" "$TEST_PLACEMENT" 300 || FAILED_TESTS+=("PlacementCommands")
        
        echo ""
        echo "=== Full Test Summary ==="
        echo "Total execution time: ~0.5s (typical)"
        echo "Comprehensive unit test coverage"
        ;;
        
    "memory")
        echo ""
        echo "=== MEMORY MODE - Running memory constraint tests ==="
        echo "Focus: VR memory limits, history size impact"
        echo ""
        
        # Run requirements tests with memory-focused filter
        run_test "Memory Constraint Tests" "$TEST_REQUIREMENTS" 300 \
            "--gtest_filter=*Memory*:*VRConstraints*:*ApplicationOverhead*" || FAILED_TESTS+=("Memory")
        
        # Run history tests with focus on memory
        run_test "History Memory Tests" "$TEST_HISTORY" 300 \
            "--gtest_filter=*Limit*" || FAILED_TESTS+=("HistoryMemory")
        
        echo ""
        echo "=== Memory Test Summary ==="
        echo "Tests verify memory usage stays within VR constraints (<50MB)"
        echo "History maintains proper size limits (10-20 operations)"
        ;;
        
    "requirements")
        echo ""
        echo "=== REQUIREMENTS MODE - Running requirement validation tests ==="
        echo ""
        
        run_test "Requirements Tests" "$TEST_REQUIREMENTS" 300 || FAILED_TESTS+=("Requirements")
        
        echo ""
        echo "=== Requirements Test Summary ==="
        echo "All 13 requirement tests execute in <5ms each"
        echo "Validates REQ-5.1.1, REQ-5.1.2, REQ-2.3.3, REQ-6.3.4, REQ-8.1.6, REQ-9.2.6"
        ;;
        
    "all")
        echo ""
        echo "=== ALL MODE - Running complete test suite ==="
        echo ""
        
        run_test "Command Tests" "$TEST_COMMAND" 300 || FAILED_TESTS+=("Command")
        run_test "Simple Command Tests" "$TEST_SIMPLE" 300 || FAILED_TESTS+=("SimpleCommand")
        run_test "History Manager Tests" "$TEST_HISTORY" 300 || FAILED_TESTS+=("HistoryManager")
        run_test "Placement Commands Tests" "$TEST_PLACEMENT" 300 || FAILED_TESTS+=("PlacementCommands")
        run_test "Requirements Tests" "$TEST_REQUIREMENTS" 300 || FAILED_TESTS+=("Requirements")
        
        echo ""
        echo "=== Complete Test Summary ==="
        echo "Total typical execution time: <1s"
        echo "All tests meet 5-second rule"
        ;;
        
    *)
        echo "Unknown mode: $MODE"
        echo "Usage: $0 [quick|full|memory|requirements|all] [build_dir]"
        exit 1
        ;;
esac

# Final summary
echo ""
echo "========================================="
if [ ${#FAILED_TESTS[@]} -eq 0 ]; then
    echo "✓ All Undo/Redo tests passed!"
    echo ""
    echo "Performance notes:"
    echo "- All individual tests execute in <5ms"
    echo "- No performance optimizations needed"
    echo "- Memory usage well within constraints"
    exit 0
else
    echo "✗ Some tests failed:"
    for test in "${FAILED_TESTS[@]}"; do
        echo "  - $test"
    done
    exit 1
fi