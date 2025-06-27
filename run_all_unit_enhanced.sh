#!/bin/bash

# Enhanced Unit Test Runner - Shows Individual Test Case Details
# Automatically discovers unit tests and reports individual test case results

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
    
    if timeout 60 "$test_executable" --gtest_output="xml:$xml_file" $filter_arg > "$log_file" 2>&1; then
        local exit_code=0
    else
        local exit_code=$?
        test_passed=false
    fi
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    # Parse results from log file to show real-time output style
    local test_cases=0
    local failed_cases=0
    local case_details=()
    
    # First, try to parse the console output for immediate feedback
    if [ -f "$log_file" ]; then
        while IFS= read -r line; do
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
                
                # Capture failure details
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
            
            # Show failure messages (next few lines after a failure)
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
    
    # If we found test cases, show summary for this executable
    if [ $test_cases -gt 0 ]; then
        if [ $failed_cases -eq 0 ]; then
            print_color "$GREEN" "  Summary: $test_cases test cases passed (${duration}s)"
            ((PASSED_TESTS++))
        else
            print_color "$RED" "  Summary: $failed_cases/$test_cases test cases failed (${duration}s)"
            ((FAILED_TESTS++))
        fi
    else
        # No detailed output available, fall back to pass/fail
        if [ "$test_passed" = true ]; then
            print_color "$GREEN" "✓ $test_name (${duration}s)"
            ((PASSED_TESTS++))
        else
            print_color "$RED" "✗ $test_name (${duration}s)"
            ((FAILED_TESTS++))
            echo "  Error output:"
            tail -n 10 "$log_file" | sed 's/^/    /'
        fi
    fi
    
    TEST_RESULTS+=("$test_name|$([ "$test_passed" = true ] && echo "PASS" || echo "FAIL")|$duration|$test_cases|$failed_cases")
    
    # Clean up XML file
    rm -f "$xml_file"
    
    # Return failure status if test failed
    [ "$test_passed" = true ]
}

# Function to run a unit test executable with detailed output
run_unit_test_detailed() {
    local test_executable=$1
    local test_name=$(basename "$test_executable")
    local filter_arg=""
    
    # If global filter pattern is set, pass it to the test
    if [ -n "$filter_pattern" ]; then
        filter_arg="--gtest_filter=$filter_pattern"
    fi
    
    if [ ! -f "$test_executable" ]; then
        print_color "$YELLOW" "- $test_name (not built)"
        ((SKIPPED_TESTS++))
        return
    fi
    
    if [ -n "$filter_pattern" ]; then
        print_color "$BOLD" "Running $test_name with filter: $filter_pattern"
    else
        print_color "$BOLD" "Running $test_name..."
    fi
    
    if ! parse_gtest_output "$test_executable" "$test_name" "/tmp/${test_name}.log" "$filter_arg"; then
        # Stop at first failure
        print_color "$RED" "STOPPING AT FIRST FAILURE as requested"
        echo
        print_color "$CYAN" "To debug this failure:"
        echo "  cd build_ninja && $test_executable --gtest_list_tests"
        echo "  cd build_ninja && $test_executable --gtest_filter='*SpecificTest*'"
        echo
        print_color "$CYAN" "To see the full log:"
        echo "  cat /tmp/${test_name}.log"
        exit 1
    fi
    echo
}

# Auto-discover test executables by pattern
discover_unit_tests() {
    find build_ninja/bin -name "test_unit_*" -type f -perm +111 2>/dev/null | sort
}

# Discover all test source files (for listing even when not built)
discover_test_sources() {
    find . -name "test_unit_*.cpp" -type f 2>/dev/null | sort
}

# Extract subsystem from test name
get_test_subsystem() {
    local test_name=$(basename "$1")
    echo "$test_name" | cut -d'_' -f3
}

# Discover all subsystems
discover_subsystems() {
    {
        while IFS= read -r test_path; do
            get_test_subsystem "$test_path"
        done < <(discover_unit_tests)
        
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
            run_unit_test_detailed "$test_path"
        fi
    done < <(discover_unit_tests)
    
    if [[ "$found_tests" == "false" ]]; then
        print_color "$YELLOW" "No tests found for subsystem: $subsystem"
    fi
}

# List available groups
list_groups() {
    print_header "Available Unit Tests"
    
    echo "Unit Test Subsystems:"
    local found_subsystems=false
    
    while IFS= read -r subsystem; do
        if [ -n "$subsystem" ]; then
            found_subsystems=true
            local capitalized_subsystem=$(echo "$subsystem" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')
            local source_count=$(discover_test_sources 2>/dev/null | grep "test_unit_${subsystem}_" | wc -l | tr -d ' ')
            source_count=${source_count:-0}
            
            printf "  ${BOLD}%-15s${NC} - ${GREEN}%d test files${NC}\n" "$capitalized_subsystem" "$source_count"
        fi
    done < <(discover_subsystems)
    
    if [ "$found_subsystems" = "false" ]; then
        echo "  No unit test subsystems found."
    fi
    
    echo
    echo "Commands:"
    printf "  ${BOLD}Run Unit Tests:${NC}\n"
    printf "    $0 foundation            - Run all foundation unit tests\n"
    printf "    $0 core                  - Run all core unit tests\n"
    printf "    $0 all                   - Run all unit tests\n"
    printf "    $0 test <test_name>      - Run specific test file or test case\n"
    echo
    printf "  ${BOLD}Options:${NC}\n"
    printf "    --brief                  - Show only summary (no individual test cases)\n"
    printf "    --filter=PATTERN         - Run only tests matching pattern\n"
    echo
    printf "  ${BOLD}Examples:${NC}\n"
    printf "    $0 test test_unit_foundation_memory_pool     - Run specific test file\n"
    printf "    $0 test MemoryPool.Allocation                - Run specific test case\n"
    printf "    $0 test \"MemoryPool.*\"                       - Run all MemoryPool tests\n"
    printf "    $0 foundation --filter=\"*Vector*\"            - Run Vector tests in foundation\n"
    echo
    printf "  ${BOLD}Information:${NC}\n"
    printf "    $0 list                  - Show all unit test files\n"
    printf "    $0 list-tests <file>     - List all test cases in a specific file\n"
}

# Function to print detailed test summary
print_detailed_summary() {
    print_header "Unit Test Summary"
    
    local total_files=$((PASSED_TESTS + FAILED_TESTS + SKIPPED_TESTS))
    local total_cases=$((PASSED_CASES + FAILED_CASES + SKIPPED_CASES))
    
    echo "Test Files: $total_files"
    print_color "$GREEN" "  Passed:  $PASSED_TESTS"
    print_color "$RED" "  Failed:  $FAILED_TESTS"
    print_color "$YELLOW" "  Skipped: $SKIPPED_TESTS"
    
    echo
    echo "Test Cases: $total_cases"
    print_color "$GREEN" "  Passed:  $PASSED_CASES"
    print_color "$RED" "  Failed:  $FAILED_CASES"
    print_color "$YELLOW" "  Skipped: $SKIPPED_CASES"
    
    if [ ${#FAILED_CASES_DETAILS[@]} -gt 0 ]; then
        echo
        print_color "$RED" "Failed Test Cases:"
        for case_detail in "${FAILED_CASES_DETAILS[@]}"; do
            echo "  - $case_detail"
        done
    fi
    
    echo
    if [ $FAILED_TESTS -eq 0 ] && [ $FAILED_CASES -eq 0 ]; then
        print_color "$GREEN" "All unit tests passed! 🎉"
        return 0
    else
        print_color "$RED" "Some unit tests failed. Please check the logs."
        return 1
    fi
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
        exec ./run_all_unit.sh "${processed_args[@]}"
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
                print_header "All Unit Tests"
                if [ -d "build_ninja" ]; then
                    print_color "$CYAN" "Building unit tests..."
                    while IFS= read -r test_source; do
                        local test_name=$(basename "$test_source" .cpp)
                        cmake --build build_ninja --target "$test_name" 2>/dev/null || true
                    done < <(discover_test_sources | grep "test_unit_")
                fi
                
                while IFS= read -r test_path; do
                    run_unit_test_detailed "$test_path"
                done < <(discover_unit_tests)
                ;;
            test)
                # Run specific test file or test case
                if [ ${#processed_args[@]} -lt 2 ]; then
                    print_color "$RED" "Error: 'test' command requires a test name"
                    echo "Usage: $0 test <test_name>"
                    exit 1
                fi
                
                local test_target="${processed_args[1]}"
                
                # Check if it's a test file name (starts with test_unit_)
                if [[ "$test_target" =~ ^test_unit_ ]]; then
                    # Remove .cpp extension if present
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
                        run_unit_test_detailed "$test_path"
                    else
                        print_color "$RED" "Test executable not found: $test_path"
                        exit 1
                    fi
                else
                    # It's a test case pattern - find which test files might contain it
                    print_color "$CYAN" "Searching for tests matching pattern: $test_target"
                    
                    # Set the filter pattern
                    filter_pattern="*${test_target}*"
                    
                    # Run all tests with the filter
                    local found_any=false
                    while IFS= read -r test_path; do
                        # Check if this test has any matching cases
                        if "$test_path" --gtest_filter="$filter_pattern" --gtest_list_tests 2>/dev/null | grep -q "\."; then
                            found_any=true
                            run_unit_test_detailed "$test_path"
                        fi
                    done < <(discover_unit_tests)
                    
                    if [ "$found_any" = false ]; then
                        print_color "$YELLOW" "No tests found matching pattern: $test_target"
                        echo "Tip: Use --gtest_list_tests to see available test cases"
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
                test_file="${test_file%.cpp}"  # Remove .cpp if present
                
                # Find the test executable
                local test_path="build_ninja/bin/$test_file"
                if [ ! -f "$test_path" ]; then
                    # Try to build it
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
                if discover_subsystems | grep -q "^${group}$"; then
                    if [ -d "build_ninja" ]; then
                        print_color "$CYAN" "Building ${group} unit tests..."
                        local test_targets=()
                        while IFS= read -r test_source; do
                            if [[ "$test_source" =~ test_unit_${group}_ ]]; then
                                local test_name=$(basename "$test_source" .cpp)
                                test_targets+=("$test_name")
                            fi
                        done < <(discover_test_sources)
                        
                        for target in "${test_targets[@]}"; do
                            cmake --build build_ninja --target "$target" 2>/dev/null || true
                        done
                    fi
                    run_subsystem_tests "$group"
                else
                    print_color "$RED" "Unknown subsystem: $group"
                    echo "Available subsystems:"
                    discover_subsystems | sed 's/^/  /'
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