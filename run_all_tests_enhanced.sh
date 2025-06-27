#!/bin/bash

# Enhanced Unified Test Runner - Comprehensive Test Execution with Detailed Reporting
# Runs all test types with enhanced visibility into individual test cases

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

# Test statistics
TOTAL_PASSED=0
TOTAL_FAILED=0
TOTAL_SKIPPED=0
TOTAL_TIME=0

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

# Function to show usage
show_usage() {
    print_header "Enhanced Unified Test Runner"
    
    echo "Usage: $0 [OPTIONS] [TEST_TYPE] [SUBSYSTEM]"
    echo
    echo "Options:"
    echo "  --brief              Show only summary (no individual test cases)"
    echo "  --filter=PATTERN     Run only tests matching pattern (for C++ tests)"
    echo "  --parallel           Run test types in parallel (experimental)"
    echo
    echo "Test Types:"
    echo "  all                  Run all tests (default)"
    echo "  unit                 Run unit tests only"
    echo "  integration          Run integration tests only"
    echo "  e2e                  Run end-to-end tests only"
    echo "  performance          Run performance tests only"
    echo "  stress               Run stress tests only"
    echo
    echo "Subsystems (for unit/integration):"
    echo "  foundation          Foundation layer tests"
    echo "  core                Core library tests"
    echo "  cli                 CLI application tests"
    echo "  rendering           Rendering subsystem tests"
    echo
    echo "Special Commands:"
    echo "  test <name>         Run specific test by name"
    echo "  list                List all available tests"
    echo "  quick               Run essential tests only"
    echo "  ci                  Run CI test suite"
    echo
    echo "Examples:"
    echo "  $0                                    # Run all tests"
    echo "  $0 unit                               # Run all unit tests"
    echo "  $0 unit foundation                    # Run foundation unit tests"
    echo "  $0 test MemoryPool.Allocation         # Run specific test case"
    echo "  $0 --filter=\"*Vector*\" unit           # Run Vector unit tests"
    echo "  $0 --brief all                        # Run all tests with summary only"
}

# Function to run tests with enhanced runner
run_test_suite() {
    local test_type=$1
    local args="${2:-}"
    local start_time=$(date +%s)
    
    case "$test_type" in
        unit)
            if [ -f "./run_all_unit_enhanced.sh" ]; then
                ./run_all_unit_enhanced.sh $args || return 1
            else
                print_color "$YELLOW" "Enhanced unit test runner not found, using standard runner"
                ./run_all_unit.sh $args || return 1
            fi
            ;;
        integration)
            if [ -f "./run_integration_tests_enhanced.sh" ]; then
                ./run_integration_tests_enhanced.sh $args || return 1
            else
                print_color "$YELLOW" "Enhanced integration test runner not found, using standard runner"
                ./run_integration_tests.sh $args || return 1
            fi
            ;;
        e2e)
            if [ -f "./run_e2e_tests_enhanced.sh" ]; then
                ./run_e2e_tests_enhanced.sh $args || return 1
            else
                print_color "$YELLOW" "Enhanced E2E test runner not found, using standard runner"
                ./run_e2e_tests.sh $args || return 1
            fi
            ;;
        performance|stress)
            # These don't have enhanced versions yet
            local runner="./run_${test_type}_tests.sh"
            if [ -f "$runner" ]; then
                $runner $args || return 1
            else
                print_color "$YELLOW" "No ${test_type} tests found"
            fi
            ;;
    esac
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    ((TOTAL_TIME += duration))
    
    return 0
}

# Function to list all available tests
list_all_tests() {
    print_header "Available Tests"
    
    # Unit tests
    if [ -f "./run_all_unit_enhanced.sh" ]; then
        print_color "$CYAN" "Unit Tests:"
        ./run_all_unit_enhanced.sh list | grep -E "^\s+[A-Z]" | sed 's/^/  /'
        echo
    fi
    
    # Integration tests
    if [ -f "./run_integration_tests_enhanced.sh" ]; then
        print_color "$CYAN" "Integration Tests:"
        ./run_integration_tests_enhanced.sh list | grep -E "^\s+[A-Z]" | sed 's/^/  /'
        echo
    fi
    
    # E2E tests
    if [ -f "./run_e2e_tests_enhanced.sh" ]; then
        print_color "$CYAN" "E2E Tests:"
        ./run_e2e_tests_enhanced.sh list | grep -E "^\s+[A-Z]" | sed 's/^/  /'
        echo
    fi
}

# Function to run specific test
run_specific_test() {
    local test_name="$1"
    
    print_header "Running Specific Test: $test_name"
    
    # Try each test runner to find the test
    local found=false
    
    # Check if it's a test file name pattern
    if [[ "$test_name" =~ ^test_ ]]; then
        # It's likely a test file
        if [[ "$test_name" =~ ^test_unit_ ]]; then
            ./run_all_unit_enhanced.sh test "$test_name" && found=true
        elif [[ "$test_name" =~ ^test_integration_ ]]; then
            ./run_integration_tests_enhanced.sh test "$test_name" && found=true
        elif [[ "$test_name" =~ ^test_e2e_ ]]; then
            ./run_e2e_tests_enhanced.sh test "$test_name" && found=true
        fi
    else
        # It's likely a test case pattern - try all C++ test runners
        print_color "$CYAN" "Searching for test case: $test_name"
        
        # Try unit tests first
        if ./run_all_unit_enhanced.sh test "$test_name" 2>/dev/null; then
            found=true
        fi
        
        # Try integration tests if not found
        if [ "$found" = false ]; then
            if ./run_integration_tests_enhanced.sh test "$test_name" 2>/dev/null; then
                found=true
            fi
        fi
    fi
    
    if [ "$found" = false ]; then
        print_color "$RED" "Test not found: $test_name"
        echo "Use '$0 list' to see available tests"
        return 1
    fi
}

# Function to run quick/essential tests
run_quick_tests() {
    print_header "Quick Test Suite (Essential Tests Only)"
    
    print_color "$CYAN" "Running essential unit tests..."
    ./run_all_unit_enhanced.sh foundation core || return 1
    
    print_color "$CYAN" "Running essential integration tests..."
    ./run_integration_tests_enhanced.sh quick || return 1
    
    print_color "$CYAN" "Running essential E2E tests..."
    # Run specific essential E2E tests
    ./run_e2e_tests_enhanced.sh test test_e2e_cli_simple_voxel.sh || true
    ./run_e2e_tests_enhanced.sh test test_e2e_cli_edge_minimal.sh || true
}

# Function to run CI test suite
run_ci_tests() {
    print_header "CI Test Suite"
    
    # Build everything first
    print_color "$CYAN" "Building all test targets..."
    cmake --build build_ninja --target all 2>/dev/null || {
        print_color "$RED" "Build failed"
        return 1
    }
    
    # Run all tests with brief output
    run_test_suite unit "--brief all" || return 1
    run_test_suite integration "--brief all" || return 1
    run_test_suite e2e "all" || return 1
}

# Main script logic
main() {
    local brief_mode=""
    local filter_pattern=""
    local parallel_mode=false
    local test_type="all"
    local subsystem=""
    local processed_args=()
    
    # Parse arguments
    for arg in "$@"; do
        case "$arg" in
            --brief)
                brief_mode="--brief"
                ;;
            --filter=*)
                filter_pattern="$arg"
                ;;
            --parallel)
                parallel_mode=true
                ;;
            -h|--help|help)
                show_usage
                exit 0
                ;;
            list)
                list_all_tests
                exit 0
                ;;
            test)
                # Next argument should be test name
                continue
                ;;
            quick)
                run_quick_tests
                exit $?
                ;;
            ci)
                run_ci_tests
                exit $?
                ;;
            unit|integration|e2e|performance|stress)
                test_type="$arg"
                ;;
            all)
                test_type="all"
                ;;
            foundation|core|cli|rendering)
                subsystem="$arg"
                ;;
            *)
                # Check if previous arg was "test"
                if [ "${processed_args[-1]:-}" = "test" ]; then
                    run_specific_test "$arg"
                    exit $?
                else
                    processed_args+=("$arg")
                fi
                ;;
        esac
        processed_args+=("$arg")
    done
    
    # Construct arguments for runners
    local runner_args="$brief_mode $filter_pattern $subsystem"
    runner_args="${runner_args## }"  # Trim leading spaces
    
    # Show header
    if [ "$test_type" = "all" ]; then
        print_header "Running All Tests"
    else
        print_header "Running $(echo "$test_type" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}') Tests"
    fi
    
    # Run tests
    local failed_suites=()
    
    if [ "$test_type" = "all" ]; then
        # Run all test types
        local test_types=(unit integration e2e performance stress)
        
        for suite in "${test_types[@]}"; do
            if [ -f "./run_${suite}_tests.sh" ] || [ -f "./run_all_${suite}.sh" ] || [ -f "./run_${suite}_tests_enhanced.sh" ] || [ -f "./run_all_${suite}_enhanced.sh" ]; then
                print_color "$PURPLE" "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
                print_color "$PURPLE" "Running ${suite} tests..."
                print_color "$PURPLE" "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
                
                if run_test_suite "$suite" "$runner_args"; then
                    ((TOTAL_PASSED++))
                else
                    ((TOTAL_FAILED++))
                    failed_suites+=("$suite")
                fi
                echo
            fi
        done
    else
        # Run specific test type
        if run_test_suite "$test_type" "$runner_args"; then
            ((TOTAL_PASSED++))
        else
            ((TOTAL_FAILED++))
            failed_suites+=("$test_type")
        fi
    fi
    
    # Print final summary
    print_header "Overall Test Summary"
    
    if [ "$test_type" = "all" ]; then
        echo "Test Suites Run: $((TOTAL_PASSED + TOTAL_FAILED))"
        print_color "$GREEN" "  Passed: $TOTAL_PASSED"
        print_color "$RED" "  Failed: $TOTAL_FAILED"
    fi
    
    if [ ${#failed_suites[@]} -gt 0 ]; then
        echo
        print_color "$RED" "Failed Test Suites:"
        for suite in "${failed_suites[@]}"; do
            echo "  - $suite"
        done
    fi
    
    echo
    echo "Total Execution Time: ${TOTAL_TIME}s"
    
    echo
    if [ $TOTAL_FAILED -eq 0 ]; then
        print_color "$GREEN" "All tests passed! 🎉"
        exit 0
    else
        print_color "$RED" "Some tests failed. Please review the output above."
        exit 1
    fi
}

# Run main function
main "$@"