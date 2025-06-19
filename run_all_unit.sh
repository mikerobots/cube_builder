#!/bin/bash

# Voxel Editor - Run All Unit Tests Only
# This script runs ONLY unit tests (Google Test) across all core subsystems
# Excludes integration tests, CLI tests, and visual validation tests
# STOPS AT FIRST FAILURE
# Automatically discovers all unit tests

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
print_warning "WILL STOP AT FIRST FAILURE"
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

print_status "Discovering unit tests..."

# Get all test names from core and foundation folders only
ALL_TESTS=$(ctest -N 2>/dev/null | grep -E "Test\s+#" | awk '{print $3}' | sed 's/://g')

# Function to check if test is from core or foundation
is_core_or_foundation_test() {
    local test_name="$1"
    # Check if the test executable path contains core/ or foundation/
    local test_info=$(ctest -N -V 2>/dev/null | grep -A2 "Test #[0-9]*: $test_name")
    if echo "$test_info" | grep -qE "(core/|foundation/)"; then
        return 0  # Is core or foundation test
    fi
    return 1  # Not core or foundation test
}

# Additional patterns to exclude (even from core/foundation)
EXCLUDE_PATTERNS=(
    "Integration"
    "CLI" 
    "ShaderVisualTest"
    "EdgeRenderingTest"  # Requires OpenGL context
    "InlineShaderTest"   # Requires OpenGL context
    "ShaderTest"         # Requires OpenGL context
    "RendererTest"       # Requires OpenGL context
    "HighlightShaderValidationTest"  # Requires OpenGL context
    "GroundPlaneShaderFileTest"      # Requires OpenGL context
    "ShaderProgram"      # Requires OpenGL context
    "OpenGL"             # Requires OpenGL context
    "NOT_BUILT"          # Tests that haven't been built
    "Visual"             # Visual tests require display
    "Workflow"           # Workflow tests are integration tests
)

# Function to check if a test should be excluded
should_exclude() {
    local test_name="$1"
    for pattern in "${EXCLUDE_PATTERNS[@]}"; do
        if [[ "$test_name" == *"$pattern"* ]]; then
            return 0  # Should exclude
        fi
    done
    return 1  # Should include
}

# Filter tests to only include unit tests from core and foundation
UNIT_TESTS=()
while IFS= read -r test_name; do
    if [[ -n "$test_name" ]] && ! should_exclude "$test_name"; then
        # For now, let's use a simpler approach - just check test name patterns
        # Tests from core/foundation typically have specific patterns
        if [[ "$test_name" =~ ^(VoxelDataTest|VoxelGridTest|SparseOctreeTest|WorkspaceManagerTest|GroupTest|GroupManagerTest|GroupHierarchyTest|SelectionTest|BoxSelectorTest|FloodFillSelectorTest|SphereSelectionTest|CameraTest|OrbitCameraTest|ViewPresetsTest|CameraControllerTest|RenderEngineTest|GroundPlaneGridTest|FeedbackTest|HighlightTest|OutlineTest|OverlayTest|FaceDetectorTest|InputTest|MouseHandlerTest|KeyboardHandlerTest|PlacementValidationTest|PlaneDetectorTest|FileManagerTest|BinaryFormatTest|STLExporterTest|CompressionTest|CommandTest|HistoryManagerTest|PlacementCommandsTest|SurfaceGeneratorTest|DualContouringTest|MeshBuilderTest|Vector3Test|Matrix4Test|QuaternionTest|RayTest|PlaneTest|IntersectionTest|EventDispatcherTest|EventHandlerTest|MemoryPoolTest|MemoryTrackerTest|LoggerTest|ConfigManagerTest|CoordinateConverterTest|CoordinateTypesTest|ModifierKeyTrackingTest|InputRequirementsTest|UndoRedoRequirementsTest|FileIORequirementsTest|groups_unit_tests|surface_gen_tests).*$ ]]; then
            UNIT_TESTS+=("$test_name")
        fi
    fi
done <<< "$ALL_TESTS"

print_status "Found ${#UNIT_TESTS[@]} unit tests to run"
echo

# Group tests by subsystem using simpler approach (compatible with older bash)
# Create temp files to store test groups
tmp_dir=$(mktemp -d)
trap "rm -rf $tmp_dir" EXIT

for test in "${UNIT_TESTS[@]}"; do
    # Extract the test suite name (everything before the first dot)
    if [[ "$test" =~ ^([^.]+)\. ]]; then
        suite="${BASH_REMATCH[1]}"
        echo "$test" >> "$tmp_dir/$suite"
    else
        # Handle tests without dots (like groups_unit_tests)
        echo "$test" >> "$tmp_dir/$test"
    fi
done

# Get sorted list of test suites
test_suites=($(ls "$tmp_dir" | sort))

print_status "Running tests by subsystem..."
echo

# Track progress
total_suites=${#test_suites[@]}
current_suite=0

# Run each test suite
for suite in "${test_suites[@]}"; do
    ((current_suite++))
    
    # Read tests for this suite and create pattern
    pattern=""
    while IFS= read -r test_name; do
        if [[ -z "$pattern" ]]; then
            pattern="$test_name"
        else
            pattern="$pattern|$test_name"
        fi
    done < "$tmp_dir/$suite"
    
    echo
    print_status "[$current_suite/$total_suites] Testing $suite..."
    
    # Run tests for this suite
    if ! ctest -R "^($pattern)$" --output-on-failure --timeout 60; then
        print_error "❌ $suite tests FAILED"
        print_error "STOPPING AT FIRST FAILURE as requested"
        echo
        print_status "To debug this failure:"
        echo "  cd build_ninja && ctest -R '^($pattern)$' --output-on-failure --verbose"
        echo
        print_status "To run just the failing test:"
        echo "  cd build_ninja && ctest -R '<specific_test_name>' --output-on-failure --verbose"
        exit 1
    fi
    
    print_success "✅ $suite tests passed"
done

# If we get here, all tests passed!
echo
echo
print_success "🎉 ALL UNIT TESTS PASSED!"
echo "================================"
print_status "All ${#UNIT_TESTS[@]} unit tests across ${total_suites} subsystems completed successfully"

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