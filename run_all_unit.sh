#!/bin/bash

# Voxel Editor - Run All Unit Tests Only
# This script runs ONLY unit tests (Google Test) across all core subsystems
# Excludes integration tests, CLI tests, and visual validation tests

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Check if we're in the right directory
if [[ ! -f "CMakeLists.txt" ]] || [[ ! -f "ARCHITECTURE.md" ]]; then
    print_error "This script must be run from the project root directory"
    exit 1
fi

print_status "Starting Voxel Editor Unit Test Suite (Unit Tests Only)"
echo "========================================================="

# Check if build directory exists
if [[ ! -d "build_ninja" ]]; then
    print_error "Build directory 'build_ninja' not found"
    print_status "Please run: cmake -B build_ninja -G Ninja"
    exit 1
fi

# Check if build directory has been configured
if [[ ! -f "build_ninja/build.ninja" ]]; then
    print_error "Build directory not properly configured"
    print_status "Please run: cmake -B build_ninja -G Ninja"
    exit 1
fi

# Change to build directory
cd build_ninja

print_status "Running unit tests by subsystem..."
echo

# Track overall results
FOUNDATION_FAILED=0
CORE_FAILED=0
TOTAL_FOUNDATION=0
TOTAL_CORE=0

# Foundation Layer Tests
echo
print_status "🏗️  FOUNDATION LAYER TESTS"
echo "=================================="

# Foundation subsystems
FOUNDATION_SYSTEMS=("Math" "Memory" "Events" "Config" "Logging")

for system in "${FOUNDATION_SYSTEMS[@]}"; do
    echo
    print_status "Testing Foundation/${system}..."
    
    # Convert to test name pattern
    case $system in
        "Math") pattern="Matrix4fTest|Vector3fTest" ;;
        "Memory") pattern="MemoryPoolTest|MemoryTrackerTest|MemoryOptimizerTest" ;;
        "Events") pattern="EventDispatcherTest" ;;
        "Config") pattern="ConfigManagerTest|ConfigSectionTest|ConfigValueTest" ;;
        "Logging") pattern="LoggerTest|PerformanceProfilerTest" ;;
    esac
    
    if ctest -R "$pattern" --output-on-failure --timeout 60; then
        print_success "✅ Foundation/${system} tests passed"
    else
        print_error "❌ Foundation/${system} tests failed"
        FOUNDATION_FAILED=$((FOUNDATION_FAILED + 1))
    fi
    TOTAL_FOUNDATION=$((TOTAL_FOUNDATION + 1))
done

# Core Library Tests
echo
echo
print_status "⚙️  CORE LIBRARY TESTS"
echo "========================"

# Core subsystems with their test patterns
CORE_SYSTEMS=(
    "Camera:CameraTest|OrbitCameraTest|ViewportTest|CameraControllerTest|ZoomFunctionalityTest|SetDistanceTest"
    "VoxelData:VoxelTypesTest|SparseOctreeTest|VoxelGridTest|WorkspaceManagerTest|VoxelDataManagerTest|CollisionSimple|VoxelDataRequirementsTest"
    "Selection:SelectionTypesTest|SelectionSetTest|SelectionManagerTest|BoxSelectorTest|SphereSelectorTest|FloodFillSelectorTest"
    "Input:InputTypesTest|InputMappingTest|MouseHandlerTest|KeyboardHandlerTest|TouchHandlerTest|VRInputHandlerTest|ModifierKeyTrackingTest|PlacementValidationTest|SmartSnappingTest|PlaneDetectorTest"
    "Groups:groups_unit_tests"
    "FileIO:file_io_unit_tests"
    "SurfaceGen:test_surface_gen"
    "UndoRedo:test_undo_redo|test_simple_command|test_history_manager"
    "Rendering:RenderTypesTest|RenderConfigTest|RenderSettingsTest|RenderStatsTest|RenderTimerTest|RenderStateTest"
    "VisualFeedback:FeedbackTest|HighlightTest|OutlineTest|OverlayTest"
)

for entry in "${CORE_SYSTEMS[@]}"; do
    system="${entry%%:*}"
    pattern="${entry#*:}"
    
    echo
    print_status "Testing Core/${system}..."
    
    if ctest -R "$pattern" --output-on-failure --timeout 60; then
        print_success "✅ Core/${system} tests passed"
    else
        print_error "❌ Core/${system} tests failed"
        CORE_FAILED=$((CORE_FAILED + 1))
    fi
    TOTAL_CORE=$((TOTAL_CORE + 1))
done

# Final Results
echo
echo
print_status "📊 FINAL RESULTS"
echo "=================="

if [[ $FOUNDATION_FAILED -eq 0 ]] && [[ $CORE_FAILED -eq 0 ]]; then
    print_success "🎉 All unit tests passed!"
    echo
    print_status "Foundation Layer: ${TOTAL_FOUNDATION}/${TOTAL_FOUNDATION} subsystems passed"
    print_status "Core Library: ${TOTAL_CORE}/${TOTAL_CORE} subsystems passed"
    
    echo
    print_success "Unit test suite completed successfully!"
    
else
    echo
    print_error "Some unit tests failed!"
    echo
    print_status "Foundation Layer: $((TOTAL_FOUNDATION - FOUNDATION_FAILED))/${TOTAL_FOUNDATION} subsystems passed"
    print_status "Core Library: $((TOTAL_CORE - CORE_FAILED))/${TOTAL_CORE} subsystems passed"
    echo
    print_status "Failed subsystems:"
    if [[ $FOUNDATION_FAILED -gt 0 ]]; then
        echo "  Foundation: ${FOUNDATION_FAILED} subsystem(s)"
    fi
    if [[ $CORE_FAILED -gt 0 ]]; then
        echo "  Core: ${CORE_FAILED} subsystem(s)"
    fi
    
    echo
    print_status "To debug failures:"
    echo "  cd build_ninja && ctest -R '<pattern>' --output-on-failure --verbose"
    
    exit 1
fi

# Return to project root
cd ..

echo
print_status "This script ran UNIT TESTS ONLY. For other test types:"
echo "  ./run_integration_tests.sh all                      # All tests (unit + integration + CLI)"
echo "  ./run_integration_tests.sh core                     # Core integration tests"
echo "  cd tests/cli_validation && ./run_all_tests.sh       # Visual validation tests"
echo "  cd tests/cli_comprehensive && ./run_all_tests.sh    # CLI workflow tests"
echo "  See tests.md for complete testing documentation"

exit 0