#!/bin/bash
# Integration tests combining multiple features

set -e
source "$(dirname "$0")/test_lib.sh"

TEST_DIR="$OUTPUT_BASE/rendering/integration"
mkdir -p "$TEST_DIR"

# Test 1: Complete workflow test
init_test "Complete Voxel Editing Workflow"
cat > "$TEST_DIR/workflow_cmd.txt" << EOF
workspace 6.0 6.0 6.0
grid on
view iso
resolution 64cm
place 192cm 0cm 192cm
place 256cm 0cm 192cm
place 192cm 0cm 256cm
screenshot $TEST_DIR/workflow_1.ppm
resolution 64cm
place 192cm 64cm 192cm
place 224cm 64cm 192cm
place 192cm 64cm 224cm
screenshot $TEST_DIR/workflow_2.ppm
resolution 16cm
place 208cm 96cm 208cm
screenshot $TEST_DIR/workflow_3.ppm
undo
undo
undo
screenshot $TEST_DIR/workflow_undo.ppm
redo
redo
screenshot $TEST_DIR/workflow_redo.ppm
save $TEST_DIR/workflow_project.vxl
clear
screenshot $TEST_DIR/workflow_clear.ppm
load $TEST_DIR/workflow_project.vxl
screenshot $TEST_DIR/workflow_loaded.ppm
export $TEST_DIR/workflow_export.stl
status
quit
EOF

if execute_cli "$TEST_DIR/workflow_cmd.txt" 60; then
    # Check all screenshots were created
    all_good=true
    for i in 1 2 3 undo redo clear loaded; do
        if ! analyze_screenshot "$TEST_DIR/workflow_$i" "$TEST_DIR/workflow_${i}_analysis.txt"; then
            fail_test "Workflow screenshot $i failed"
            all_good=false
            break
        fi
    done
    
    if [ "$all_good" = true ]; then
        # Verify clear actually cleared
        if check_visible_content "$TEST_DIR/workflow_clear_analysis.txt" 2; then
            fail_test "Clear command didn't remove voxels"
        else
            # Verify load restored voxels
            if check_visible_content "$TEST_DIR/workflow_loaded_analysis.txt" 5; then
                pass_test "Complete workflow executed successfully"
            else
                fail_test "Load didn't restore voxels"
            fi
        fi
    fi
else
    fail_test "Workflow test failed to execute"
fi

# Test 2: Performance with many voxels
init_test "Performance Test - Many Voxels"
cat > "$TEST_DIR/performance_cmd.txt" << EOF
workspace 8.0 8.0 8.0
grid off
view iso
resolution 16cm
EOF

# Create a 10x10x10 grid of voxels
for x in $(seq 0 16 160); do
    for y in $(seq 0 16 160); do
        for z in $(seq 0 16 160); do
            echo "place ${x}cm ${y}cm ${z}cm" >> "$TEST_DIR/performance_cmd.txt"
        done
    done
done

cat >> "$TEST_DIR/performance_cmd.txt" << EOF
status
zoom 2.5
screenshot $TEST_DIR/performance_1000.ppm
quit
EOF

START_TIME=$(date +%s)
if execute_cli "$TEST_DIR/performance_cmd.txt" 120; then
    END_TIME=$(date +%s)
    ELAPSED=$((END_TIME - START_TIME))
    
    if analyze_screenshot "$TEST_DIR/performance_1000" "$TEST_DIR/performance_analysis.txt"; then
        if check_visible_content "$TEST_DIR/performance_analysis.txt" 10; then
            pass_test "Rendered 1000 voxels in ${ELAPSED}s"
            echo "Performance metric: ${ELAPSED}s for 1000 voxels" > "$OUTPUT_BASE/metrics/performance_1000.txt"
        else
            fail_test "1000 voxels not properly rendered"
        fi
    else
        fail_test "Failed to analyze performance screenshot"
    fi
else
    fail_test "Performance test timed out or crashed"
fi

# Test 3: Complex scene with mixed features
init_test "Complex Scene Composition"
cat > "$TEST_DIR/complex_scene_cmd.txt" << EOF
workspace 8.0 8.0 8.0
grid on
view iso

# Create a base platform
resolution 256cm
place 256cm 0cm 256cm
place 512cm 0cm 256cm
place 256cm 0cm 512cm
place 512cm 0cm 512cm

# Add medium structures
resolution 64cm
place 256cm 256cm 256cm
place 320cm 256cm 256cm
place 384cm 256cm 384cm
place 448cm 256cm 384cm

# Add details
resolution 64cm
place 256cm 320cm 256cm
place 320cm 320cm 256cm
place 384cm 320cm 384cm

# Add fine details
resolution 16cm
place 176cm 224cm 176cm
place 304cm 224cm 304cm

# Different views
screenshot $TEST_DIR/complex_iso.ppm
view front
zoom 1.5
screenshot $TEST_DIR/complex_front.ppm
view top
screenshot $TEST_DIR/complex_top.ppm

# Test edge rendering
render edges
view iso
screenshot $TEST_DIR/complex_edges.ppm
render solid

status
quit
EOF

if execute_cli "$TEST_DIR/complex_scene_cmd.txt" 60; then
    views=("iso" "front" "top" "edges")
    all_good=true
    
    for view in "${views[@]}"; do
        if analyze_screenshot "$TEST_DIR/complex_$view" "$TEST_DIR/complex_${view}_analysis.txt"; then
            if ! check_visible_content "$TEST_DIR/complex_${view}_analysis.txt" 5; then
                fail_test "Complex scene $view view not rendered properly"
                all_good=false
                break
            fi
        else
            fail_test "Failed to analyze complex scene $view view"
            all_good=false
            break
        fi
    done
    
    if [ "$all_good" = true ]; then
        pass_test "Complex scene rendered from all views"
    fi
else
    fail_test "Complex scene test failed"
fi

# Test 4: Stress test undo/redo with rendering
init_test "Undo/Redo Stress with Rendering"
cat > "$TEST_DIR/undo_stress_cmd.txt" << EOF
workspace 5.0 5.0 5.0
view front
resolution 64cm
EOF

# Place 20 voxels
for i in $(seq 0 19); do
    echo "place $((i*64))cm 0cm 64cm" >> "$TEST_DIR/undo_stress_cmd.txt"
done

echo "screenshot $TEST_DIR/undo_stress_full.ppm" >> "$TEST_DIR/undo_stress_cmd.txt"

# Undo all
for i in $(seq 0 19); do
    echo "undo" >> "$TEST_DIR/undo_stress_cmd.txt"
done

echo "screenshot $TEST_DIR/undo_stress_empty.ppm" >> "$TEST_DIR/undo_stress_cmd.txt"

# Redo half
for i in $(seq 0 9); do
    echo "redo" >> "$TEST_DIR/undo_stress_cmd.txt"
done

echo "screenshot $TEST_DIR/undo_stress_half.ppm" >> "$TEST_DIR/undo_stress_cmd.txt"
echo "quit" >> "$TEST_DIR/undo_stress_cmd.txt"

if execute_cli "$TEST_DIR/undo_stress_cmd.txt" 45; then
    if analyze_screenshot "$TEST_DIR/undo_stress_full" "$TEST_DIR/undo_full_analysis.txt" && \
       analyze_screenshot "$TEST_DIR/undo_stress_empty" "$TEST_DIR/undo_empty_analysis.txt" && \
       analyze_screenshot "$TEST_DIR/undo_stress_half" "$TEST_DIR/undo_half_analysis.txt"; then
        
        # Check that we have different numbers of visible elements
        full_colors=$(grep "Unique colors:" "$TEST_DIR/undo_full_analysis.txt" | grep -oE '[0-9]+$')
        empty_colors=$(grep "Unique colors:" "$TEST_DIR/undo_empty_analysis.txt" | grep -oE '[0-9]+$')
        half_colors=$(grep "Unique colors:" "$TEST_DIR/undo_half_analysis.txt" | grep -oE '[0-9]+$')
        
        if [ "$full_colors" -gt "$half_colors" ] && [ "$half_colors" -gt "$empty_colors" ]; then
            pass_test "Undo/redo stress test shows correct progression"
        else
            fail_test "Undo/redo progression incorrect: full=$full_colors, half=$half_colors, empty=$empty_colors"
        fi
    else
        fail_test "Failed to analyze undo/redo screenshots"
    fi
else
    fail_test "Undo/redo stress test failed"
fi

# Test 5: File format compatibility
init_test "File Format Save/Load/Export"
cat > "$TEST_DIR/file_format_cmd.txt" << EOF
workspace 5.0 5.0 5.0
resolution 64cm
place 96cm 0cm 96cm
place 128cm 0cm 96cm
place 96cm 0cm 128cm
place 64cm 64cm 64cm
save $TEST_DIR/format_test.vxl
export $TEST_DIR/format_test.stl
screenshot $TEST_DIR/format_original.ppm
clear
load $TEST_DIR/format_test.vxl
screenshot $TEST_DIR/format_loaded.ppm
quit
EOF

if execute_cli "$TEST_DIR/file_format_cmd.txt" 30; then
    if [ -f "$TEST_DIR/format_test.vxl" ] && [ -f "$TEST_DIR/format_test.stl" ]; then
        # Compare original and loaded screenshots
        if analyze_screenshot "$TEST_DIR/format_original" "$TEST_DIR/format_orig_analysis.txt" && \
           analyze_screenshot "$TEST_DIR/format_loaded" "$TEST_DIR/format_load_analysis.txt"; then
            
            orig_colors=$(grep "Unique colors:" "$TEST_DIR/format_orig_analysis.txt" | grep -oE '[0-9]+$')
            load_colors=$(grep "Unique colors:" "$TEST_DIR/format_load_analysis.txt" | grep -oE '[0-9]+$')
            
            if [ "$orig_colors" -eq "$load_colors" ]; then
                pass_test "File save/load preserves scene correctly"
            else
                fail_test "Loaded scene differs from original"
            fi
        else
            fail_test "Failed to analyze file format screenshots"
        fi
    else
        fail_test "Failed to create save/export files"
    fi
else
    fail_test "File format test failed"
fi

# Print summary
print_summary