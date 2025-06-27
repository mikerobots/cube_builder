#!/bin/bash

# Unit Test Runner - Auto-Discovery Version
# Automatically discovers unit tests using naming convention: test_unit_*

set -uo pipefail

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Test results
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

# Function to run a unit test executable
run_unit_test() {
    local test_executable=$1
    local test_name=$(basename "$test_executable")
    
    if [ ! -f "$test_executable" ]; then
        print_result "$test_name" "SKIP" "0"
        return
    fi
    
    local start_time=$(date +%s)
    
    if timeout 60 "$test_executable" > "/tmp/${test_name}.log" 2>&1; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        print_result "$test_name" "PASS" "$duration"
    else
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        print_result "$test_name" "FAIL" "$duration"
        echo "  Error output:"
        tail -n 10 "/tmp/${test_name}.log" | sed 's/^/    /'
        
        # Continue to run all tests instead of stopping at first failure
        # Save full log for later analysis
        echo "  Full log saved to: /tmp/${test_name}.log"
    fi
}

# Auto-discover test executables by pattern
discover_unit_tests() {
    find build_ninja/bin -name "test_unit_*" -type f -perm +111 2>/dev/null | sort
}

# Discover all test source files (for listing even when not built)
discover_test_sources() {
    # Search for unit test source files
    find . -name "test_unit_*.cpp" -type f 2>/dev/null | sort
}

# Extract subsystem from test name
get_test_subsystem() {
    local test_name=$(basename "$1")
    # Pattern: test_unit_<subsystem>_<component>_<description>
    echo "$test_name" | cut -d'_' -f3
}

# Discover all subsystems
discover_subsystems() {
    {
        # From built executables
        while IFS= read -r test_path; do
            get_test_subsystem "$test_path"
        done < <(discover_unit_tests)
        
        # From source files (in case not built yet)
        while IFS= read -r test_path; do
            get_test_subsystem "$test_path"
        done < <(discover_test_sources)
    } | sort -u | grep -v "^$"
}

# Run tests by subsystem
run_subsystem_tests() {
    local subsystem=$1
    print_header "$(echo "$subsystem" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}') Unit Tests"
    
    local found_tests=false
    while IFS= read -r test_path; do
        local test_subsystem=$(get_test_subsystem "$test_path")
        if [[ "$test_subsystem" == "$subsystem" ]]; then
            found_tests=true
            run_unit_test "$test_path"
        fi
    done < <(discover_unit_tests)
    
    if [[ "$found_tests" == "false" ]]; then
        print_color "$YELLOW" "No tests found for subsystem: $subsystem"
    fi
}

# List available groups (dynamic)
list_groups() {
    print_header "Available Unit Tests"
    
    # Show unit test counts by subsystem
    echo "Unit Test Subsystems:"
    local found_subsystems=false
    
    while IFS= read -r subsystem; do
        if [ -n "$subsystem" ]; then
            found_subsystems=true
            local capitalized_subsystem=$(echo "$subsystem" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')
            
            # Count unit test files for this subsystem
            local source_count=$(discover_test_sources 2>/dev/null | grep "test_unit_${subsystem}_" | wc -l | tr -d ' ')
            source_count=${source_count:-0}
            
            printf "  ${BOLD}%-15s${NC} - ${GREEN}%d test files${NC}\n" "$capitalized_subsystem" "$source_count"
        fi
    done < <(discover_subsystems)
    
    if [ "$found_subsystems" = "false" ]; then
        echo "  No unit test subsystems found."
        echo "  Looking for files matching pattern: test_unit_<subsystem>_<description>.cpp"
    fi
    
    echo
    echo "Commands:"
    printf "  ${BOLD}Run Unit Tests:${NC}\n"
    printf "    $0 foundation            - Run foundation unit tests\n"
    printf "    $0 core                  - Run core unit tests\n"
    printf "    $0 all                   - Run all unit tests\n"
    echo
    printf "  ${BOLD}Information:${NC}\n"
    printf "    $0 list                  - Show all unit test files\n"
    echo
    printf "  ${BOLD}Note:${NC} Tests are built automatically when you run them\n"
}

# Function to print test summary
print_summary() {
    print_header "Unit Test Summary"
    
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
        echo
        print_color "$CYAN" "To debug failed tests:"
        echo "  1. Check the full logs in /tmp/"
        echo "  2. Run individual tests with filters:"
        echo "     cd build_ninja && ./bin/test_unit_<subsystem>_<name> --gtest_filter='*SpecificTest*'"
        echo "  3. List all tests in an executable:"
        echo "     cd build_ninja && ./bin/test_unit_<subsystem>_<name> --gtest_list_tests"
    fi
    
    echo
    if [ $FAILED_TESTS -eq 0 ]; then
        print_color "$GREEN" "All unit tests passed! 🎉"
        return 0
    else
        print_color "$RED" "Some unit tests failed. Please check the logs."
        return 1
    fi
}

# Main script logic
main() {
    # If no arguments, show help
    if [ $# -eq 0 ]; then
        list_groups
        exit 0
    fi
    
    # Process arguments
    for group in "$@"; do
        case "$group" in
            list|help|-h|--help)
                list_groups
                exit 0
                ;;
            all)
                # Run all discovered unit tests
                print_header "All Unit Tests"
                if [ -d "build_ninja" ]; then
                    print_color "$CYAN" "Building unit tests..."
                    # Build only unit test targets
                    while IFS= read -r test_source; do
                        local test_name=$(basename "$test_source" .cpp)
                        cmake --build build_ninja --target "$test_name" 2>/dev/null || true
                    done < <(discover_test_sources | grep "test_unit_")
                fi
                
                while IFS= read -r test_path; do
                    run_unit_test "$test_path"
                done < <(discover_unit_tests)
                ;;
            *)
                # Try to run as a subsystem
                if discover_subsystems | grep -q "^${group}$"; then
                    # Build tests for this subsystem if needed
                    if [ -d "build_ninja" ]; then
                        print_color "$CYAN" "Building ${group} unit tests..."
                        # Build only the specific subsystem tests
                        local test_targets=()
                        while IFS= read -r test_source; do
                            if [[ "$test_source" =~ test_unit_${group}_ ]]; then
                                local test_name=$(basename "$test_source" .cpp)
                                test_targets+=("$test_name")
                            fi
                        done < <(discover_test_sources)
                        
                        # Build each target individually to avoid building broken tests
                        for target in "${test_targets[@]}"; do
                            cmake --build build_ninja --target "$target" 2>/dev/null || true
                        done
                    fi
                    run_subsystem_tests "$group"
                else
                    print_color "$RED" "Unknown subsystem: $group"
                    echo "Available subsystems:"
                    discover_subsystems | sed 's/^/  /'
                    echo
                    echo "Use '$0 list' to see all available groups"
                    exit 1
                fi
                ;;
        esac
    done
    
    # Print summary
    print_summary
}

# Run main function
main "$@"