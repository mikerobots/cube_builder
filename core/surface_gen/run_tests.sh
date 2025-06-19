#!/bin/bash

# Surface Generation subsystem test runner with standardized options
# Usage: ./run_tests.sh [build_dir] [--quick|--full|--slow|--help]
#
# Test execution modes:
#   --quick  : Run all tests under 1 second (default) - 55 fast tests
#   --full   : Run all tests under 5 seconds - 57 tests (adds 2 moderate tests)
#   --slow   : Run all tests over 5 seconds - ALL 72 tests (adds 15 slow tests)
#
# Expected execution times:
#   --quick mode: ~0.2s total (55 fast tests)
#   --full mode:  ~6s total (57 tests, includes 2 moderate mesh generation tests)
#   --slow mode:  ~14s total (ALL 72 tests, includes all SurfaceGenerator tests)
#
# Test Categories:
#   Quick Tests (55): Fast unit tests and basic functionality
#     - MeshTest (7 tests): Basic mesh operations (0ms each)
#     - SurfaceSettingsTest (7 tests): Settings validation (0ms each)  
#     - MeshGenerationEventTest (1 test): Event construction (0ms)
#     - SimplificationSettingsTest (1 test): Settings presets (0ms)
#     - MeshCacheEntryTest (1 test): Cache entry operations (11ms)
#     - LODLevelTest (1 test): LOD conversion (0ms)
#     - ExportQualityTest (1 test): Quality values (0ms)
#     - MeshBuilderTest (10 tests): Mesh building operations (0ms each)
#     - MeshUtilsTest (2 tests): Utility calculations (0ms each)
#     - MeshCacheTest.BasicCaching: Basic cache operations (0ms)
#     - MeshCacheTest.RegionInvalidation: Cache invalidation (0ms)
#     - LODManagerTest (2 tests): LOD management (0ms each)
#     - DualContouringTest (10 tests): Core algorithm tests (2-8ms each)
#     - SurfaceGenRequirementsTest (9 tests): Fast requirement tests (50-119ms each)
#
#   Full Tests (2): Moderate mesh generation tests  
#     - SurfaceGeneratorTest.ExportMeshGeneration: Export quality mesh (2.1s)
#     - SurfaceGeneratorTest.PreviewMeshGeneration: Preview mesh generation (3.7s)
#
#   Slow Tests (15): Heavy mesh generation and performance tests
#     - SurfaceGeneratorTest.BasicGeneration: Basic mesh generation (526ms)
#     - SurfaceGeneratorTest.CustomSettings: Custom settings test (570ms)
#     - SurfaceGeneratorTest.EmptyGrid: Empty grid handling (334ms)
#     - SurfaceGeneratorTest.SingleVoxel: Single voxel mesh (532ms)
#     - SurfaceGeneratorTest.CacheEnabled: Cache enabled test (557ms)
#     - SurfaceGeneratorTest.CacheDisabled: Cache disabled test (1032ms)
#     - SurfaceGeneratorTest.AsyncGeneration: Async generation (524ms)
#     - SurfaceGeneratorTest.MultipleAsyncGenerations: Multiple async (568ms)
#     - SurfaceGeneratorTest.ProgressCallback: Progress callback (508ms)
#     - SurfaceGeneratorTest.VoxelDataChanged: Data change handling (512ms)
#     - SurfaceGeneratorTest.CacheMemoryLimit: Cache memory test (564ms)
#     - SurfaceGeneratorTest.ClearCache: Cache clearing (508ms)
#     - MeshCacheTest.LRUEviction: LRU cache eviction (60ms)
#     - SurfaceGenRequirementsTest.MultiResolutionLOD: Multi-res LOD (341ms)
#     - SurfaceGenRequirementsTest.HighQualityExport: High quality export (267ms)

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
            echo "Surface Generation subsystem test runner"
            echo ""
            echo "Usage: $0 [build_dir] [--quick|--full|--slow|--help]"
            echo ""
            echo "Test modes:"
            echo "  --quick  Run fast tests (<1s) - DEFAULT - 55 tests in ~0.2s"
            echo "  --full   Run moderate tests (<5s) - 57 tests in ~6s"
            echo "  --slow   Run all tests (>5s) - ALL 72 tests in ~14s"
            echo "  --help   Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                          # Run quick tests with build_ninja"
            echo "  $0 build_debug             # Run quick tests with build_debug"
            echo "  $0 --full                  # Run full test suite"
            echo "  $0 build_ninja --slow      # Run all tests including slow ones"
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

echo -e "${GREEN}Surface Generation Subsystem Test Runner${NC}"
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
TEST_EXECUTABLE="$BUILD_DIR/bin/test_surface_gen"

# Check if test executable exists, build if needed
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo -e "${YELLOW}Building Surface Generation tests...${NC}"
    if ! time_command cmake --build "$BUILD_DIR" --target test_surface_gen; then
        echo -e "${RED}Error: Failed to build Surface Generation tests${NC}"
        exit 1
    fi
fi

# Verify executable exists after build
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo -e "${RED}Error: Test executable not found: $TEST_EXECUTABLE${NC}"
    exit 1
fi

# Define test filters based on mode
GTEST_FILTER=""
TIMEOUT_DURATION=""

case $TEST_MODE in
    quick)
        echo -e "${GREEN}Running QUICK tests (<1s): 55 fast tests${NC}"
        echo "=========================================="
        echo "Test Categories: Basic mesh operations, settings, cache operations,"
        echo "                dual contouring algorithm, fast requirement tests"
        echo ""
        
        # Exclude slow SurfaceGeneratorTest.* tests and the 2 moderate tests
        GTEST_FILTER="--gtest_filter=-SurfaceGeneratorTest.*:SurfaceGenRequirementsTest.MultiResolutionLOD:SurfaceGenRequirementsTest.HighQualityExport:MeshCacheTest.LRUEviction"
        TIMEOUT_DURATION="60"
        
        # Run tests with 60s timeout
        if ! /opt/homebrew/bin/timeout $TIMEOUT_DURATION "$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" \
            --gtest_brief=1 \
            --gtest_print_time=1 \
            $GTEST_FILTER; then
            echo -e "${RED}Error: Quick tests failed or timed out${NC}"
            exit 1
        fi
        ;;
        
    full)
        echo -e "${GREEN}Running FULL tests (<5s): 57 tests (adds 2 moderate tests)${NC}"
        echo "================================================="
        echo "Test Categories: All quick tests + 2 moderate mesh generation tests"
        echo ""
        
        # Exclude only the slowest SurfaceGeneratorTest.* tests (keep the 2 moderate ones)
        GTEST_FILTER="--gtest_filter=-SurfaceGeneratorTest.BasicGeneration:SurfaceGeneratorTest.CustomSettings:SurfaceGeneratorTest.EmptyGrid:SurfaceGeneratorTest.SingleVoxel:SurfaceGeneratorTest.CacheEnabled:SurfaceGeneratorTest.CacheDisabled:SurfaceGeneratorTest.AsyncGeneration:SurfaceGeneratorTest.MultipleAsyncGenerations:SurfaceGeneratorTest.ProgressCallback:SurfaceGeneratorTest.VoxelDataChanged:SurfaceGeneratorTest.CacheMemoryLimit:SurfaceGeneratorTest.ClearCache:MeshCacheTest.LRUEviction:SurfaceGenRequirementsTest.MultiResolutionLOD:SurfaceGenRequirementsTest.HighQualityExport"
        TIMEOUT_DURATION="300"
        
        # Run tests with 300s timeout
        if ! /opt/homebrew/bin/timeout $TIMEOUT_DURATION "$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" \
            --gtest_brief=1 \
            --gtest_print_time=1 \
            $GTEST_FILTER; then
            echo -e "${RED}Error: Full tests failed or timed out${NC}"
            exit 1
        fi
        ;;
        
    slow)
        echo -e "${GREEN}Running SLOW tests (all): ALL 72 Surface Generation tests${NC}"
        echo "======================================================="
        echo "Test Categories: All tests including heavy mesh generation and performance tests"
        echo ""
        
        # Run all tests without filter
        GTEST_FILTER=""
        TIMEOUT_DURATION="600"
        
        # Run tests with 600s timeout (10 minutes for all tests)
        if ! /opt/homebrew/bin/timeout $TIMEOUT_DURATION "$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" \
            --gtest_brief=1 \
            --gtest_print_time=1; then
            echo -e "${RED}Error: Slow tests failed or timed out${NC}"
            exit 1
        fi
        ;;
esac

# Count total tests run
TEST_COUNT=$("$PROJECT_ROOT/execute_command.sh" "$TEST_EXECUTABLE" --gtest_list_tests 2>/dev/null | grep -c "^  " || echo "72")

echo ""
echo -e "${GREEN}Surface Generation tests completed successfully!${NC}"
echo "Tests run: varies by mode ($TEST_MODE)"
echo "Mode: $TEST_MODE"

case $TEST_MODE in
    quick)
        echo "Quick mode: 55 fast tests completed - excellent for rapid development"
        ;;
    full)
        echo "Full mode: 57 tests completed - includes moderate mesh generation tests"
        ;;
    slow)
        echo "Slow mode: ALL 72 tests completed - comprehensive test coverage"
        ;;
esac