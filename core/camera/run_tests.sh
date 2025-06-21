#!/bin/bash

# Camera Subsystem Test Runner - Auto-Discovery Version
# Automatically discovers and runs tests based on naming convention

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
SUBSYSTEM_NAME=$(basename "$SCRIPT_DIR")
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Default build directory
BUILD_DIR="${BUILD_DIR:-build_ninja}"

# Test results tracking
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0
TEST_RESULTS=()

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

# Function to print test result
print_result() {
    local test_name=$1
    local status=$2
    local duration=$3
    
    if [ "$status" = "PASS" ]; then
        print_color "$GREEN" "✓ $test_name (${duration}s)"
        ((PASSED_TESTS++))
    elif [ "$status" = "FAIL" ]; then
        print_color "$RED" "✗ $test_name (${duration}s)"
        ((FAILED_TESTS++))
    elif [ "$status" = "SKIP" ]; then
        print_color "$YELLOW" "- $test_name (skipped)"
        ((SKIPPED_TESTS++))
    fi
    
    TEST_RESULTS+=("$test_name|$status|$duration")
}

# Function to discover test source files
discover_test_sources() {
    local pattern="${1:-*}"
    find "$SCRIPT_DIR/tests" -name "test_unit_core_${SUBSYSTEM_NAME}_${pattern}.cpp" -type f 2>/dev/null | sort
}

# Function to discover built test executables
discover_test_executables() {
    local pattern="${1:-*}"
    find "$PROJECT_ROOT/$BUILD_DIR/bin" -name "test_unit_core_${SUBSYSTEM_NAME}_${pattern}" -type f -perm +111 2>/dev/null | sort
}

# Function to extract test component from filename
get_test_component() {
    local filename=$(basename "$1" .cpp)
    # Extract component from pattern: test_unit_core_<subsystem>_<component>
    echo "$filename" | sed "s/test_unit_core_${SUBSYSTEM_NAME}_//"
}

# Function to run a single test executable
run_test() {
    local test_executable=$1
    local test_name=$(basename "$test_executable")
    
    if [ ! -f "$test_executable" ]; then
        print_result "$test_name" "SKIP" "0"
        return
    fi
    
    local start_time=$(date +%s)
    
    if timeout 60 "$PROJECT_ROOT/execute_command.sh" "$test_executable" > "/tmp/${test_name}.log" 2>&1; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        print_result "$test_name" "PASS" "$duration"
    else
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        print_result "$test_name" "FAIL" "$duration"
        echo "  Error output:"
        tail -n 10 "/tmp/${test_name}.log" | sed 's/^/    /'
        
        # Option to stop at first failure
        if [ "${STOP_ON_FAILURE:-1}" -eq 1 ]; then
            print_color "$RED" "STOPPING AT FIRST FAILURE"
            echo
            print_color "$CYAN" "To debug this failure:"
            echo "  cd $PROJECT_ROOT && $test_executable --gtest_list_tests"
            echo "  cd $PROJECT_ROOT && $test_executable --gtest_filter='*SpecificTest*'"
            exit 1
        fi
    fi
}

# Function to build specific test targets
build_tests() {
    local pattern="${1:-*}"
    print_color "$CYAN" "Building ${SUBSYSTEM_NAME} tests matching pattern: ${pattern}..."
    
    # Discover test sources to build
    local test_sources=()
    while IFS= read -r test_source; do
        if [ -f "$test_source" ]; then
            local test_name=$(basename "$test_source" .cpp)
            test_sources+=("$test_name")
        fi
    done < <(discover_test_sources "$pattern")
    
    if [ ${#test_sources[@]} -eq 0 ]; then
        print_color "$YELLOW" "No test sources found matching pattern: test_unit_core_${SUBSYSTEM_NAME}_${pattern}.cpp"
        return 1
    fi
    
    # Build each test target
    cd "$PROJECT_ROOT"
    for target in "${test_sources[@]}"; do
        if ! cmake --build "$BUILD_DIR" --target "$target" 2>/dev/null; then
            print_color "$YELLOW" "Warning: Failed to build $target"
        fi
    done
}

# Function to list available tests
list_tests() {
    local capitalized_name=$(echo "$SUBSYSTEM_NAME" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')
    print_header "$capitalized_name Test Discovery"
    
    echo "Test Source Files:"
    local source_count=0
    while IFS= read -r test_source; do
        if [ -f "$test_source" ]; then
            local component=$(get_test_component "$test_source")
            printf "  ${BOLD}%-30s${NC} - %s\n" "$component" "$(basename "$test_source")"
            ((source_count++))
        fi
    done < <(discover_test_sources)
    
    echo
    echo "Built Test Executables:"
    local exec_count=0
    while IFS= read -r test_exec; do
        if [ -f "$test_exec" ]; then
            local component=$(get_test_component "$test_exec")
            printf "  ${GREEN}%-30s${NC} - %s\n" "$component" "$(basename "$test_exec")"
            ((exec_count++))
        fi
    done < <(discover_test_executables)
    
    echo
    print_color "$CYAN" "Summary:"
    echo "  Source files found: $source_count"
    echo "  Built executables:  $exec_count"
    
    if [ $exec_count -lt $source_count ]; then
        echo
        print_color "$YELLOW" "Note: Some tests are not built. Run with --build to build them."
    fi
}

# Function to print test summary
print_summary() {
    local capitalized_name=$(echo "$SUBSYSTEM_NAME" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')
    print_header "$capitalized_name Test Summary"
    
    local total=$((PASSED_TESTS + FAILED_TESTS + SKIPPED_TESTS))
    
    echo "Total Tests: $total"
    print_color "$GREEN" "  Passed:  $PASSED_TESTS"
    print_color "$RED" "  Failed:  $FAILED_TESTS"
    print_color "$YELLOW" "  Skipped: $SKIPPED_TESTS"
    
    if [ $FAILED_TESTS -gt 0 ]; then
        echo
        print_color "$RED" "Failed Tests:"
        for result in "${TEST_RESULTS[@]}"; do
            IFS='|' read -r name status duration <<< "$result"
            if [ "$status" = "FAIL" ]; then
                echo "  - $name"
            fi
        done
    fi
    
    echo
    if [ $FAILED_TESTS -eq 0 ]; then
        print_color "$GREEN" "All ${SUBSYSTEM_NAME} tests passed! 🎉"
        return 0
    else
        print_color "$RED" "Some ${SUBSYSTEM_NAME} tests failed. Please check the logs."
        return 1
    fi
}

# Function to show usage
show_usage() {
    local capitalized_name=$(echo "$SUBSYSTEM_NAME" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')
    cat << EOF
$capitalized_name Subsystem Test Runner with Auto-Discovery

Usage: $0 [options] [pattern]

Options:
  --list              List all available tests
  --build             Build tests before running
  --no-stop-on-fail   Continue running tests after failure
  --help              Show this help message

Arguments:
  pattern   Optional pattern to filter tests (e.g., "camera", "viewport", "*zoom*")
            Default: "*" (all tests)

Examples:
  $0                     # Run all ${SUBSYSTEM_NAME} tests
  $0 --build             # Build and run all tests
  $0 camera              # Run only camera-related tests
  $0 --list              # List all available tests
  $0 --build viewport    # Build and run viewport tests

Build Directory: ${BUILD_DIR}
Test Pattern: test_unit_core_${SUBSYSTEM_NAME}_<pattern>.cpp

EOF
}

# Main script logic
main() {
    local do_build=0
    local do_list=0
    local pattern="*"
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --help|-h)
                show_usage
                exit 0
                ;;
            --list)
                do_list=1
                shift
                ;;
            --build)
                do_build=1
                shift
                ;;
            --no-stop-on-fail)
                STOP_ON_FAILURE=0
                shift
                ;;
            -*)
                print_color "$RED" "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
            *)
                pattern="$1"
                shift
                ;;
        esac
    done
    
    # Change to project root
    cd "$PROJECT_ROOT"
    
    # Check build directory
    if [ ! -d "$BUILD_DIR" ]; then
        print_color "$RED" "Error: Build directory '$BUILD_DIR' does not exist"
        echo "Please run: cmake -B $BUILD_DIR -G Ninja"
        exit 1
    fi
    
    # List tests if requested
    if [ $do_list -eq 1 ]; then
        list_tests
        exit 0
    fi
    
    # Build tests if requested
    if [ $do_build -eq 1 ]; then
        build_tests "$pattern"
    fi
    
    # Run tests
    local capitalized_name=$(echo "$SUBSYSTEM_NAME" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')
    print_header "Running $capitalized_name Tests (pattern: $pattern)"
    
    # Check if we need to build tests
    local source_tests=$(discover_test_sources "$pattern")
    local built_tests=$(discover_test_executables "$pattern")
    
    # Count number of source files and built executables
    local source_count=0
    local built_count=0
    
    if [ -n "$source_tests" ]; then
        source_count=$(echo "$source_tests" | wc -l | tr -d ' ')
    fi
    
    if [ -n "$built_tests" ]; then
        built_count=$(echo "$built_tests" | wc -l | tr -d ' ')
    fi
    
    # Build if we have source files but missing some executables
    if [ "$source_count" -gt 0 ] && [ "$built_count" -lt "$source_count" ]; then
        print_color "$YELLOW" "Found $source_count test sources but only $built_count built. Building all tests..."
        build_tests "$pattern"
        # Re-check for built tests
        built_tests=$(discover_test_executables "$pattern")
        if [ -z "$built_tests" ]; then
            print_color "$RED" "Failed to build tests"
            exit 1
        fi
    fi
    
    # Run the tests
    local found_tests=false
    while IFS= read -r test_exec; do
        if [ -f "$test_exec" ]; then
            found_tests=true
            run_test "$test_exec"
        fi
    done < <(echo "$built_tests")
    
    if [ "$found_tests" = "false" ]; then
        print_color "$RED" "No tests found matching pattern: test_unit_core_${SUBSYSTEM_NAME}_${pattern}"
        exit 1
    fi
    
    # Print summary
    print_summary
}

# Run main function
main "$@"