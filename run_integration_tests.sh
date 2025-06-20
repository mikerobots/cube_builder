#!/bin/bash

# Integration Test Runner - C++ Tests Only
# Runs integration tests that verify cross-component functionality

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

# Function to run a C++ test executable
run_cpp_test() {
    local test_executable=$1
    local test_name=$(basename "$test_executable")
    
    if [ ! -f "$test_executable" ]; then
        print_result "$test_name" "SKIP" "0"
        return
    fi
    
    local start_time=$(date +%s)
    
    if timeout 60 $test_executable > "/tmp/${test_name}.log" 2>&1; then
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
    print_header "Available Integration Test Groups"
    
    printf "Integration Groups:\n"
    printf "  ${BOLD}core${NC}          - Core cross-component integration tests\n"
    printf "  ${BOLD}cli-cpp${NC}       - CLI application integration tests (C++)\n"
    printf "  ${BOLD}interaction${NC}   - Mouse and keyboard interaction tests\n"
    printf "  ${BOLD}shader${NC}        - Shader integration tests\n"
    printf "  ${BOLD}visual-feedback${NC} - Visual feedback system integration\n"
    printf "  ${BOLD}verification${NC}  - Core functionality verification tests\n"
    printf "\n"
    printf "Meta Groups:\n"
    printf "  ${BOLD}all${NC}           - Run all integration tests\n"
    printf "  ${BOLD}quick${NC}         - Run quick integration tests only\n"
    printf "\n"
    printf "Usage: $0 [group1] [group2] ...\n"
    printf "       $0 list    - Show this help\n"
    printf "       $0         - Show this help\n"
    printf "\n"
    printf "Note: For CLI end-to-end tests, use ./run_e2e_tests.sh\n"
}

# Function to run core integration tests
run_core_tests() {
    print_header "Core Integration Tests"
    
    # Build tests if needed
    if [ -d "build_ninja" ]; then
        print_color "$CYAN" "Building core integration tests..."
        cmake --build build_ninja --target test_ground_plane_voxel_placement test_workspace_boundary_placement test_mouse_boundary_clicking test_mouse_ground_plane_clicking test_camera_cube_visibility test_camera_cube_visibility_simple 2>/dev/null || true
    fi
    
    # Run each test executable from bin directory
    local test_executables=(
        "build_ninja/bin/test_camera_cube_visibility"
        "build_ninja/bin/test_camera_cube_visibility_simple" 
        "build_ninja/bin/test_ground_plane_voxel_placement"
        "build_ninja/bin/test_workspace_boundary_placement"
        "build_ninja/bin/test_mouse_boundary_clicking"
        "build_ninja/bin/test_mouse_ground_plane_clicking"
    )
    
    for test in "${test_executables[@]}"; do
        if [ -x "$test" ]; then
            run_cpp_test "$test"
        fi
    done
}

# Function to run CLI C++ integration tests
run_cli_cpp_tests() {
    print_header "CLI C++ Integration Tests"
    
    # Build CLI tests if needed
    if [ -d "build_ninja" ]; then
        print_color "$CYAN" "Building CLI integration tests..."
        # Build individual CLI test targets
        local cli_targets=(
            test_cli_commands
            test_cli_error_handling
            test_cli_headless
            test_cli_rendering
            test_cli_rendering_basic
            test_click_voxel_placement
            test_face_clicking
            test_mouse_ray_movement
            test_voxel_face_clicking
            test_voxel_face_clicking_simple
            test_zoom_behavior
        )
        
        for target in "${cli_targets[@]}"; do
            cmake --build build_ninja --target "$target" 2>/dev/null || true
        done
    fi
    
    # Run CLI test executables from bin directory
    local test_executables=(
        "build_ninja/bin/test_cli_commands"
        "build_ninja/bin/test_cli_error_handling"
        "build_ninja/bin/test_cli_headless"
        "build_ninja/bin/test_cli_rendering"
        "build_ninja/bin/test_cli_rendering_basic"
        "build_ninja/bin/test_click_voxel_placement"
        "build_ninja/bin/test_face_clicking"
        "build_ninja/bin/test_mouse_ray_movement"
        "build_ninja/bin/test_voxel_face_clicking"
        "build_ninja/bin/test_voxel_face_clicking_simple"
        "build_ninja/bin/test_zoom_behavior"
    )
    
    for test in "${test_executables[@]}"; do
        if [ -x "$test" ]; then
            run_cpp_test "$test"
        fi
    done
}

# Function to run interaction tests
run_interaction_tests() {
    print_header "Interaction Integration Tests"
    
    # Mouse and keyboard interaction tests
    local cpp_tests=(
        "build_ninja/bin/test_click_voxel_placement"
        "build_ninja/bin/test_face_clicking"
        "build_ninja/bin/test_mouse_ray_movement"
        "build_ninja/bin/test_voxel_face_clicking"
        "build_ninja/bin/test_zoom_behavior"
        "build_ninja/bin/test_mouse_ground_plane_clicking"
        "build_ninja/bin/test_mouse_boundary_clicking"
    )
    
    for test in "${cpp_tests[@]}"; do
        if [ -x "$test" ]; then
            run_cpp_test "$test"
        fi
    done
}

# Function to run shader tests
run_shader_tests() {
    print_header "Shader Integration Tests"
    
    # Build shader tests if needed
    if [ -d "build_ninja" ]; then
        print_color "$CYAN" "Building shader tests..."
        cmake --build build_ninja --target ShaderTest 2>/dev/null || true
    fi
    
    local test_executable="build_ninja/bin/ShaderTest"
    if [ -x "$test_executable" ]; then
        run_cpp_test "$test_executable"
    fi
}

# Function to run visual feedback integration tests
run_visual_feedback_tests() {
    print_header "Visual Feedback Integration Tests"
    
    # Build visual feedback integration tests
    if [ -d "build_ninja" ]; then
        print_color "$CYAN" "Building visual feedback integration tests..."
        cmake --build build_ninja --target test_feedback_renderer_integration test_overlay_renderer_integration test_visual_feedback_requirements_integration 2>/dev/null || true
    fi
    
    local test_executables=(
        "build_ninja/bin/test_feedback_renderer_integration"
        "build_ninja/bin/test_overlay_renderer_integration"
        "build_ninja/bin/test_visual_feedback_requirements_integration"
    )
    
    for test in "${test_executables[@]}"; do
        if [ -x "$test" ]; then
            run_cpp_test "$test"
        fi
    done
}

# Function to run verification tests
run_verification_tests() {
    print_header "Core Functionality Verification Tests"
    
    # Build verification tests if needed
    if [ -d "build_ninja" ]; then
        print_color "$CYAN" "Building verification tests..."
        cmake --build build_ninja --target test_core_functionality 2>/dev/null || true
    fi
    
    local test_executable="build_ninja/bin/test_core_functionality"
    if [ -x "$test_executable" ]; then
        run_cpp_test "$test_executable"
    fi
}

# Function to run quick integration tests
run_quick_tests() {
    print_header "Quick Integration Tests"
    
    # Run only the essential quick integration tests
    local quick_tests=(
        "build_ninja/bin/test_camera_cube_visibility_simple"
        "build_ninja/bin/test_cli_rendering_basic"
        "build_ninja/bin/test_core_functionality"
    )
    
    for test in "${quick_tests[@]}"; do
        if [ -x "$test" ]; then
            run_cpp_test "$test"
        fi
    done
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
            core)
                run_core_tests
                ;;
            cli-cpp)
                run_cli_cpp_tests
                ;;
            interaction)
                run_interaction_tests
                ;;
            shader)
                run_shader_tests
                ;;
            visual-feedback)
                run_visual_feedback_tests
                ;;
            verification)
                run_verification_tests
                ;;
            quick)
                run_quick_tests
                ;;
            all)
                run_core_tests
                run_cli_cpp_tests
                run_interaction_tests
                run_shader_tests
                run_visual_feedback_tests
                run_verification_tests
                ;;
            *)
                print_color "$RED" "Unknown integration test group: $group"
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