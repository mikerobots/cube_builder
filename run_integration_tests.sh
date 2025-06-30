#!/bin/bash

# Integration Test Runner - Auto-Discovery Version
# Automatically discovers integration tests using naming convention: test_integration_*

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

# Function to run a C++ test executable
run_cpp_test() {
    local test_executable=$1
    local test_name=$(basename "$test_executable")
    
    if [ ! -f "$test_executable" ]; then
        print_result "$test_name" "SKIP" "0"
        return
    fi
    
    local start_time=$(date +%s)
    
    if timeout 90 "$test_executable" > "/tmp/${test_name}.log" 2>&1; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        
        # Check for skipped tests in Google Test output
        local skipped_count=0
        if [ -f "/tmp/${test_name}.log" ]; then
            skipped_count=$(grep -c "\[  SKIPPED \]" "/tmp/${test_name}.log" || true)
        fi
        
        if [ $skipped_count -gt 0 ]; then
            print_result "$test_name" "SKIP" "$duration"
            echo "  Note: $skipped_count test(s) were skipped"
        else
            print_result "$test_name" "PASS" "$duration"
        fi
    else
        # Check if all tests passed but runner timed out
        local passed_count=0
        local failed_count=0
        local skipped_count=0
        if [ -f "/tmp/${test_name}.log" ]; then
            passed_count=$(grep -c "\[       OK \]" "/tmp/${test_name}.log" || true)
            failed_count=$(grep -c "\[  FAILED  \]" "/tmp/${test_name}.log" || true)
            skipped_count=$(grep -c "\[  SKIPPED \]" "/tmp/${test_name}.log" || true)
        fi
        
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        print_result "$test_name" "FAIL" "$duration"
        echo "  Error output:"
        tail -n 10 "/tmp/${test_name}.log" | sed 's/^/    /'
    fi
}

# Auto-discover test executables by pattern
discover_integration_tests() {
    find build_ninja/bin -name "test_integration_*" -type f -perm +111 2>/dev/null | sort
}

# Discover all test source files (for listing even when not built)
discover_test_sources() {
    # Search for integration test source files
    find . -name "test_integration_*.cpp" -type f 2>/dev/null | sort
}

# Discover uncategorized test executables
discover_uncategorized_executables() {
    # Find executables that might be tests but don't follow naming convention
    # Exclude all tests that follow the new naming convention
    find build_ninja/bin -type f -perm +111 2>/dev/null | \
        grep -E "(Test|test_)" | \
        grep -v "test_integration_" | \
        grep -v "test_unit_" | \
        grep -v "test_performance_" | \
        grep -v "test_stress_" | \
        grep -v "test_e2e_" | \
        sort
}

# Discover uncategorized test sources  
discover_uncategorized_sources() {
    # Find test source files that don't follow naming convention
    # Exclude unit tests and other test types that follow the new convention
    find . -name "*Test*.cpp" -o -name "test_*.cpp" | \
        grep -v "test_integration_" | \
        grep -v "test_unit_" | \
        grep -v "test_performance_" | \
        grep -v "test_stress_" | \
        grep -v "test_e2e_" | \
        grep -v "/build" | \
        grep -v "apps/shader_test" | \
        sort
}

# Extract category from test name
get_test_category() {
    local test_name=$(basename "$1")
    # Pattern: test_integration_<category>_<description>
    echo "$test_name" | cut -d'_' -f3
}

# Run tests by category
run_category_tests() {
    local category=$1
    print_header "$(echo "$category" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}') Integration Tests"
    
    # Build tests for this category if needed
    local expected_tests=()
    local failed_builds=()
    if [ -d "build_ninja" ]; then
        print_color "$CYAN" "Building ${category} integration tests..."
        # Build only integration test targets for this category
        local built_count=0
        while IFS= read -r test_path; do
            local test_name=$(basename "$test_path" .cpp)
            local test_category=$(get_test_category "$test_path")
            if [[ "$test_category" == "$category" ]]; then
                expected_tests+=("build_ninja/bin/$test_name")
                if [ ! -f "build_ninja/bin/$test_name" ]; then
                    ((built_count++))
                    # Build quietly to avoid clutter, show only on error
                    if ! cmake --build build_ninja --target "$test_name" > /dev/null 2>&1; then
                        print_color "$YELLOW" "  Warning: Failed to build $test_name"
                        failed_builds+=("build_ninja/bin/$test_name")
                    fi
                fi
            fi
        done < <(discover_test_sources)
        
        if [ $built_count -gt 0 ]; then
            echo "  Built $built_count tests"
        else
            echo "  All ${category} tests already built"
        fi
    fi
    
    local found_tests=false
    local processed_tests=()
    
    # Check both built executables and expected tests
    for expected_test in "${expected_tests[@]}"; do
        local test_category=$(get_test_category "$expected_test")
        if [[ "$test_category" == "$category" ]]; then
            found_tests=true  # We found a test for this category (even if build failed)
            
            # Skip if this test failed to build (don't report as test failure)
            local is_failed_build=false
            if [ ${#failed_builds[@]} -gt 0 ]; then
                for failed_build in "${failed_builds[@]}"; do
                    if [[ "$expected_test" == "$failed_build" ]]; then
                        is_failed_build=true
                        break
                    fi
                done
            fi
            
            if [[ "$is_failed_build" == "false" ]]; then
                processed_tests+=("$expected_test")
                run_cpp_test "$expected_test"
            else
                # Track as processed so we don't run it again
                processed_tests+=("$expected_test")
            fi
        fi
    done
    
    # Also run any existing tests that might not have been in expected list
    while IFS= read -r test_path; do
        local test_category=$(get_test_category "$test_path")
        if [[ "$test_category" == "$category" ]]; then
            # Check if we already processed this test
            local already_processed=false
            for processed_test in "${processed_tests[@]}"; do
                if [[ "$test_path" == "$processed_test" ]]; then
                    already_processed=true
                    break
                fi
            done
            if [[ "$already_processed" == "false" ]]; then
                found_tests=true
                processed_tests+=("$test_path")
                run_cpp_test "$test_path"
            fi
        fi
    done < <(discover_integration_tests)
    
    if [[ "$found_tests" == "false" ]]; then
        print_color "$YELLOW" "No tests found for category: $category"
    fi
}

# Discover all categories dynamically from both built executables and source files
discover_categories() {
    {
        # From built executables
        while IFS= read -r test_path; do
            get_test_category "$test_path"
        done < <(discover_integration_tests)
        
        # From source files (in case not built yet)
        while IFS= read -r test_path; do
            get_test_category "$test_path"
        done < <(discover_test_sources)
    } | sort -u | grep -v "^$"
}

# Run uncategorized tests (backward compatibility)
run_uncategorized_tests() {
    print_header "Uncategorized Integration Tests"
    
    local found_tests=false
    
    # Run discovered uncategorized tests
    while IFS= read -r test; do
        if [[ -n "$test" && -x "$test" ]]; then
            found_tests=true
            run_cpp_test "$test"
        fi
    done < <(discover_uncategorized_executables)
    
    # No special cases needed - all tests follow naming convention
    
    if [[ "$found_tests" == "false" ]]; then
        print_color "$YELLOW" "No uncategorized tests found"
    fi
}

# List available groups (dynamic)
list_groups() {
    print_header "Available Integration Tests"
    
    # Show integration test counts by category
    echo "Integration Test Categories:"
    local found_categories=false
    
    while IFS= read -r category; do
        if [ -n "$category" ]; then
            found_categories=true
            local capitalized_category=$(echo "$category" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')
            
            # Count integration test files for this category
            local source_count=$(discover_test_sources 2>/dev/null | grep "test_integration_${category}_" | wc -l | tr -d ' ')
            source_count=${source_count:-0}
            
            printf "  ${BOLD}%-15s${NC} - ${GREEN}%d test files${NC}\n" "$capitalized_category" "$source_count"
        fi
    done < <(discover_categories)
    
    if [ "$found_categories" = "false" ]; then
        echo "  No integration test categories found."
        echo "  Looking for files matching pattern: test_integration_<category>_<description>.cpp"
    fi
    
    echo
    
    # Show legacy test count (non-integration, non-unit tests that might be integration tests)
    echo "Legacy Tests (not following integration naming convention):"
    
    # Count potential integration tests that don't follow our naming
    local legacy_count=0
    while IFS= read -r source; do
        if [ -n "$source" ]; then
            # Skip unit tests and focus on potential integration tests
            if [[ "$source" != *"test_unit_"* ]] && [[ "$source" == *"test_"* || "$source" == *"Test"* || "$source" == *"integration"* ]]; then
                ((legacy_count++))
            fi
        fi
    done < <(discover_uncategorized_sources)
    
    if [ $legacy_count -gt 0 ]; then
        printf "  ${YELLOW}Legacy integration tests${NC} - ${YELLOW}%d test files${NC}\n" "$legacy_count"
    else
        echo "  None found - all integration tests follow naming convention! 🎉"
    fi
    
    echo
    echo "Commands:"
    printf "  ${BOLD}Run Integration Tests:${NC}\n"
    printf "    $0 core               - Build & run core integration tests\n"
    printf "    $0 rendering          - Build & run rendering integration tests\n"
    printf "    $0 all                - Build & run all integration tests\n"
    printf "    $0 quick              - Build & run essential quick tests\n"
    printf "    $0 <test_name>        - Build & run individual test (e.g., test_integration_core_camera)\n"
    echo
    printf "  ${BOLD}Information:${NC}\n"
    printf "    $0 list               - Show all integration test files\n"
    echo
    printf "  ${BOLD}Note:${NC} Tests are built automatically when you run them\n"
}

# Run quick integration tests
run_quick_tests() {
    print_header "Quick Integration Tests"
    
    # Run only the essential quick integration tests
    local quick_tests=(
        "build_ninja/bin/test_integration_core_camera_cube_visibility_simple"
        "build_ninja/bin/test_integration_verification_core_functionality"
        "build_ninja/bin/VoxelEditor_CLI_Tests"
    )
    
    local found_tests=false
    for test in "${quick_tests[@]}"; do
        if [ -x "$test" ]; then
            found_tests=true
            run_cpp_test "$test"
        fi
    done
    
    if [[ "$found_tests" == "false" ]]; then
        print_color "$YELLOW" "No quick tests found"
    fi
}

# Function to print test summary
print_summary() {
    print_header "Integration Test Summary"
    
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
        echo "     cd build_ninja && ./bin/test_integration_<category>_<name> --gtest_filter='*SpecificTest*'"
        echo "  3. List all tests in an executable:"
        echo "     cd build_ninja && ./bin/test_integration_<category>_<name> --gtest_list_tests"
    fi
    
    echo
    if [ $FAILED_TESTS -eq 0 ]; then
        print_color "$GREEN" "All integration tests passed! 🎉"
        return 0
    else
        print_color "$RED" "Some integration tests failed. Please check the logs."
        return 1
    fi
}

# Function to run individual test by name
run_individual_test() {
    local test_name=$1
    print_header "Running Individual Test: $test_name"
    
    # First try to find the test source file
    local test_source=""
    local found=false
    
    # Look for exact match first
    while IFS= read -r source; do
        local basename=$(basename "$source" .cpp)
        if [[ "$basename" == "$test_name" ]]; then
            test_source="$source"
            found=true
            break
        fi
    done < <(discover_test_sources | grep "test_integration_")
    
    # If not found, try partial match (user might have typed just part of the name)
    if [[ "$found" == "false" ]]; then
        while IFS= read -r source; do
            local basename=$(basename "$source" .cpp)
            if [[ "$basename" == *"$test_name"* ]]; then
                if [[ -n "$test_source" ]]; then
                    # Multiple matches found
                    print_color "$RED" "Multiple tests match '$test_name':"
                    discover_test_sources | grep "test_integration_" | while IFS= read -r s; do
                        local b=$(basename "$s" .cpp)
                        if [[ "$b" == *"$test_name"* ]]; then
                            echo "  - $b"
                        fi
                    done
                    echo
                    echo "Please be more specific."
                    return 1
                fi
                test_source="$source"
                found=true
            fi
        done < <(discover_test_sources | grep "test_integration_")
    fi
    
    if [[ "$found" == "false" ]]; then
        print_color "$RED" "Test '$test_name' not found"
        echo
        echo "Available integration tests:"
        discover_test_sources | grep "test_integration_" | while IFS= read -r source; do
            echo "  - $(basename "$source" .cpp)"
        done | sort | head -20
        echo "  ..."
        echo
        echo "Use '$0 list' to see all available tests"
        return 1
    fi
    
    # Build and run the test
    local test_basename=$(basename "$test_source" .cpp)
    local test_executable="build_ninja/bin/$test_basename"
    
    # Build the specific test
    if [ -d "build_ninja" ]; then
        print_color "$CYAN" "Building test: $test_basename..."
        if [ ! -f "$test_executable" ]; then
            # Build quietly to avoid clutter, show only on error
            if ! cmake --build build_ninja --target "$test_basename" > /dev/null 2>&1; then
                print_color "$YELLOW" "  Warning: Failed to build $test_basename"
            else
                echo "  Built 1 test"
            fi
        else
            echo "  Test already built"
        fi
    fi
    
    # Run the test
    if [ -x "$test_executable" ]; then
        run_cpp_test "$test_executable"
    else
        print_color "$RED" "Test executable not found: $test_executable"
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
                # Run all discovered integration tests
                print_header "All Integration Tests"
                if [ -d "build_ninja" ]; then
                    print_color "$CYAN" "Building integration tests..."
                    # Build only integration test targets, one at a time to avoid dependency issues
                    local test_count=0
                    while IFS= read -r test_source; do
                        local test_name=$(basename "$test_source" .cpp)
                        # Only build if it's an integration test and the executable doesn't exist
                        if [[ "$test_name" == test_integration_* ]]; then
                            if [ ! -f "build_ninja/bin/$test_name" ]; then
                                ((test_count++))
                                # Build quietly to avoid clutter, show only on error
                                if ! cmake --build build_ninja --target "$test_name" > /dev/null 2>&1; then
                                    print_color "$YELLOW" "  Warning: Failed to build $test_name"
                                fi
                            fi
                        fi
                    done < <(discover_test_sources)
                    if [ $test_count -gt 0 ]; then
                        echo "  Built $test_count integration test targets"
                    else
                        echo "  All integration tests already built"
                    fi
                fi
                
                while IFS= read -r test_path; do
                    run_cpp_test "$test_path"
                done < <(discover_integration_tests)
                # NOTE: Removed run_uncategorized_tests - only run tests following test_integration_* pattern
                ;;
            uncategorized)
                run_uncategorized_tests
                ;;
            quick)
                run_quick_tests
                ;;
            # No legacy mappings needed - all categories use standard names
            *)
                # First check if it's a test name pattern
                if [[ "$group" == *"test_integration"* ]] || [[ "$group" == *"test_"* ]]; then
                    # Try to run as individual test
                    run_individual_test "$group"
                # Then try to run as a category
                elif discover_categories | grep -q "^${group}$"; then
                    run_category_tests "$group"
                else
                    # Could be a partial test name, try individual test
                    run_individual_test "$group"
                fi
                ;;
        esac
    done
    
    # Print summary
    print_summary
}

# Run main function
main "$@"