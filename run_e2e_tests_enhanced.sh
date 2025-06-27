#!/bin/bash

# Enhanced E2E Test Runner - Improved File-Level Reporting
# Automatically discovers E2E shell scripts and provides better visibility

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
DIM='\033[2m'

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Test results
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0
TEST_RESULTS=()
FAILED_TEST_DETAILS=()

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

# Function to run an E2E test script with enhanced output
run_e2e_test() {
    local test_script=$1
    local test_name=$(basename "$test_script")
    local test_dir=$(dirname "$test_script")
    
    if [ ! -f "$test_script" ]; then
        print_color "$YELLOW" "- $test_name (not found)"
        ((SKIPPED_TESTS++))
        TEST_RESULTS+=("$test_name|SKIP|0")
        return
    fi
    
    if [ ! -x "$test_script" ]; then
        print_color "$YELLOW" "- $test_name (not executable)"
        ((SKIPPED_TESTS++))
        TEST_RESULTS+=("$test_name|SKIP|0")
        return
    fi
    
    print_color "$BOLD" "Running $test_name..."
    
    local start_time=$(date +%s)
    local log_file="/tmp/${test_name}.log"
    
    # Change to test directory for relative paths
    pushd "$test_dir" > /dev/null
    
    # Run test with timeout and capture output
    if timeout 30 "./$test_name" > "$log_file" 2>&1; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        print_color "$GREEN" "✓ $test_name (${duration}s)"
        ((PASSED_TESTS++))
        TEST_RESULTS+=("$test_name|PASS|$duration")
        
        # Show test output summary if available
        if grep -q "PASS\|SUCCESS\|✓" "$log_file" 2>/dev/null; then
            local pass_count=$(grep -c "PASS\|SUCCESS\|✓" "$log_file" || true)
            if [ $pass_count -gt 0 ]; then
                print_color "$DIM$GREEN" "  $pass_count successful checks"
            fi
        fi
    else
        local exit_code=$?
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        
        if [ $exit_code -eq 124 ]; then
            print_color "$RED" "✗ $test_name (TIMEOUT after ${duration}s)"
            FAILED_TEST_DETAILS+=("$test_name: Timeout after 30s")
        else
            print_color "$RED" "✗ $test_name (${duration}s, exit code: $exit_code)"
            FAILED_TEST_DETAILS+=("$test_name: Exit code $exit_code")
        fi
        
        ((FAILED_TESTS++))
        TEST_RESULTS+=("$test_name|FAIL|$duration")
        
        # Show error output
        echo "  Error output:"
        if [ -f "$log_file" ]; then
            # Show last error or failure message
            if grep -E "ERROR|FAIL|✗|Failed" "$log_file" > /dev/null 2>&1; then
                grep -E "ERROR|FAIL|✗|Failed" "$log_file" | tail -n 5 | sed 's/^/    /'
            else
                tail -n 10 "$log_file" | sed 's/^/    /'
            fi
        fi
    fi
    
    popd > /dev/null
}

# Auto-discover E2E test scripts
discover_e2e_tests() {
    find . -name "test_e2e_*.sh" -type f 2>/dev/null | sort
}

# Extract category from test name
get_test_category() {
    local test_name=$(basename "$1")
    # Pattern: test_e2e_<category>_<description>.sh
    # Extract category (e.g., cli, rendering, etc.)
    echo "$test_name" | sed -E 's/test_e2e_([^_]+)_.*/\1/'
}

# Discover all categories
discover_categories() {
    discover_e2e_tests | while read -r test_path; do
        get_test_category "$test_path"
    done | sort -u | grep -v "^$"
}

# Run tests by category
run_category_tests() {
    local category=$1
    print_header "$(echo "$category" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}') E2E Tests"
    
    local found_tests=false
    while IFS= read -r test_path; do
        local test_category=$(get_test_category "$test_path")
        if [[ "$test_category" == "$category" ]]; then
            found_tests=true
            run_e2e_test "$test_path"
            echo
        fi
    done < <(discover_e2e_tests)
    
    if [[ "$found_tests" == "false" ]]; then
        print_color "$YELLOW" "No tests found for category: $category"
    fi
}

# Function to list available tests
list_tests() {
    print_header "Available E2E Tests"
    
    echo "E2E Test Categories:"
    local found_categories=false
    
    while IFS= read -r category; do
        if [ -n "$category" ]; then
            found_categories=true
            local capitalized_category=$(echo "$category" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')
            local test_count=$(discover_e2e_tests | grep "test_e2e_${category}_" | wc -l | tr -d ' ')
            test_count=${test_count:-0}
            
            printf "  ${BOLD}%-20s${NC} - ${GREEN}%d test scripts${NC}\n" "$capitalized_category" "$test_count"
        fi
    done < <(discover_categories)
    
    if [ "$found_categories" = "false" ]; then
        echo "  No E2E test categories found."
    fi
    
    echo
    echo "Test Scripts by Category:"
    while IFS= read -r category; do
        if [ -n "$category" ]; then
            echo
            print_color "$CYAN" "  $category:"
            while IFS= read -r test_path; do
                local test_category=$(get_test_category "$test_path")
                if [[ "$test_category" == "$category" ]]; then
                    local test_name=$(basename "$test_path")
                    echo "    - $test_name"
                fi
            done < <(discover_e2e_tests)
        fi
    done < <(discover_categories)
    
    echo
    echo "Commands:"
    printf "  ${BOLD}Run E2E Tests:${NC}\n"
    printf "    $0 cli               - Run CLI E2E tests\n"
    printf "    $0 rendering         - Run rendering E2E tests\n"
    printf "    $0 all               - Run all E2E tests\n"
    printf "    $0 test <script>     - Run specific test script\n"
    echo
    printf "  ${BOLD}Examples:${NC}\n"
    printf "    $0 test test_e2e_cli_simple_voxel.sh    - Run specific test\n"
    printf "    $0 cli                                   - Run all CLI tests\n"
    echo
    printf "  ${BOLD}Information:${NC}\n"
    printf "    $0 list              - Show all available E2E tests\n"
}

# Function to print detailed test summary
print_detailed_summary() {
    print_header "E2E Test Summary"
    
    local total=$((PASSED_TESTS + FAILED_TESTS + SKIPPED_TESTS))
    
    echo "Total Tests: $total"
    print_color "$GREEN" "  Passed:  $PASSED_TESTS"
    print_color "$RED" "  Failed:  $FAILED_TESTS"
    print_color "$YELLOW" "  Skipped: $SKIPPED_TESTS"
    
    if [ ${#FAILED_TEST_DETAILS[@]} -gt 0 ]; then
        echo
        print_color "$RED" "Failed Tests:"
        for detail in "${FAILED_TEST_DETAILS[@]}"; do
            echo "  - $detail"
        done
    fi
    
    # Show test timing statistics
    if [ ${#TEST_RESULTS[@]} -gt 0 ]; then
        echo
        echo "Test Execution Times:"
        local total_time=0
        for result in "${TEST_RESULTS[@]}"; do
            IFS='|' read -r name status duration <<< "$result"
            if [ "$status" != "SKIP" ]; then
                ((total_time += duration))
            fi
        done
        
        # Sort by duration and show slowest tests
        local sorted_results=$(printf '%s\n' "${TEST_RESULTS[@]}" | sort -t'|' -k3 -nr)
        echo "  Slowest tests:"
        echo "$sorted_results" | head -n 3 | while IFS='|' read -r name status duration; do
            if [ "$status" != "SKIP" ] && [ "$duration" -gt 0 ]; then
                printf "    %-40s %3ds\n" "$name" "$duration"
            fi
        done
        
        echo "  Total execution time: ${total_time}s"
    fi
    
    echo
    if [ $FAILED_TESTS -eq 0 ]; then
        print_color "$GREEN" "All E2E tests passed! 🎉"
        return 0
    else
        print_color "$RED" "Some E2E tests failed. Please check the logs."
        return 1
    fi
}

# Main script logic
main() {
    # If no arguments, show help
    if [ $# -eq 0 ]; then
        list_tests
        exit 0
    fi
    
    # Process arguments
    for arg in "$@"; do
        case "$arg" in
            list|help|-h|--help)
                list_tests
                exit 0
                ;;
            all)
                print_header "All E2E Tests"
                while IFS= read -r test_path; do
                    run_e2e_test "$test_path"
                    echo
                done < <(discover_e2e_tests)
                ;;
            test)
                # Run specific test script
                if [ $# -lt 2 ]; then
                    print_color "$RED" "Error: 'test' command requires a test script name"
                    echo "Usage: $0 test <test_script>"
                    exit 1
                fi
                
                shift  # Remove 'test' from arguments
                local test_script="$1"
                
                # Find the test script
                local found=false
                while IFS= read -r test_path; do
                    if [[ "$(basename "$test_path")" == "$test_script" ]]; then
                        found=true
                        run_e2e_test "$test_path"
                        break
                    fi
                done < <(discover_e2e_tests)
                
                if [ "$found" = false ]; then
                    print_color "$RED" "Test script not found: $test_script"
                    echo "Use '$0 list' to see available tests"
                    exit 1
                fi
                ;;
            *)
                # Try to run as a category
                if discover_categories | grep -q "^${arg}$"; then
                    run_category_tests "$arg"
                else
                    print_color "$RED" "Unknown category: $arg"
                    echo "Available categories:"
                    discover_categories | sed 's/^/  /'
                    echo
                    echo "Use '$0 list' to see all available tests"
                    exit 1
                fi
                ;;
        esac
    done
    
    # Print summary
    print_detailed_summary
}

# Run main function
main "$@"