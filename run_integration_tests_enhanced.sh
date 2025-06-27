#!/bin/bash

# Enhanced Integration Test Runner - Shows Individual Test Case Details
# Automatically discovers integration tests and reports individual test case results

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
PASSED_CASES=0
FAILED_CASES=0
SKIPPED_CASES=0
TEST_RESULTS=()
FAILED_CASES_DETAILS=()

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

# Function to parse Google Test output and display individual test cases
parse_gtest_output() {
    local test_executable=$1
    local test_name=$2
    local log_file=$3
    local filter_arg="${4:-}"  # Optional filter argument
    local xml_file="/tmp/${test_name}_results.xml"
    
    # Run test with XML output for structured results
    local start_time=$(date +%s)
    local test_passed=true
    
    if timeout 120 "$test_executable" --gtest_output="xml:$xml_file" $filter_arg > "$log_file" 2>&1; then
        local exit_code=0
    else
        local exit_code=$?
        test_passed=false
    fi
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    # Parse results from log file
    local test_cases=0
    local failed_cases=0
    local skipped_in_test=false
    
    if [ -f "$log_file" ]; then
        while IFS= read -r line; do
            # Detect if tests were skipped
            if [[ "$line" =~ "Test is being skipped" ]] || [[ "$line" =~ "SKIPPED" ]]; then
                skipped_in_test=true
            fi
            
            # Detect test suite start
            if [[ "$line" =~ \[----------\]\ ([0-9]+)\ tests?\ from\ (.+) ]]; then
                local suite_count="${BASH_REMATCH[1]}"
                local suite_name="${BASH_REMATCH[2]}"
                print_color "$CYAN" "  Running $suite_count tests from $suite_name"
            fi
            
            # Detect individual test case results
            if [[ "$line" =~ \[\ \ \ \ \ \ \ OK\ \]\ (.+)\ \(([0-9]+)\ ms\) ]]; then
                local case_name="${BASH_REMATCH[1]}"
                local case_time="${BASH_REMATCH[2]}"
                print_color "$GREEN" "    ✓ $case_name (${case_time}ms)"
                ((PASSED_CASES++))
                ((test_cases++))
            elif [[ "$line" =~ \[\ \ FAILED\ \ \]\ (.+)\ \(([0-9]+)\ ms\) ]]; then
                local case_name="${BASH_REMATCH[1]}"
                local case_time="${BASH_REMATCH[2]}"
                print_color "$RED" "    ✗ $case_name (${case_time}ms)"
                ((FAILED_CASES++))
                ((failed_cases++))
                ((test_cases++))
                FAILED_CASES_DETAILS+=("$test_name::$case_name")
            fi
            
            # Detect skipped tests
            if [[ "$line" =~ \[\ \ SKIPPED\ \]\ (.+) ]] || [[ "$line" =~ You\ have\ ([0-9]+)\ disabled\ test ]]; then
                if [[ "$line" =~ \[\ \ SKIPPED\ \]\ (.+) ]]; then
                    local case_name="${BASH_REMATCH[1]}"
                    print_color "$YELLOW" "    - $case_name (skipped)"
                    ((SKIPPED_CASES++))
                fi
            fi
            
            # Show failure messages
            if [[ "$line" =~ ^(.+):([0-9]+):\ Failure$ ]]; then
                print_color "$DIM$RED" "      $line"
                # Read next few lines for error details
                local error_lines=0
                while IFS= read -r error_line && [ $error_lines -lt 3 ]; do
                    if [[ ! "$error_line" =~ ^\[\ \ FAILED\ \ \] ]]; then
                        print_color "$DIM$RED" "      $error_line"
                        ((error_lines++))
                    else
                        break
                    fi
                done
            fi
        done < "$log_file"
    fi
    
    # Handle skipped tests
    if [ "$skipped_in_test" = true ] && [ $test_cases -eq 0 ]; then
        print_color "$YELLOW" "  Test skipped (integration test requirements not met)"
        ((SKIPPED_TESTS++))
        TEST_RESULTS+=("$test_name|SKIP|$duration|0|0")
    elif [ $test_cases -gt 0 ]; then
        if [ $failed_cases -eq 0 ]; then
            print_color "$GREEN" "  Summary: $test_cases test cases passed (${duration}s)"
            ((PASSED_TESTS++))
        else
            print_color "$RED" "  Summary: $failed_cases/$test_cases test cases failed (${duration}s)"
            ((FAILED_TESTS++))
        fi
        TEST_RESULTS+=("$test_name|$([ "$test_passed" = true ] && echo "PASS" || echo "FAIL")|$duration|$test_cases|$failed_cases")
    else
        # No detailed output available
        if [ "$test_passed" = true ]; then
            print_color "$GREEN" "✓ $test_name (${duration}s)"
            ((PASSED_TESTS++))
        else
            print_color "$RED" "✗ $test_name (${duration}s)"
            ((FAILED_TESTS++))
            echo "  Error output:"
            tail -n 10 "$log_file" | sed 's/^/    /'
        fi
        TEST_RESULTS+=("$test_name|$([ "$test_passed" = true ] && echo "PASS" || echo "FAIL")|$duration|0|0")
    fi
    
    # Clean up XML file
    rm -f "$xml_file"
    
    # Return failure status if test failed
    [ "$test_passed" = true ]
}

# Function to run an integration test with detailed output
run_integration_test_detailed() {
    local test_executable=$1
    local test_name=$(basename "$test_executable")
    local filter_arg=""
    
    # If global filter pattern is set, pass it to the test
    if [ -n "${filter_pattern:-}" ]; then
        filter_arg="--gtest_filter=$filter_pattern"
    fi
    
    # Check if test needs to be built
    if [ ! -f "$test_executable" ]; then
        # Try to build it
        local test_target="${test_name%.cpp}"
        if [ -d "build_ninja" ]; then
            print_color "$CYAN" "Building $test_target..."
            if ! cmake --build build_ninja --target "$test_target" 2>/dev/null; then
                print_color "$YELLOW" "- $test_name (failed to build)"
                ((SKIPPED_TESTS++))
                return
            fi
        else
            print_color "$YELLOW" "- $test_name (not built)"
            ((SKIPPED_TESTS++))
            return
        fi
    fi
    
    if [ -n "$filter_pattern" ]; then
        print_color "$BOLD" "Running $test_name with filter: $filter_pattern"
    else
        print_color "$BOLD" "Running $test_name..."
    fi
    
    parse_gtest_output "$test_executable" "$test_name" "/tmp/${test_name}.log" "$filter_arg"
    echo
}

# Auto-discover test executables by pattern
discover_integration_tests() {
    find build_ninja/bin -name "test_integration_*" -type f -perm +111 2>/dev/null | sort
}

# Discover all test source files
discover_test_sources() {
    find . -name "test_integration_*.cpp" -type f 2>/dev/null | sort
}

# Extract category from test name
get_test_category() {
    local test_name=$(basename "$1")
    # Pattern: test_integration_<category>_<description>
    echo "$test_name" | cut -d'_' -f3
}

# Discover all categories
discover_categories() {
    {
        while IFS= read -r test_path; do
            get_test_category "$test_path"
        done < <(discover_integration_tests)
        
        while IFS= read -r test_path; do
            get_test_category "$test_path"
        done < <(discover_test_sources)
    } | sort -u | grep -v "^$"
}

# Run tests by category
run_category_tests() {
    local category=$1
    print_header "$(echo "$category" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}') Integration Tests"
    
    local found_tests=false
    while IFS= read -r test_path; do
        local test_category=$(get_test_category "$test_path")
        if [[ "$test_category" == "$category" ]]; then
            found_tests=true
            run_integration_test_detailed "$test_path"
        fi
    done < <(discover_integration_tests)
    
    if [[ "$found_tests" == "false" ]]; then
        print_color "$YELLOW" "No tests found for category: $category"
    fi
}

# Function to list available groups
list_groups() {
    print_header "Available Integration Tests"
    
    echo "Integration Test Categories:"
    local found_categories=false
    
    while IFS= read -r category; do
        if [ -n "$category" ]; then
            found_categories=true
            local capitalized_category=$(echo "$category" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')
            local source_count=$(discover_test_sources 2>/dev/null | grep "test_integration_${category}_" | wc -l | tr -d ' ')
            source_count=${source_count:-0}
            
            printf "  ${BOLD}%-20s${NC} - ${GREEN}%d test files${NC}\n" "$capitalized_category" "$source_count"
        fi
    done < <(discover_categories)
    
    if [ "$found_categories" = "false" ]; then
        echo "  No integration test categories found."
    fi
    
    echo
    echo "Commands:"
    printf "  ${BOLD}Run Integration Tests:${NC}\n"
    printf "    $0 core              - Run core integration tests\n"
    printf "    $0 rendering         - Run rendering integration tests\n"
    printf "    $0 all               - Run all integration tests\n"
    printf "    $0 quick             - Run essential integration tests\n"
    printf "    $0 test <test_name>  - Run specific test file or test case\n"
    echo
    printf "  ${BOLD}Options:${NC}\n"
    printf "    --brief              - Show only summary (no individual test cases)\n"
    printf "    --filter=PATTERN     - Run only tests matching pattern\n"
    echo
    printf "  ${BOLD}Examples:${NC}\n"
    printf "    $0 test test_integration_core_camera         - Run specific test file\n"
    printf "    $0 test CameraVisibility.ViewFrustum         - Run specific test case\n"
    printf "    $0 test \"Camera.*\"                           - Run all Camera tests\n"
    printf "    $0 core --filter=\"*Visibility*\"              - Run Visibility tests in core\n"
    echo
    printf "  ${BOLD}Information:${NC}\n"
    printf "    $0 list              - Show available test categories\n"
    printf "    $0 list-tests <file> - List all test cases in a specific file\n"
}

# Function to print detailed test summary
print_detailed_summary() {
    print_header "Integration Test Summary"
    
    local total_files=$((PASSED_TESTS + FAILED_TESTS + SKIPPED_TESTS))
    local total_cases=$((PASSED_CASES + FAILED_CASES + SKIPPED_CASES))
    
    echo "Test Files: $total_files"
    print_color "$GREEN" "  Passed:  $PASSED_TESTS"
    print_color "$RED" "  Failed:  $FAILED_TESTS"
    print_color "$YELLOW" "  Skipped: $SKIPPED_TESTS"
    
    if [ $total_cases -gt 0 ]; then
        echo
        echo "Test Cases: $total_cases"
        print_color "$GREEN" "  Passed:  $PASSED_CASES"
        print_color "$RED" "  Failed:  $FAILED_CASES"
        print_color "$YELLOW" "  Skipped: $SKIPPED_CASES"
    fi
    
    if [ ${#FAILED_CASES_DETAILS[@]} -gt 0 ]; then
        echo
        print_color "$RED" "Failed Test Cases:"
        for case_detail in "${FAILED_CASES_DETAILS[@]}"; do
            echo "  - $case_detail"
        done
    fi
    
    echo
    if [ $FAILED_TESTS -eq 0 ] && [ $FAILED_CASES -eq 0 ]; then
        print_color "$GREEN" "All integration tests passed! 🎉"
        return 0
    else
        print_color "$RED" "Some integration tests failed. Please check the logs."
        return 1
    fi
}

# Quick test selection (essential tests only)
run_quick_tests() {
    print_header "Quick Integration Tests (Essential Only)"
    
    # Define essential tests
    local essential_tests=(
        "test_integration_core_camera_visibility"
        "test_integration_core_voxel_selection"
        "test_integration_rendering_basic"
    )
    
    for test_name in "${essential_tests[@]}"; do
        local test_path="build_ninja/bin/$test_name"
        if [ -f "$test_path" ]; then
            run_integration_test_detailed "$test_path"
        else
            print_color "$YELLOW" "Essential test not found: $test_name"
        fi
    done
}

# Main script logic
main() {
    local brief_mode=false
    local filter_pattern=""
    local processed_args=()
    
    # Parse options
    for arg in "$@"; do
        case "$arg" in
            --brief)
                brief_mode=true
                ;;
            --filter=*)
                filter_pattern="${arg#--filter=}"
                ;;
            *)
                processed_args+=("$arg")
                ;;
        esac
    done
    
    # If brief mode, use the original runner
    if [ "$brief_mode" = true ]; then
        exec ./run_integration_tests.sh "${processed_args[@]}"
    fi
    
    # If no arguments, show help
    if [ ${#processed_args[@]} -eq 0 ]; then
        list_groups
        exit 0
    fi
    
    # Process arguments
    for group in "${processed_args[@]}"; do
        case "$group" in
            list|help|-h|--help)
                list_groups
                exit 0
                ;;
            all)
                print_header "All Integration Tests"
                if [ -d "build_ninja" ]; then
                    print_color "$CYAN" "Building integration tests..."
                    while IFS= read -r test_source; do
                        local test_name=$(basename "$test_source" .cpp)
                        cmake --build build_ninja --target "$test_name" 2>/dev/null || true
                    done < <(discover_test_sources | grep "test_integration_")
                fi
                
                while IFS= read -r test_path; do
                    run_integration_test_detailed "$test_path"
                done < <(discover_integration_tests)
                ;;
            quick)
                run_quick_tests
                ;;
            test)
                # Run specific test file or test case
                if [ ${#processed_args[@]} -lt 2 ]; then
                    print_color "$RED" "Error: 'test' command requires a test name"
                    echo "Usage: $0 test <test_name>"
                    exit 1
                fi
                
                local test_target="${processed_args[1]}"
                
                # Check if it's a test file name
                if [[ "$test_target" =~ ^test_integration_ ]]; then
                    test_target="${test_target%.cpp}"
                    
                    # Build the specific test
                    if [ -d "build_ninja" ]; then
                        print_color "$CYAN" "Building $test_target..."
                        cmake --build build_ninja --target "$test_target" 2>/dev/null || {
                            print_color "$RED" "Failed to build $test_target"
                            exit 1
                        }
                    fi
                    
                    # Run the specific test file
                    local test_path="build_ninja/bin/$test_target"
                    if [ -f "$test_path" ]; then
                        run_integration_test_detailed "$test_path"
                    else
                        print_color "$RED" "Test executable not found: $test_path"
                        exit 1
                    fi
                else
                    # It's a test case pattern
                    print_color "$CYAN" "Searching for tests matching pattern: $test_target"
                    
                    filter_pattern="*${test_target}*"
                    
                    local found_any=false
                    while IFS= read -r test_path; do
                        if "$test_path" --gtest_filter="$filter_pattern" --gtest_list_tests 2>/dev/null | grep -q "\."; then
                            found_any=true
                            run_integration_test_detailed "$test_path"
                        fi
                    done < <(discover_integration_tests)
                    
                    if [ "$found_any" = false ]; then
                        print_color "$YELLOW" "No tests found matching pattern: $test_target"
                    fi
                fi
                ;;
            list-tests)
                # List all test cases in a specific file
                if [ ${#processed_args[@]} -lt 2 ]; then
                    print_color "$RED" "Error: 'list-tests' command requires a test file name"
                    echo "Usage: $0 list-tests <test_file>"
                    exit 1
                fi
                
                local test_file="${processed_args[1]}"
                test_file="${test_file%.cpp}"
                
                local test_path="build_ninja/bin/$test_file"
                if [ ! -f "$test_path" ]; then
                    if [ -d "build_ninja" ]; then
                        print_color "$CYAN" "Building $test_file..."
                        cmake --build build_ninja --target "$test_file" 2>/dev/null || {
                            print_color "$RED" "Test not found or failed to build: $test_file"
                            exit 1
                        }
                    fi
                fi
                
                if [ -f "$test_path" ]; then
                    print_header "Test cases in $test_file"
                    "$test_path" --gtest_list_tests
                else
                    print_color "$RED" "Test executable not found: $test_path"
                    exit 1
                fi
                exit 0
                ;;
            *)
                if discover_categories | grep -q "^${group}$"; then
                    if [ -d "build_ninja" ]; then
                        print_color "$CYAN" "Building ${group} integration tests..."
                        local test_targets=()
                        while IFS= read -r test_source; do
                            if [[ "$test_source" =~ test_integration_${group}_ ]]; then
                                local test_name=$(basename "$test_source" .cpp)
                                test_targets+=("$test_name")
                            fi
                        done < <(discover_test_sources)
                        
                        for target in "${test_targets[@]}"; do
                            cmake --build build_ninja --target "$target" 2>/dev/null || true
                        done
                    fi
                    run_category_tests "$group"
                else
                    print_color "$RED" "Unknown category: $group"
                    echo "Available categories:"
                    discover_categories | sed 's/^/  /'
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