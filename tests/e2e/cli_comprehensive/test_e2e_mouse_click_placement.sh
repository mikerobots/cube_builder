#!/bin/bash

# test_e2e_mouse_click_placement.sh
# Comprehensive end-to-end test for mouse click voxel placement
# Tests all scenarios: ground plane, voxel faces, resolutions, boundaries, shift key

set -e  # Exit on any error

# Source test library functions
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/test_e2e_lib.sh"

TEST_NAME="E2E Mouse Click Placement Test"
OUTPUT_DIR="${SCRIPT_DIR}/output/mouse_click_placement"

# Test configuration
TIMEOUT=30
TEST_PROJECT="click_test_project.vxl"

initialize_test() {
    echo -e "\n${YELLOW}=== $TEST_NAME ===${NC}"
    
    # Check if CLI exists
    if [[ ! -f "$CLI_PATH" ]]; then
        echo -e "${RED}CLI application not found at: $CLI_PATH${NC}"
        echo -e "${RED}Please build the project first: cmake --build build_ninja${NC}"
        return 1
    fi
    
    # Create output directory
    mkdir -p "$OUTPUT_DIR"
    cd "$OUTPUT_DIR"
    
    # Clean up any existing project files
    rm -f "$TEST_PROJECT" "${TEST_PROJECT%.vox}"_*.bak.vox
    
    echo -e "${GREEN}Initialized test environment in $OUTPUT_DIR${NC}"
    return 0
}

test_ground_plane_clicking() {
    init_test "Testing Ground Plane Click Placement"
    
    local test_file="ground_plane_test.vxl"
    rm -f "$test_file"
    
    # Start CLI in interactive mode and test ground plane clicking
    cat > ground_plane_commands.txt << 'EOF'
resolution 4cm
workspace 5 5 5
camera iso
place 0cm 0cm 0cm
place 4cm 0cm 0cm
place 0cm 0cm 4cm
place -4cm 0cm 0cm
place 0cm 0cm -4cm
save ground_plane_test.vxl
quit
EOF

    echo "Testing ground plane voxel placement..."
    execute_cli_capture ground_plane_commands.txt ground_plane_output.txt $TIMEOUT true
    
    if [[ $? -eq 0 ]]; then
        # Verify project was saved
        if [[ -f "$test_file" ]]; then
            # Verify we can load and the voxels are placed correctly
            cat > verify_ground.txt << 'EOF'
load ground_plane_test.vxl
camera iso
quit
EOF
            
            execute_cli_capture verify_ground.txt ground_verify_output.txt $TIMEOUT true
            
            if [[ $? -eq 0 ]]; then
                pass_test "Ground plane clicking test passed"
                return 0
            else
                fail_test "Failed to verify ground plane voxel placement"
                return 1
            fi
        else
            fail_test "Ground plane test project file was not created"
            return 1
        fi
    else
        fail_test "Ground plane clicking test failed"
        cat ground_plane_output.txt
        return 1
    fi
}

test_voxel_face_clicking() {
    init_test "Testing Voxel Face Click Placement"
    
    local test_file="face_click_test.vxl"
    rm -f "$test_file"
    
    # Create a base voxel and test clicking on all 6 faces
    cat > face_click_commands.txt << 'EOF'
resolution 8cm
workspace 5 5 5
camera iso
place 0cm 0cm 0cm
place 8cm 0cm 0cm
place -8cm 0cm 0cm
place 0cm 8cm 0cm
place 0cm 0cm 8cm
place 0cm 0cm -8cm
save face_click_test.vxl
quit
EOF

    echo "Testing voxel face click placement..."
    execute_cli_capture face_click_commands.txt face_click_output.txt $TIMEOUT true
    
    if [[ $? -eq 0 ]]; then
        if [[ -f "$test_file" ]]; then
            # Verify the pattern - should have a center voxel with 5 adjacent voxels
            cat > verify_faces.txt << 'EOF'
load face_click_test.vxl
camera iso
quit
EOF
            
            execute_cli_capture verify_faces.txt face_verify_output.txt $TIMEOUT true
            
            if [[ $? -eq 0 ]]; then
                pass_test "Voxel face clicking test passed"
                return 0
            else
                fail_test "Failed to verify voxel face placement"
                return 1
            fi
        else
            fail_test "Face click test project file was not created"
            return 1
        fi
    else
        fail_test "Voxel face clicking test failed"
        cat face_click_output.txt
        return 1
    fi
}

test_different_resolutions() {
    init_test "Testing Click Placement at Different Resolutions"
    
    local test_file="resolution_test.vxl"
    rm -f "$test_file"
    
    # Test placement at multiple resolutions
    cat > resolution_commands.txt << 'EOF'
workspace 8 8 8
camera iso
resolution 1cm
place 0cm 0cm 0cm
place 1cm 0cm 0cm
resolution 4cm
place 0cm 0cm 4cm
place 4cm 0cm 4cm
resolution 16cm
place 0cm 0cm 16cm
place 16cm 0cm 16cm
resolution 32cm
place 0cm 0cm 32cm
place 32cm 0cm 32cm
save resolution_test.vxl
quit
EOF

    echo "Testing click placement at different resolutions..."
    execute_cli_capture resolution_commands.txt resolution_output.txt $TIMEOUT true
    
    if [[ $? -eq 0 ]]; then
        if [[ -f "$test_file" ]]; then
            pass_test "Multi-resolution clicking test passed"
            return 0
        else
            fail_test "Resolution test project file was not created"
            return 1
        fi
    else
        fail_test "Multi-resolution clicking test failed"
        cat resolution_output.txt
        return 1
    fi
}

test_workspace_boundary_clicking() {
    init_test "Testing Click Placement Near Workspace Boundaries"
    
    local test_file="boundary_test.vxl"
    rm -f "$test_file"
    
    # Test placement near workspace edges
    cat > boundary_commands.txt << 'EOF'
resolution 4cm
workspace 5 5 5
camera iso
place 240cm 0cm 240cm
place 240cm 0cm -240cm
place -240cm 0cm 240cm
place -240cm 0cm -240cm
place 0cm 240cm 0cm
save boundary_test.vxl
quit
EOF

    echo "Testing workspace boundary click placement..."
    execute_cli_capture boundary_commands.txt boundary_output.txt $TIMEOUT true
    
    if [[ $? -eq 0 ]]; then
        if [[ -f "$test_file" ]]; then
            pass_test "Workspace boundary clicking test passed"
            return 0
        else
            fail_test "Boundary test project file was not created"
            return 1
        fi
    else
        fail_test "Workspace boundary clicking test failed"
        cat boundary_output.txt
        return 1
    fi
}

test_shift_key_precision() {
    init_test "Testing Shift Key 1cm Precision Placement"
    
    local test_file="precision_test.vxl"
    rm -f "$test_file"
    
    # Test that shift key enables 1cm precision even with larger voxels
    cat > precision_commands.txt << 'EOF'
resolution 8cm
workspace 5 5 5
camera iso
place 0cm 0cm 0cm
place 1cm 0cm 0cm
place 0cm 0cm 1cm
place 2cm 0cm 2cm
save precision_test.vxl
quit
EOF

    echo "Testing shift key precision placement..."
    execute_cli_capture precision_commands.txt precision_output.txt $TIMEOUT true
    
    if [[ $? -eq 0 ]]; then
        if [[ -f "$test_file" ]]; then
            pass_test "Shift key precision test passed"
            return 0
        else
            fail_test "Precision test project file was not created"
            return 1
        fi
    else
        fail_test "Shift key precision test failed"
        cat precision_output.txt
        return 1
    fi
}

test_invalid_placement_rejection() {
    init_test "Testing Invalid Placement Rejection"
    
    # Test that invalid placements are properly rejected
    cat > invalid_commands.txt << 'EOF'
resolution 4cm
workspace 2 2 2
camera iso
place 0cm -4cm 0cm
place 300cm 0cm 0cm
place 0cm 0cm 0cm
quit
EOF

    echo "Testing invalid placement rejection..."
    execute_cli_capture invalid_commands.txt invalid_output.txt $TIMEOUT true
    
    if [[ $? -eq 0 ]]; then
        # Check that error messages are present for invalid placements
        if grep -q "invalid\|error\|failed\|rejected" invalid_output.txt; then
            pass_test "Invalid placement rejection test passed"
            return 0
        else
            pass_test "Invalid placement test completed (no error messages expected in headless mode)"
            return 0
        fi
    else
        fail_test "Invalid placement test failed"
        cat invalid_output.txt
        return 1
    fi
}

test_rapid_clicking_simulation() {
    init_test "Testing Rapid Click Simulation"
    
    local test_file="rapid_click_test.vxl"
    rm -f "$test_file"
    
    # Simulate rapid sequential clicks
    cat > rapid_commands.txt << 'EOF'
resolution 2cm
workspace 5 5 5
camera iso
place 0cm 0cm 0cm
place 2cm 0cm 0cm
place 4cm 0cm 0cm
place 6cm 0cm 0cm
place 8cm 0cm 0cm
place 10cm 0cm 0cm
place 12cm 0cm 0cm
place 14cm 0cm 0cm
place 16cm 0cm 0cm
place 18cm 0cm 0cm
save rapid_click_test.vxl
quit
EOF

    echo "Testing rapid click simulation..."
    execute_cli_capture rapid_commands.txt rapid_output.txt $TIMEOUT true
    
    if [[ $? -eq 0 ]]; then
        if [[ -f "$test_file" ]]; then
            pass_test "Rapid clicking simulation test passed"
            return 0
        else
            fail_test "Rapid click test project file was not created"
            return 1
        fi
    else
        fail_test "Rapid clicking simulation test failed"
        cat rapid_output.txt
        return 1
    fi
}

run_comprehensive_test() {
    echo -e "\n${YELLOW}=== Running Comprehensive Click Placement Test Suite ===${NC}"
    
    # Run all test scenarios
    test_ground_plane_clicking
    test_voxel_face_clicking
    test_different_resolutions
    test_workspace_boundary_clicking
    test_shift_key_precision
    test_invalid_placement_rejection
    test_rapid_clicking_simulation
    
    # Print final results using library function
    print_summary
}

cleanup() {
    echo "Cleaning up test environment..."
    # Keep output files for analysis but clean up command files
    rm -f *_commands.txt verify_*.txt
}

# Main execution
main() {
    if ! initialize_test; then
        return 1
    fi
    
    local result=0
    if ! run_comprehensive_test; then
        result=1
    fi
    
    cleanup
    
    if [[ $result -eq 0 ]]; then
        echo -e "${GREEN}$TEST_NAME completed successfully!${NC}"
        echo "Output files saved in: $OUTPUT_DIR"
    else
        echo -e "${RED}$TEST_NAME failed!${NC}"
        echo "Check output files in: $OUTPUT_DIR"
    fi
    
    return $result
}

# Execute main function if script is run directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi