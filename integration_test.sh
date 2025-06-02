#!/bin/bash

# Integration Test for Voxel Editor CLI
# This script tests various command sequences to verify integration

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}=== Voxel Editor CLI Integration Test ===${NC}"
echo

# Check if executable exists
if [ ! -f "build/bin/VoxelEditor_CLI" ]; then
    echo -e "${RED}Error: VoxelEditor_CLI not found. Please run ./build.sh first.${NC}"
    exit 1
fi

# Function to run a test
run_test() {
    local test_name=$1
    local commands_file=$2
    
    echo -e "${YELLOW}Running test: $test_name${NC}"
    
    if ./build/bin/VoxelEditor_CLI < $commands_file > test_output.log 2>&1; then
        echo -e "${GREEN}✓ $test_name passed${NC}"
        return 0
    else
        echo -e "${RED}✗ $test_name failed${NC}"
        echo "Output:"
        tail -20 test_output.log
        return 1
    fi
}

# Test 1: Basic Commands
cat > test1_basic.txt << EOF
help
status
quit
EOF

run_test "Basic Commands" test1_basic.txt

# Test 2: Voxel Operations
cat > test2_voxels.txt << EOF
new
resolution 8cm
place 0 0 0
place 1 0 0
place 2 0 0
place 0 1 0
place 0 2 0
status
undo
undo
status
redo
status
quit
EOF

run_test "Voxel Operations" test2_voxels.txt

# Test 3: Selection and Groups
cat > test3_groups.txt << EOF
new
resolution 16cm
fill 0 0 0 5 5 0
selectbox 0 0 0 2 2 0
group "floor_section"
selectnone
selectbox 3 0 0 5 2 0
group "wall_section"
groups
hide floor_section
groups
show floor_section
quit
EOF

run_test "Selection and Groups" test3_groups.txt

# Test 4: Camera Controls
cat > test4_camera.txt << EOF
new
place 0 0 0
place 1 0 0
place 0 1 0
place 0 0 1
camera front
camera top
camera iso
zoom 1.5
zoom 0.8
rotate 45 0
resetview
quit
EOF

run_test "Camera Controls" test4_camera.txt

# Test 5: File Operations
cat > test5_files.txt << EOF
new
resolution 32cm
fill 0 0 0 3 3 3
save test_integration.cvef
new
status
open test_integration.cvef
status
export test_integration.stl
quit
EOF

run_test "File Operations" test5_files.txt

# Test 6: Complex Workflow
cat > test6_complex.txt << EOF
new
workspace 6 6 6
resolution 64cm
fill 0 0 0 5 0 5
fill 0 0 0 0 3 5
fill 5 0 0 5 3 5
fill 0 0 5 5 0 5
selectall
group "house_walls"
resolution 32cm
fill 1 0 1 4 0 1
fill 1 0 4 4 0 4
selectbox 1 0 1 4 0 4
group "house_interior"
resolution 16cm
place 2 0 0
place 3 0 0
selectbox 2 0 0 3 0 0
group "door"
groups
camera iso
zoom 0.7
save house_project.cvef
quit
EOF

run_test "Complex Workflow" test6_complex.txt

# Test 7: Multi-Resolution
cat > test7_multireso.txt << EOF
new
resolution 512cm
place 0 0 0
resolution 256cm
place 1 0 0
resolution 128cm
place 2 0 0
resolution 64cm
place 3 0 0
resolution 32cm
place 4 0 0
resolution 16cm
place 5 0 0
resolution 8cm
place 6 0 0
resolution 4cm
place 7 0 0
resolution 2cm
place 8 0 0
resolution 1cm
place 9 0 0
status
quit
EOF

run_test "Multi-Resolution" test7_multireso.txt

# Clean up
echo
echo -e "${GREEN}Cleaning up test files...${NC}"
rm -f test*.txt test_output.log test_integration.cvef test_integration.stl house_project.cvef

echo
echo -e "${GREEN}=== Integration Tests Complete ===${NC}"
echo

# Summary
echo "Test Summary:"
echo "- Basic command processing ✓"
echo "- Voxel placement and undo/redo ✓"
echo "- Selection and group management ✓"
echo "- Camera control ✓"
echo "- File save/load/export ✓"
echo "- Complex workflows ✓"
echo "- Multi-resolution support ✓"