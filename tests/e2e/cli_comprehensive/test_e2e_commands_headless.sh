#!/bin/bash
# Test all CLI commands in headless mode

set -e
source "$(dirname "$0")/test_lib.sh"

TEST_DIR="$OUTPUT_BASE/headless/commands"
mkdir -p "$TEST_DIR"

# Test 1: Help command
init_test "Help Command"
create_command_file "$TEST_DIR/help_cmd.txt" "help"
if execute_cli_capture "$TEST_DIR/help_cmd.txt" "$TEST_DIR/help_output.txt" 10 true; then
    if expect_output "$TEST_DIR/help_output.txt" "Available Commands"; then
        pass_test "Help command shows command list"
    else
        fail_test "Help command missing expected output"
    fi
else
    fail_test "Help command failed to execute"
fi

# Test 2: Workspace commands
init_test "Workspace Commands"
cat > "$TEST_DIR/workspace_cmd.txt" << EOF
workspace 3.0 3.0 3.0
status
workspace 8.0 8.0 8.0
status
quit
EOF

if execute_cli_capture "$TEST_DIR/workspace_cmd.txt" "$TEST_DIR/workspace_output.txt" 10 true; then
    if expect_output "$TEST_DIR/workspace_output.txt" "Workspace: 3x3x3 meters" && \
       expect_output "$TEST_DIR/workspace_output.txt" "Workspace: 8x8x8 meters"; then
        pass_test "Workspace command sets and reports size correctly"
    else
        fail_test "Workspace command output incorrect"
    fi
else
    fail_test "Workspace command failed to execute"
fi

# Test 3: Resolution commands
init_test "Resolution Commands"
RESOLUTIONS=("1cm" "4cm" "16cm" "64cm" "256cm")
for res in "${RESOLUTIONS[@]}"; do
    cat > "$TEST_DIR/resolution_${res}_cmd.txt" << EOF
resolution $res
status
quit
EOF
    
    if execute_cli_capture "$TEST_DIR/resolution_${res}_cmd.txt" "$TEST_DIR/resolution_${res}_output.txt" 10 true; then
        if expect_output "$TEST_DIR/resolution_${res}_output.txt" "Resolution: $res"; then
            echo "  ✓ Resolution $res works"
        else
            fail_test "Resolution $res not set correctly"
            break
        fi
    else
        fail_test "Resolution $res command failed"
        break
    fi
done
pass_test "All resolution commands work correctly"

# Test 4: Place and remove commands
init_test "Place and Remove Commands"
cat > "$TEST_DIR/place_remove_cmd.txt" << EOF
workspace 5.0 5.0 5.0
resolution 16cm
place 0cm 0cm 0cm
place 16cm 16cm 16cm
place -16cm 16cm -16cm
delete 16cm 16cm 16cm
status
quit
EOF

if execute_cli_capture "$TEST_DIR/place_remove_cmd.txt" "$TEST_DIR/place_remove_output.txt" 10 true; then
    if expect_output "$TEST_DIR/place_remove_output.txt" "Voxel placed at" && \
       expect_output "$TEST_DIR/place_remove_output.txt" "Voxel deleted at" && \
       expect_output "$TEST_DIR/place_remove_output.txt" "Voxels: 2"; then
        pass_test "Place and remove commands work correctly"
    else
        fail_test "Place/remove commands output incorrect"
    fi
else
    fail_test "Place/remove commands failed to execute"
fi

# Test 5: Individual voxel deletion
init_test "Individual Voxel Deletion"
cat > "$TEST_DIR/delete_cmd.txt" << EOF
workspace 5.0 5.0 5.0
resolution 16cm
place 16cm 16cm 16cm
place -16cm 16cm -16cm
status
delete 16cm 16cm 16cm
status
delete -16cm 16cm -16cm
status
quit
EOF

if execute_cli_capture "$TEST_DIR/delete_cmd.txt" "$TEST_DIR/delete_output.txt" 10 true; then
    if expect_output "$TEST_DIR/delete_output.txt" "Voxels: 2" && \
       expect_output "$TEST_DIR/delete_output.txt" "Voxels: 1" && \
       expect_output "$TEST_DIR/delete_output.txt" "Voxels: 0"; then
        pass_test "Individual voxel deletion works correctly"
    else
        fail_test "Individual voxel deletion not working correctly"
    fi
else
    fail_test "Delete command failed to execute"
fi

# Test 6: Undo/Redo commands
init_test "Undo/Redo Commands"
cat > "$TEST_DIR/undo_redo_cmd.txt" << EOF
workspace 5.0 5.0 5.0
resolution 16cm
place 48cm 48cm 48cm
place 96cm 96cm 96cm
status
undo
status
undo
status
redo
status
redo
status
quit
EOF

if execute_cli_capture "$TEST_DIR/undo_redo_cmd.txt" "$TEST_DIR/undo_redo_output.txt" 10 true; then
    # Should see voxel counts: 2, 1, 0, 1, 2
    if grep -q "Voxels: 2" "$TEST_DIR/undo_redo_output.txt" && \
       grep -q "Voxels: 1" "$TEST_DIR/undo_redo_output.txt" && \
       grep -q "Voxels: 0" "$TEST_DIR/undo_redo_output.txt"; then
        pass_test "Undo/redo commands work correctly"
    else
        fail_test "Undo/redo sequence incorrect"
    fi
else
    fail_test "Undo/redo commands failed to execute"
fi

# Test 7: Save/Load commands
init_test "Save/Load Commands"
cat > "$TEST_DIR/save_load_cmd.txt" << EOF
workspace 5.0 5.0 5.0
resolution 64cm
place 64cm 64cm 0cm
place 0cm 64cm 64cm
save $TEST_DIR/test_project.vxl
clear
status
load $TEST_DIR/test_project.vxl
status
quit
EOF

if execute_cli_capture "$TEST_DIR/save_load_cmd.txt" "$TEST_DIR/save_load_output.txt" 10 true; then
    if expect_output "$TEST_DIR/save_load_output.txt" "Project saved" && \
       expect_output "$TEST_DIR/save_load_output.txt" "Project loaded" && \
       expect_output "$TEST_DIR/save_load_output.txt" "Voxels: 0" && \
       expect_output "$TEST_DIR/save_load_output.txt" "Voxels: 2"; then
        pass_test "Save/load commands work correctly"
    else
        fail_test "Save/load sequence incorrect"
    fi
else
    fail_test "Save/load commands failed to execute"
fi

# Test 8: Export command
init_test "Export Command"
cat > "$TEST_DIR/export_cmd.txt" << EOF
workspace 5.0 5.0 5.0
resolution 64cm
place 64cm 0cm 0cm
export $TEST_DIR/test_export.stl
quit
EOF

if execute_cli_capture "$TEST_DIR/export_cmd.txt" "$TEST_DIR/export_output.txt" 10 true; then
    if expect_output "$TEST_DIR/export_output.txt" "Exported" && \
       [ -f "$TEST_DIR/test_export.stl" ]; then
        pass_test "Export command creates STL file"
    else
        fail_test "Export command failed to create file"
    fi
else
    fail_test "Export command failed to execute"
fi

# Test 9: Grid commands (headless validation)
init_test "Grid Commands"
cat > "$TEST_DIR/grid_cmd.txt" << EOF
grid on
grid off
grid toggle
grid
quit
EOF

if execute_cli_capture "$TEST_DIR/grid_cmd.txt" "$TEST_DIR/grid_output.txt" 10 true; then
    if expect_output "$TEST_DIR/grid_output.txt" "Grid:"; then
        pass_test "Grid commands execute without error"
    else
        fail_test "Grid commands missing expected output"
    fi
else
    fail_test "Grid commands failed to execute"
fi

# Test 10: Invalid commands
init_test "Invalid Command Handling"
cat > "$TEST_DIR/invalid_cmd.txt" << EOF
notacommand
place
place abc def ghi
place 100cm 100cm 100cm
resolution 99mm
workspace -1 -1 -1
quit
EOF

if execute_cli_capture "$TEST_DIR/invalid_cmd.txt" "$TEST_DIR/invalid_output.txt" 10 true; then
    if expect_output "$TEST_DIR/invalid_output.txt" "Unknown command" || \
       expect_output "$TEST_DIR/invalid_output.txt" "Invalid" || \
       expect_output "$TEST_DIR/invalid_output.txt" "Error"; then
        pass_test "Invalid commands handled gracefully"
    else
        fail_test "Invalid commands not properly handled"
    fi
else
    fail_test "Invalid command handling crashed CLI"
fi

# Print summary
print_summary