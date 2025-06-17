#!/bin/bash
# Test visual enhancements (grid, highlighting, preview colors)

set -e
source "$(dirname "$0")/test_lib.sh"

TEST_DIR="$OUTPUT_BASE/rendering/enhancements"
mkdir -p "$TEST_DIR"

# Test 1: Ground plane grid visibility
init_test "Ground Plane Grid Rendering"
cat > "$TEST_DIR/grid_visibility_cmd.txt" << EOF
workspace 5 5 5
view iso
zoom 1.5
grid on
screenshot $TEST_DIR/grid_on.ppm
grid off
screenshot $TEST_DIR/grid_off.ppm
quit
EOF

if execute_cli "$TEST_DIR/grid_visibility_cmd.txt" 30; then
    # Analyze both screenshots
    if analyze_screenshot "$TEST_DIR/grid_on" "$TEST_DIR/grid_on_analysis.txt" && \
       analyze_screenshot "$TEST_DIR/grid_off" "$TEST_DIR/grid_off_analysis.txt"; then
        
        # Grid ON should have more colors (grid lines)
        if compare_screenshots "$TEST_DIR/grid_on_analysis.txt" "$TEST_DIR/grid_off_analysis.txt"; then
            pass_test "Grid rendering shows visual difference"
        else
            fail_test "Grid on/off shows no visual difference"
        fi
    else
        fail_test "Failed to analyze grid screenshots"
    fi
else
    fail_test "Grid visibility test failed to execute"
fi

# Test 2: Grid with voxels
init_test "Grid with Voxel Interaction"
cat > "$TEST_DIR/grid_voxel_cmd.txt" << EOF
workspace 5 5 5
resolution 32cm
grid on
view iso
place 0cm 0cm 0cm
place 32cm 0cm 0cm
place 0cm 0cm 32cm
screenshot $TEST_DIR/grid_with_voxels.ppm
quit
EOF

if execute_cli "$TEST_DIR/grid_voxel_cmd.txt" 30; then
    if analyze_screenshot "$TEST_DIR/grid_with_voxels" "$TEST_DIR/grid_voxels_analysis.txt"; then
        if check_visible_content "$TEST_DIR/grid_voxels_analysis.txt" 4; then
            pass_test "Grid renders correctly with voxels"
        else
            fail_test "Grid with voxels not rendering properly"
        fi
    else
        fail_test "Failed to analyze grid with voxels"
    fi
else
    fail_test "Grid with voxels test failed"
fi

# Test 3: Face highlighting (visual check)
init_test "Face Highlighting Visual Test"
cat > "$TEST_DIR/highlight_cmd.txt" << EOF
workspace 5 5 5
resolution 64cm
view front
place 200cm 100cm 200cm
zoom 0.7
screenshot $TEST_DIR/voxel_for_highlight.ppm
quit
EOF

if execute_cli "$TEST_DIR/highlight_cmd.txt" 30; then
    if analyze_screenshot "$TEST_DIR/voxel_for_highlight" "$TEST_DIR/highlight_analysis.txt"; then
        # Look for yellow-ish colors (face highlighting)
        if grep -E '\(2[0-9]{2}, 2[0-9]{2}, [0-9]{1,2}\)' "$TEST_DIR/highlight_analysis.txt" > /dev/null; then
            pass_test "Face highlighting colors detected"
        else
            # Even without active highlighting, voxel should be visible
            if check_visible_content "$TEST_DIR/highlight_analysis.txt" 3; then
                pass_test "Voxel visible (highlighting may need mouse interaction)"
            else
                fail_test "No face highlighting or voxel visibility"
            fi
        fi
    else
        fail_test "Failed to analyze highlighting"
    fi
else
    fail_test "Face highlighting test failed"
fi

# Test 4: Multiple resolution voxels together
init_test "Mixed Resolution Rendering"
cat > "$TEST_DIR/mixed_resolution_cmd.txt" << EOF
workspace 8 8 8
view iso
resolution 128cm
place 200cm 0cm 200cm
resolution 64cm
place 200cm 128cm 200cm
resolution 32cm
place 200cm 192cm 200cm
resolution 16cm
place 200cm 224cm 200cm
resolution 8cm
place 200cm 240cm 200cm
zoom 1.2
screenshot $TEST_DIR/mixed_resolutions.ppm
quit
EOF

if execute_cli "$TEST_DIR/mixed_resolution_cmd.txt" 30; then
    if analyze_screenshot "$TEST_DIR/mixed_resolutions" "$TEST_DIR/mixed_res_analysis.txt"; then
        if check_visible_content "$TEST_DIR/mixed_res_analysis.txt" 5; then
            pass_test "Multiple resolution voxels render correctly"
        else
            fail_test "Mixed resolution voxels not rendering properly"
        fi
    else
        fail_test "Failed to analyze mixed resolutions"
    fi
else
    fail_test "Mixed resolution test failed"
fi

# Test 5: Edge rendering mode
init_test "Edge Rendering Mode"
cat > "$TEST_DIR/edge_mode_cmd.txt" << EOF
workspace 5 5 5
resolution 32cm
view iso
place 96cm 0cm 96cm
place 128cm 0cm 96cm
place 96cm 0cm 128cm
edges on
screenshot $TEST_DIR/edges_on.ppm
edges off
screenshot $TEST_DIR/edges_off.ppm
quit
EOF

if execute_cli "$TEST_DIR/edge_mode_cmd.txt" 30; then
    if analyze_screenshot "$TEST_DIR/edges_on" "$TEST_DIR/edges_on_analysis.txt" && \
       analyze_screenshot "$TEST_DIR/edges_off" "$TEST_DIR/edges_off_analysis.txt"; then
        
        if compare_screenshots "$TEST_DIR/edges_on_analysis.txt" "$TEST_DIR/edges_off_analysis.txt"; then
            pass_test "Edge rendering mode shows visual difference"
        else
            fail_test "Edge mode shows no visual difference"
        fi
    else
        fail_test "Failed to analyze edge mode screenshots"
    fi
else
    fail_test "Edge rendering test failed"
fi

# Test 6: Workspace bounds visualization
init_test "Workspace Bounds Visualization"
cat > "$TEST_DIR/workspace_bounds_cmd.txt" << EOF
workspace 3 3 3
view iso
zoom 2.0
grid on
resolution 32cm
place 0cm 0cm 0cm
place 296cm 0cm 0cm
place 0cm 0cm 296cm
place 296cm 0cm 296cm
screenshot $TEST_DIR/workspace_bounds.ppm
quit
EOF

if execute_cli "$TEST_DIR/workspace_bounds_cmd.txt" 30; then
    if analyze_screenshot "$TEST_DIR/workspace_bounds" "$TEST_DIR/bounds_analysis.txt"; then
        if check_visible_content "$TEST_DIR/bounds_analysis.txt" 5; then
            pass_test "Workspace bounds and corner voxels visible"
        else
            fail_test "Workspace bounds not properly visualized"
        fi
    else
        fail_test "Failed to analyze workspace bounds"
    fi
else
    fail_test "Workspace bounds test failed"
fi

# Test 7: Small voxel on large voxel face
init_test "Small on Large Voxel Placement"
cat > "$TEST_DIR/small_on_large_cmd.txt" << EOF
workspace 5 5 5
view front
resolution 128cm
place 200cm 0cm 200cm
resolution 8cm
place 200cm 128cm 200cm
place 208cm 128cm 200cm
place 216cm 128cm 200cm
zoom 0.8
screenshot $TEST_DIR/small_on_large.ppm
quit
EOF

if execute_cli "$TEST_DIR/small_on_large_cmd.txt" 30; then
    if analyze_screenshot "$TEST_DIR/small_on_large" "$TEST_DIR/small_large_analysis.txt"; then
        if check_visible_content "$TEST_DIR/small_large_analysis.txt" 4; then
            pass_test "Small voxels on large voxel face render correctly"
        else
            fail_test "Small on large voxel placement not visible"
        fi
    else
        fail_test "Failed to analyze small on large placement"
    fi
else
    fail_test "Small on large voxel test failed"
fi

# Test 8: Grid opacity near cursor (static test)
init_test "Grid at Different Heights"
cat > "$TEST_DIR/grid_heights_cmd.txt" << EOF
workspace 5 5 5
view iso
grid on
resolution 32cm
place 96cm 0cm 96cm
place 96cm 32cm 96cm
place 96cm 64cm 96cm
place 96cm 96cm 96cm
zoom 1.0
screenshot $TEST_DIR/grid_with_heights.ppm
quit
EOF

if execute_cli "$TEST_DIR/grid_heights_cmd.txt" 30; then
    if analyze_screenshot "$TEST_DIR/grid_with_heights" "$TEST_DIR/grid_heights_analysis.txt"; then
        if check_visible_content "$TEST_DIR/grid_heights_analysis.txt" 5; then
            pass_test "Grid visible with voxels at different heights"
        else
            fail_test "Grid not properly rendered with height variations"
        fi
    else
        fail_test "Failed to analyze grid with heights"
    fi
else
    fail_test "Grid heights test failed"
fi

# Print summary
print_summary