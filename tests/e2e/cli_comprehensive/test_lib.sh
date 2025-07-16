#!/bin/bash
# Common test library functions

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test result tracking
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0
CURRENT_TEST=""

# Paths
# Get absolute path to script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
CLI_PATH="$ROOT_DIR/build_ninja/bin/VoxelEditor_CLI"
TOOLS_DIR="$ROOT_DIR/tools"
OUTPUT_BASE="$SCRIPT_DIR/output"

# Initialize test environment
init_test() {
    local test_name="$1"
    CURRENT_TEST="$test_name"
    echo -e "\n${YELLOW}=== Running: $test_name ===${NC}"
    TESTS_RUN=$((TESTS_RUN + 1))
}

# Report test pass
pass_test() {
    local message="$1"
    echo -e "${GREEN}✅ PASS${NC}: $message"
    TESTS_PASSED=$((TESTS_PASSED + 1))
}

# Report test fail
fail_test() {
    local message="$1"
    echo -e "${RED}❌ FAIL${NC}: $message"
    TESTS_FAILED=$((TESTS_FAILED + 1))
    return 1
}

# Execute CLI with timeout
execute_cli() {
    local commands_file="$1"
    local timeout_seconds="${2:-30}"
    local headless="${3:-false}"
    
    local args=""
    if [ "$headless" = "true" ]; then
        args="--headless"
    fi
    
    timeout "$timeout_seconds"s "$CLI_PATH" $args < "$commands_file"
}

# Execute CLI and capture output
execute_cli_capture() {
    local commands_file="$1"
    local output_file="$2"
    local timeout_seconds="${3:-30}"
    local headless="${4:-false}"
    
    local args=""
    if [ "$headless" = "true" ]; then
        args="--headless"
    fi
    
    timeout "$timeout_seconds"s "$CLI_PATH" $args < "$commands_file" > "$output_file" 2>&1
}

# Check if output contains expected text
expect_output() {
    local output_file="$1"
    local expected="$2"
    
    if grep -q "$expected" "$output_file"; then
        return 0
    else
        echo "Expected to find: $expected"
        echo "Actual output:"
        cat "$output_file"
        return 1
    fi
}

# Check if output does NOT contain text
expect_no_output() {
    local output_file="$1"
    local unexpected="$2"
    
    if grep -q "$unexpected" "$output_file"; then
        echo "Found unexpected: $unexpected"
        echo "Actual output:"
        cat "$output_file"
        return 1
    else
        return 0
    fi
}

# Analyze PPM screenshot
analyze_screenshot() {
    local screenshot_file="$1"
    local analysis_file="$2"
    
    # Check for various extensions the CLI might create
    local actual_file=""
    for ext in "" ".ppm" ".ppm.ppm"; do
        if [ -f "${screenshot_file}${ext}" ]; then
            actual_file="${screenshot_file}${ext}"
            break
        fi
    done
    
    if [ -z "$actual_file" ]; then
        echo "Screenshot not found: $screenshot_file"
        return 1
    fi
    
    "$TOOLS_DIR/analyze_ppm_colors.py" "$actual_file" > "$analysis_file"
}

# Check if screenshot has visible content
check_visible_content() {
    local analysis_file="$1"
    local min_colors="${2:-2}"
    
    local unique_colors=$(grep "Unique colors:" "$analysis_file" | grep -oE '[0-9]+$')
    
    if [ "$unique_colors" -ge "$min_colors" ]; then
        return 0
    else
        echo "Expected at least $min_colors colors, found $unique_colors"
        return 1
    fi
}

# Compare two screenshots
compare_screenshots() {
    local analysis1="$1"
    local analysis2="$2"
    
    local colors1=$(grep "Unique colors:" "$analysis1" | grep -oE '[0-9]+$')
    local colors2=$(grep "Unique colors:" "$analysis2" | grep -oE '[0-9]+$')
    
    if [ "$colors1" != "$colors2" ]; then
        return 0  # Different
    else
        # Could add more sophisticated comparison here
        return 1  # Same
    fi
}

# Create test command file
create_command_file() {
    local output_file="$1"
    shift
    
    # Write commands to file
    for cmd in "$@"; do
        echo "$cmd" >> "$output_file"
    done
    echo "quit" >> "$output_file"
}

# Clean up test artifacts
cleanup_test() {
    local test_dir="$1"
    rm -f "$test_dir"/*.ppm* "$test_dir"/*.txt
}

# Print test summary
print_summary() {
    echo -e "\n${YELLOW}========================================"
    echo "TEST SUMMARY"
    echo "========================================${NC}"
    echo "Total tests run: $TESTS_RUN"
    echo -e "Passed: ${GREEN}$TESTS_PASSED${NC}"
    echo -e "Failed: ${RED}$TESTS_FAILED${NC}"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "\n${GREEN}🎉 ALL TESTS PASSED!${NC}"
        return 0
    else
        echo -e "\n${RED}💥 SOME TESTS FAILED${NC}"
        return 1
    fi
}

# Ensure output directories exist
setup_output_dirs() {
    mkdir -p "$OUTPUT_BASE/headless"
    mkdir -p "$OUTPUT_BASE/rendering"
    mkdir -p "$OUTPUT_BASE/logs"
    mkdir -p "$OUTPUT_BASE/metrics"
}