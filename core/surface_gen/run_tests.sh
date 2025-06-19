#!/bin/bash

# Surface Generation subsystem test runner
# Usage: ./run_tests.sh [--quick|--full|--performance] [build_dir]
#
# Performance characteristics:
# - Quick mode: ~2s total (excludes slow mesh generation tests)
# - Full mode: ~13s total (includes all standard tests)
# - Performance mode: ~20s+ (includes stress tests)
#
# Notable slow tests:
# - PreviewMeshGeneration: ~3.5s
# - ExportMeshGeneration: ~1.9s
# - CacheDisabled: ~1.0s
#
# Options:
#   --quick: Run core functionality tests only, exclude slow tests
#   --full: Run all standard tests (default)
#   --performance: Run all tests including stress/performance tests

set -e

# Parse arguments
MODE="full"
BUILD_DIR=""

for arg in "$@"; do
    case $arg in
        --quick)
            MODE="quick"
            ;;
        --full)
            MODE="full"
            ;;
        --performance)
            MODE="performance"
            ;;
        *)
            BUILD_DIR="$arg"
            ;;
    esac
done

# Default build directory if not specified
BUILD_DIR="${BUILD_DIR:-build_ninja}"

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "=========================================="
echo "Surface Generation Test Runner"
echo "=========================================="
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

# Check if test executable exists
TEST_EXECUTABLE="$BUILD_DIR/bin/test_surface_gen"
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo "Building Surface Generation tests..."
    cmake --build "$BUILD_DIR" --target test_surface_gen
fi

# Set test filters based on mode
GTEST_FILTER=""
TIMEOUT_DURATION="300s"

if [ "$MODE" = "quick" ]; then
    # Quick mode: exclude slow tests (>1s)
    # Exclude: PreviewMeshGeneration, ExportMeshGeneration, CacheDisabled, and other slow tests
    GTEST_FILTER="--gtest_filter=-SurfaceGeneratorTest.PreviewMeshGeneration:SurfaceGeneratorTest.ExportMeshGeneration:SurfaceGeneratorTest.CacheDisabled:SurfaceGeneratorTest.BasicGeneration:SurfaceGeneratorTest.CustomSettings:SurfaceGeneratorTest.EmptyGrid:SurfaceGeneratorTest.SingleVoxel:SurfaceGeneratorTest.CacheEnabled:SurfaceGeneratorTest.AsyncGeneration:SurfaceGeneratorTest.MultipleAsyncGenerations:SurfaceGeneratorTest.ProgressCallback:SurfaceGeneratorTest.VoxelDataChanged:SurfaceGeneratorTest.CacheMemoryLimit:SurfaceGeneratorTest.ClearCache:SurfaceGenRequirementsTest.MultiResolutionLOD:SurfaceGenRequirementsTest.HighQualityExport:MeshCacheTest.LRUEviction"
    TIMEOUT_DURATION="60s"
    echo "Running in QUICK mode (core tests only, ~2s total)"
elif [ "$MODE" = "performance" ]; then
    # Performance mode: run all tests including stress tests
    GTEST_FILTER=""
    TIMEOUT_DURATION="600s"
    echo "Running in PERFORMANCE mode (all tests including stress tests)"
else
    # Full mode: standard tests
    GTEST_FILTER="--gtest_filter=-*Stress*:*Performance*:*LargeScale*"
    echo "Running in FULL mode (standard tests, ~13s total)"
fi

# Run the tests with timing and timeout
echo ""
echo "Executing Surface Generation tests..."
echo "=========================================="

# Start timer
START_TIME=$(date +%s.%N 2>/dev/null || date +%s)

# Run tests with:
# - Timing output for each test
# - Timeout based on mode
# - Color output
timeout $TIMEOUT_DURATION "$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" \
    --gtest_print_time=1 \
    --gtest_color=yes \
    $GTEST_FILTER

TEST_EXIT_CODE=$?

# End timer
END_TIME=$(date +%s.%N 2>/dev/null || date +%s)

# Calculate duration (handle systems without nanosecond support)
if [[ $START_TIME == *.* ]] && [[ $END_TIME == *.* ]]; then
    DURATION=$(echo "$END_TIME - $START_TIME" | bc)
else
    DURATION=$((END_TIME - START_TIME))
fi

echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo "Total execution time: ${DURATION}s"

# Performance warnings based on mode
if [ "$MODE" = "quick" ]; then
    if (( $(echo "$DURATION > 5" | bc -l 2>/dev/null || echo 0) )); then
        echo "WARNING: Quick mode took longer than 5 seconds!"
        echo "Consider reviewing test exclusions"
    fi
elif [ "$MODE" = "full" ]; then
    if (( $(echo "$DURATION > 30" | bc -l 2>/dev/null || echo 0) )); then
        echo "WARNING: Full mode took longer than 30 seconds!"
        echo "Consider moving slow tests to performance mode"
    fi
fi

echo ""
echo "Mode Summary:"
echo "- Quick: Fast validation (~2s) - excludes mesh generation tests"
echo "- Full: Standard testing (~13s) - includes all core tests"
echo "- Performance: Complete testing (~20s+) - includes stress tests"

# Exit with test result
if [ $TEST_EXIT_CODE -eq 0 ]; then
    echo ""
    echo "✅ Surface Generation tests completed successfully!"
else
    echo ""
    echo "❌ Surface Generation tests failed with exit code $TEST_EXIT_CODE"
fi

exit $TEST_EXIT_CODE