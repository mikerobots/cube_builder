#!/bin/bash

# Unified Test Runner - Comprehensive Test Discovery and Execution
# Supports all test types: unit, integration, e2e, performance, stress
# Auto-discovers tests by naming convention: test_<type>_<subsystem>_<component>_<description>

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
    elif [ "$status" = "PASS*" ]; then
        print_color "$GREEN" "✓ $test_name (${duration}s) *"
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

# Function to run a test executable
run_test() {
    local test_executable=$1
    local test_name=$(basename "$test_executable")
    local timeout_duration=120
    
    # Set timeout based on test type
    if [[ "$test_name" == *"performance"* ]] || [[ "$test_name" == *"stress"* ]]; then
        timeout_duration=300
    elif [[ "$test_name" == *"e2e"* ]]; then
        timeout_duration=180
    fi
    
    if [ ! -f "$test_executable" ]; then
        print_result "$test_name" "SKIP" "0"
        return
    fi
    
    local start_time=$(date +%s)
    
    if timeout $timeout_duration "$test_executable" > "/tmp/${test_name}.log" 2>&1; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        print_result "$test_name" "PASS" "$duration"
    else
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        print_result "$test_name" "FAIL" "$duration"
        echo "  Error output:"
        tail -n 10 "/tmp/${test_name}.log" | sed 's/^/    /'
    fi
}

# Test Discovery Functions

# Discover tests by type and pattern
discover_tests_by_type() {
    local test_type=$1
    find build_ninja/bin -name "test_${test_type}_*" -type f -perm +111 2>/dev/null | sort
}

# Discover test source files by type
discover_test_sources_by_type() {
    local test_type=$1
    find . -name "test_${test_type}_*.cpp" -type f 2>/dev/null | sort
}

# Discover all test types
discover_test_types() {
    {
        # From built executables
        find build_ninja/bin -name "test_*" -type f -perm +111 2>/dev/null | while read test; do
            basename "$test" | cut -d'_' -f2
        done
        
        # From source files
        find . -name "test_*.cpp" -type f 2>/dev/null | while read test; do
            basename "$test" | cut -d'_' -f2
        done
    } | sort -u | grep -E "^(unit|integration|e2e|performance|stress)$"
}

# Extract subsystem from test name
get_test_subsystem() {
    local test_name=$(basename "$1")
    # Pattern: test_<type>_<subsystem>_<component>_<description>
    echo "$test_name" | cut -d'_' -f3
}

# Discover subsystems for a test type
discover_subsystems_for_type() {
    local test_type=$1
    {
        # From built executables
        while IFS= read -r test_path; do
            get_test_subsystem "$test_path"
        done < <(discover_tests_by_type "$test_type")
        
        # From source files
        while IFS= read -r test_path; do
            get_test_subsystem "$test_path"
        done < <(discover_test_sources_by_type "$test_type")
    } | sort -u | grep -v "^$"
}

# Discover uncategorized/legacy tests
discover_uncategorized_tests() {
    # Find test executables that don't follow the new naming convention
    find build_ninja/bin -type f -executable 2>/dev/null | \
        grep -E "(Test|test)" | \
        grep -v -E "test_(unit|integration|e2e|performance|stress)_" | \
        sort
}

# Discover uncategorized test sources
discover_uncategorized_test_sources() {
    # Find test source files that don't follow the new naming convention
    # Only look in test directories and exclude special frameworks
    find . -path "*/tests/*" -name "*.cpp" -o -path "*/test/*" -name "*.cpp" | \
        grep -E "(Test|test)" | \
        grep -v -E "test_(unit|integration|e2e|performance|stress)_" | \
        grep -v "/build" | \
        grep -v "apps/shader_test" | \
        sort
}

# Run tests by type
run_tests_by_type() {
    local test_type=$1
    local capitalized_type=$(echo "$test_type" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')
    print_header "${capitalized_type} Tests"
    
    # Build tests for this type if needed
    if [ -d "build_ninja" ]; then
        print_color "$CYAN" "Building ${test_type} tests..."
        cmake --build build_ninja --parallel $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) 2>/dev/null || true
    fi
    
    local found_tests=false
    while IFS= read -r test_path; do
        if [ -n "$test_path" ]; then
            found_tests=true
            run_test "$test_path"
        fi
    done < <(discover_tests_by_type "$test_type")
    
    if [ "$found_tests" = "false" ]; then
        print_color "$YELLOW" "No ${test_type} tests found"
    fi
}

# Run tests by type and subsystem
run_tests_by_type_and_subsystem() {
    local test_type=$1
    local subsystem=$2
    local capitalized_type=$(echo "$test_type" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')
    local capitalized_subsystem=$(echo "$subsystem" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')
    print_header "${capitalized_type} Tests - ${capitalized_subsystem} Subsystem"
    
    local found_tests=false
    while IFS= read -r test_path; do
        local test_subsystem=$(get_test_subsystem "$test_path")
        if [[ "$test_subsystem" == "$subsystem" ]]; then
            found_tests=true
            run_test "$test_path"
        fi
    done < <(discover_tests_by_type "$test_type")
    
    if [ "$found_tests" = "false" ]; then
        print_color "$YELLOW" "No ${test_type} tests found for subsystem: $subsystem"
    fi
}

# Run uncategorized tests
run_uncategorized_tests() {
    print_header "Uncategorized/Legacy Tests"
    
    local found_tests=false
    
    # Run discovered uncategorized test executables
    while IFS= read -r test; do
        if [[ -n "$test" && -x "$test" ]]; then
            found_tests=true
            run_test "$test"
        fi
    done < <(discover_uncategorized_tests)
    
    if [ "$found_tests" = "false" ]; then
        print_color "$YELLOW" "No uncategorized tests found"
    fi
}

# List all available test groups
list_test_groups() {
    print_header "Available Test Groups"
    
    # Discover test types and show actual files
    echo "Available Test Files:"
    local found_types=false
    while IFS= read -r test_type; do
        if [ -n "$test_type" ]; then
            found_types=true
            local capitalized_type=$(echo "$test_type" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')
            printf "\n  ${BOLD}${capitalized_type} Tests:${NC}\n"
            
            # Show subsystems and their files
            while IFS= read -r subsystem; do
                if [ -n "$subsystem" ]; then
                    local capitalized_subsystem=$(echo "$subsystem" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')
                    printf "    ${CYAN}${capitalized_subsystem}:${NC}\n"
                    
                    # Show actual test files for this subsystem
                    while IFS= read -r test_file; do
                        if [ -n "$test_file" ]; then
                            local file_subsystem=$(get_test_subsystem "$test_file")
                            if [[ "$file_subsystem" == "$subsystem" ]]; then
                                local basename_file=$(basename "$test_file")
                                printf "      ${GREEN}%-60s${NC}\n" "$basename_file"
                            fi
                        fi
                    done < <(discover_test_sources_by_type "$test_type")
                fi
            done < <(discover_subsystems_for_type "$test_type")
        fi
    done < <(discover_test_types)
    
    if [ "$found_types" = "false" ]; then
        echo "No tests following new naming convention found."
        echo "Pattern: test_<type>_<subsystem>_<component>_<description>.cpp"
    fi
    
    echo
    
    # Show uncategorized tests with file names (sample)
    echo "Legacy Tests (not following naming convention):"
    local uncategorized_source_count=$(discover_uncategorized_test_sources | wc -l | tr -d ' ')
    
    if [ "$uncategorized_source_count" -gt 0 ]; then
        printf "  ${YELLOW}Found %d legacy test files${NC} (showing first 10):\n" "$uncategorized_source_count"
        discover_uncategorized_test_sources | head -10 | while IFS= read -r test_file; do
            if [ -n "$test_file" ]; then
                local basename_file=$(basename "$test_file")
                printf "    ${YELLOW}%-60s${NC}\n" "$basename_file"
            fi
        done
        if [ "$uncategorized_source_count" -gt 10 ]; then
            printf "    ${YELLOW}... and %d more${NC}\n" $((uncategorized_source_count - 10))
        fi
    else
        echo "  None found - all tests follow naming convention! 🎉"
    fi
    
    echo
    echo "Commands:"
    printf "  ${BOLD}Run Tests:${NC}\n"
    printf "    $0 unit foundation    - Build & run unit tests for foundation\n"
    printf "    $0 integration core   - Build & run core integration tests\n"
    printf "    $0 integration        - Build & run all integration tests\n"
    printf "    $0 all                - Build & run all tests\n"
    echo
    printf "  ${BOLD}Information:${NC}\n"
    printf "    $0 list               - Show all available test files\n"
    echo
    printf "  ${BOLD}Note:${NC} Tests are built automatically when you run them\n"
}

# Function to print test summary
print_summary() {
    print_header "Test Summary"
    
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
        print_color "$GREEN" "All tests passed! 🎉"
        return 0
    else
        print_color "$RED" "Some tests failed. Please check the logs."
        return 1
    fi
}

# Main script logic
main() {
    # If no arguments, show help
    if [ $# -eq 0 ]; then
        list_test_groups
        exit 0
    fi
    
    # Process arguments
    local test_type=$1
    local subsystem=${2:-}
    
    case "$test_type" in
        list|help|-h|--help)
            list_test_groups
            exit 0
            ;;
        all)
            # Run all test types
            for type in unit integration e2e performance stress; do
                if [ "$(discover_tests_by_type "$type" | wc -l)" -gt 0 ]; then
                    run_tests_by_type "$type"
                fi
            done
            run_uncategorized_tests
            ;;
        unit|integration|e2e|performance|stress)
            if [ -n "$subsystem" ]; then
                run_tests_by_type_and_subsystem "$test_type" "$subsystem"
            else
                run_tests_by_type "$test_type"
            fi
            ;;
        legacy|uncategorized)
            run_uncategorized_tests
            ;;
        *)
            print_color "$RED" "Unknown test type: $test_type"
            echo "Available types: unit, integration, e2e, performance, stress, legacy, all"
            echo "Use '$0 list' to see all available test groups"
            exit 1
            ;;
    esac
    
    # Print summary
    print_summary
}

# Run main function
main "$@"