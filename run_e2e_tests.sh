#!/bin/bash

# End-to-End Test Runner - CLI Shell Tests
# Runs end-to-end tests that validate complete user workflows through the CLI

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
    elif [ "$status" = "FAIL" ]; then
        print_color "$RED" "✗ $test_name (${duration}s)"
        ((FAILED_TESTS++))
    elif [ "$status" = "SKIP" ]; then
        print_color "$YELLOW" "- $test_name (skipped)"
        ((SKIPPED_TESTS++))
    fi
    
    TEST_RESULTS+=("$test_name|$status|$duration")
}

# Function to run a shell script test
run_shell_test() {
    local test_script=$1
    local test_name=$(basename "$test_script" .sh)
    
    if [ ! -f "$test_script" ]; then
        print_result "$test_name" "SKIP" "0"
        return
    fi
    
    if [ ! -x "$test_script" ]; then
        chmod +x "$test_script"
    fi
    
    local start_time=$(date +%s)
    
    # Run test from its directory to ensure relative paths work
    local test_dir=$(dirname "$test_script")
    local test_file=$(basename "$test_script")
    
    if timeout 300 bash -c "cd '$test_dir' && './$test_file'" > "/tmp/${test_name}.log" 2>&1; then
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

# Function to list all test groups
list_groups() {
    print_header "Available End-to-End Test Groups"
    
    printf "E2E Groups:\n"
    printf "  ${BOLD}cli-validation${NC} - CLI validation tests (screenshot-based)\n"
    printf "  ${BOLD}cli-comprehensive${NC} - Comprehensive CLI workflow tests\n"
    printf "  ${BOLD}visual${NC}        - Visual validation tests\n"
    printf "  ${BOLD}rendering${NC}     - Rendering validation tests\n"
    printf "  ${BOLD}workflow${NC}      - Complete user workflow tests\n"
    printf "  ${BOLD}boundary${NC}      - Boundary and edge case tests\n"
    printf "\n"
    printf "Meta Groups:\n"
    printf "  ${BOLD}all${NC}           - Run all end-to-end tests\n"
    printf "  ${BOLD}quick${NC}         - Run quick smoke tests only\n"
    printf "  ${BOLD}smoke${NC}         - Basic functionality smoke tests\n"
    printf "\n"
    printf "Usage: $0 [group1] [group2] ...\n"
    printf "       $0 list    - Show this help\n"
    printf "       $0         - Show this help\n"
    printf "\n"
    printf "Note: For C++ integration tests, use ./run_integration_tests.sh\n"
}

# Function to run CLI validation tests
run_cli_validation_tests() {
    print_header "CLI Validation Tests"
    
    local test_dir="tests/e2e/cli_validation"
    if [ -d "$test_dir" ]; then
        for test in "$test_dir"/test_*.sh; do
            if [ -f "$test" ]; then
                run_shell_test "$test"
            fi
        done
    else
        # Fallback to current location for backward compatibility
        test_dir="tests/cli_validation"
        if [ -d "$test_dir" ]; then
            for test in "$test_dir"/test_*.sh; do
                if [ -f "$test" ]; then
                    run_shell_test "$test"
                fi
            done
        fi
    fi
}

# Function to run comprehensive CLI tests
run_comprehensive_tests() {
    print_header "Comprehensive CLI Test Suite"
    
    local test_dir="tests/e2e/cli_comprehensive"
    if [ -d "$test_dir" ]; then
        for test in "$test_dir"/test_*.sh; do
            if [ -f "$test" ]; then
                run_shell_test "$test"
            fi
        done
    else
        # Fallback to current location for backward compatibility
        test_dir="tests/cli_comprehensive"
        if [ -d "$test_dir" ]; then
            for test in "$test_dir"/test_*.sh; do
                if [ -f "$test" ]; then
                    run_shell_test "$test"
                fi
            done
        fi
    fi
}

# Function to run visual validation tests
run_visual_tests() {
    print_header "Visual Validation Tests"
    
    # Tests that validate visual output through screenshots
    local visual_tests=(
        "tests/e2e/cli_validation/test_basic_voxel_placement.sh"
        "tests/e2e/cli_validation/test_camera_views.sh"
        "tests/e2e/cli_validation/test_render_modes.sh"
        "tests/e2e/cli_comprehensive/test_visual_enhancements.sh"
        "apps/cli/tests/test_screenshot_validation.sh"
    )
    
    # Fallback paths for backward compatibility
    local fallback_tests=(
        "tests/cli_validation/test_basic_voxel_placement.sh"
        "tests/cli_validation/test_camera_views.sh"
        "tests/cli_validation/test_render_modes.sh"
        "tests/cli_comprehensive/test_visual_enhancements.sh"
        "apps/cli/tests/test_screenshot_validation.sh"
    )
    
    for test in "${visual_tests[@]}" "${fallback_tests[@]}"; do
        if [ -f "$test" ]; then
            run_shell_test "$test"
        fi
    done
}

# Function to run rendering tests
run_rendering_tests() {
    print_header "Rendering Validation Tests"
    
    # CLI rendering tests
    local test_dirs=(
        "tests/e2e/cli_validation"
        "tests/cli_validation"  # fallback
    )
    
    for test_dir in "${test_dirs[@]}"; do
        if [ -d "$test_dir" ]; then
            cd "$test_dir" 2>/dev/null || continue
            for test in test_render*.sh test_camera*.sh; do
                if [ -f "$test" ]; then
                    run_shell_test "./$test"
                fi
            done
            cd - > /dev/null
            break  # Only run from first available directory
        fi
    done
}

# Function to run workflow tests
run_workflow_tests() {
    print_header "Complete Workflow Tests"
    
    # Tests that validate complete user workflows
    local workflow_tests=(
        "tests/e2e/cli_comprehensive/test_integration.sh"
        "tests/e2e/cli_comprehensive/test_enhancement_validation.sh"
        "tests/e2e/cli_comprehensive/test_new_features.sh"
        "tests/cli_comprehensive/test_integration.sh"  # fallback
        "tests/cli_comprehensive/test_enhancement_validation.sh"  # fallback
        "tests/cli_comprehensive/test_new_features.sh"  # fallback
    )
    
    for test in "${workflow_tests[@]}"; do
        if [ -f "$test" ]; then
            run_shell_test "$test"
        fi
    done
}

# Function to run boundary tests
run_boundary_tests() {
    print_header "Boundary and Edge Case Tests"
    
    # Run boundary-specific shell scripts
    local boundary_dirs=(
        "tests/e2e"
        "tests"  # fallback
    )
    
    for base_dir in "${boundary_dirs[@]}"; do
        if [ -d "$base_dir" ]; then
            find "$base_dir" -name "test_*boundary*.sh" -o -name "test_*edge*.sh" -o -name "test_workspace*.sh" | while read -r test; do
                if [ -f "$test" ]; then
                    run_shell_test "$test"
                fi
            done
            break  # Only search in first available directory
        fi
    done
}

# Function to run quick smoke tests
run_quick_tests() {
    print_header "Quick Smoke Tests"
    
    # Run only the essential quick tests
    local quick_tests=(
        "tests/e2e/cli_validation/test_basic_voxel_placement.sh"
        "tests/e2e/cli_comprehensive/test_basic_smoke.sh"
        "tests/e2e/cli_comprehensive/test_minimal.sh"
        "tests/cli_validation/test_basic_voxel_placement.sh"  # fallback
        "tests/cli_comprehensive/test_basic_smoke.sh"  # fallback
        "tests/cli_comprehensive/test_minimal.sh"  # fallback
    )
    
    for test in "${quick_tests[@]}"; do
        if [ -f "$test" ]; then
            run_shell_test "$test"
            break  # Only run first available version
        fi
    done
}

# Function to run smoke tests
run_smoke_tests() {
    print_header "Smoke Tests"
    
    # Basic functionality smoke tests
    local smoke_tests=(
        "tests/e2e/cli_comprehensive/test_basic_smoke.sh"
        "tests/e2e/cli_comprehensive/test_minimal.sh"
        "tests/e2e/cli_validation/test_basic_voxel_placement.sh"
        "tests/cli_comprehensive/test_basic_smoke.sh"  # fallback
        "tests/cli_comprehensive/test_minimal.sh"  # fallback
        "tests/cli_validation/test_basic_voxel_placement.sh"  # fallback
    )
    
    for test in "${smoke_tests[@]}"; do
        if [ -f "$test" ]; then
            run_shell_test "$test"
        fi
    done
}

# Function to print test summary
print_summary() {
    print_header "End-to-End Test Summary"
    
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
        print_color "$GREEN" "All end-to-end tests passed! 🎉"
        return 0
    else
        print_color "$RED" "Some end-to-end tests failed. Please check the logs."
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
            cli-validation)
                run_cli_validation_tests
                ;;
            cli-comprehensive)
                run_comprehensive_tests
                ;;
            visual)
                run_visual_tests
                ;;
            rendering)
                run_rendering_tests
                ;;
            workflow)
                run_workflow_tests
                ;;
            boundary)
                run_boundary_tests
                ;;
            quick)
                run_quick_tests
                ;;
            smoke)
                run_smoke_tests
                ;;
            all)
                run_cli_validation_tests
                run_comprehensive_tests
                run_visual_tests
                run_rendering_tests
                run_workflow_tests
                run_boundary_tests
                ;;
            *)
                print_color "$RED" "Unknown end-to-end test group: $group"
                echo "Use '$0 list' to see available groups"
                exit 1
                ;;
        esac
    done
    
    # Print summary
    print_summary
}

# Run main function
main "$@"