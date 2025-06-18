#!/bin/bash

# Voxel Data subsystem performance test runner
# Usage: ./run_performance.sh [build_dir]

set -e

# Default build directory
BUILD_DIR="${1:-build_ninja}"

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "Running Voxel Data subsystem performance tests..."
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
TEST_EXECUTABLE="$BUILD_DIR/bin/VoxelEditor_VoxelData_Tests"
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo "Building Voxel Data tests..."
    cmake --build "$BUILD_DIR" --target VoxelEditor_VoxelData_Tests
fi

# Run only performance/memory/stress tests
echo ""
echo "Executing Voxel Data performance tests..."
echo "==========================================="

# Include only performance, memory, and stress tests using gtest filter
"$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" --gtest_filter="*Performance*:*Memory*:*Stress*"

echo ""
echo "Voxel Data performance tests completed!"
echo ""
echo "Performance tests included:"
echo "- MemoryPoolOperations"
echo "- StressTestLargeDataset"
echo "- MemoryUsageTracking"
echo "- MemoryManagement"
echo "- PerformanceMetrics"
echo "- PerformanceTest_CollisionCheck10000Voxels"
echo "- SparseStoragePerformance"
echo "- MemoryOptimization"
echo "- StressTestLargeGrid"
echo "- MemoryUsageScaling"