#!/bin/bash

# Performance Test for Voxel Editor CLI
# Tests system performance with large voxel counts

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${GREEN}=== Voxel Editor Performance Test ===${NC}"
echo

# Check if executable exists
if [ ! -f "build/bin/VoxelEditor_CLI" ]; then
    echo -e "${RED}Error: VoxelEditor_CLI not found. Please run ./build.sh first.${NC}"
    exit 1
fi

# Test 1: Large Voxel Count
echo -e "${YELLOW}Test 1: Large Voxel Count (10,000 voxels)${NC}"
cat > perf_test1.txt << EOF
new
resolution 16cm
fill 0 0 0 20 20 25
status
save perf_test1.cvef
quit
EOF

time ./build/bin/VoxelEditor_CLI < perf_test1.txt > perf1.log 2>&1
echo -e "${GREEN}✓ Completed${NC}"
grep "Voxels:" perf1.log || echo "Voxel count not found"
echo

# Test 2: Many Groups
echo -e "${YELLOW}Test 2: Many Groups (100 groups)${NC}"
cat > perf_test2.txt << EOF
new
resolution 32cm
EOF

# Generate 100 groups
for i in {1..100}; do
    echo "place $i 0 0" >> perf_test2.txt
    echo "select $i 0 0" >> perf_test2.txt
    echo "group \"group_$i\"" >> perf_test2.txt
    echo "selectnone" >> perf_test2.txt
done

echo "groups" >> perf_test2.txt
echo "status" >> perf_test2.txt
echo "quit" >> perf_test2.txt

time ./build/bin/VoxelEditor_CLI < perf_test2.txt > perf2.log 2>&1
echo -e "${GREEN}✓ Completed${NC}"
echo

# Test 3: Undo/Redo Stress
echo -e "${YELLOW}Test 3: Undo/Redo Stress (1000 operations)${NC}"
cat > perf_test3.txt << EOF
new
resolution 8cm
EOF

# Generate 500 place operations
for i in {0..499}; do
    x=$((i % 10))
    y=$((i / 10 % 10))
    z=$((i / 100))
    echo "place $x $y $z" >> perf_test3.txt
done

# Undo all
for i in {1..500}; do
    echo "undo" >> perf_test3.txt
done

# Redo half
for i in {1..250}; do
    echo "redo" >> perf_test3.txt
done

echo "status" >> perf_test3.txt
echo "quit" >> perf_test3.txt

time ./build/bin/VoxelEditor_CLI < perf_test3.txt > perf3.log 2>&1
echo -e "${GREEN}✓ Completed${NC}"
echo

# Test 4: File I/O Performance
echo -e "${YELLOW}Test 4: File I/O Performance${NC}"
cat > perf_test4.txt << EOF
new
resolution 32cm
fill 0 0 0 30 30 30
save large_project.cvef
new
open large_project.cvef
export large_project.stl
quit
EOF

time ./build/bin/VoxelEditor_CLI < perf_test4.txt > perf4.log 2>&1
echo -e "${GREEN}✓ Completed${NC}"

# Check file sizes
if [ -f "large_project.cvef" ]; then
    size=$(du -h large_project.cvef | cut -f1)
    echo "Project file size: $size"
fi
if [ -f "large_project.stl" ]; then
    size=$(du -h large_project.stl | cut -f1)
    echo "STL file size: $size"
fi
echo

# Test 5: Multi-Resolution Stress
echo -e "${YELLOW}Test 5: Multi-Resolution Stress${NC}"
cat > perf_test5.txt << EOF
new
resolution 1cm
fill 0 0 0 10 10 10
resolution 8cm
fill 20 0 0 30 10 10
resolution 64cm
fill 0 20 0 10 30 10
resolution 512cm
place 0 0 20
status
quit
EOF

time ./build/bin/VoxelEditor_CLI < perf_test5.txt > perf5.log 2>&1
echo -e "${GREEN}✓ Completed${NC}"
echo

# Clean up
echo -e "${BLUE}Cleaning up test files...${NC}"
rm -f perf_test*.txt perf*.log *.cvef *.stl

echo
echo -e "${GREEN}=== Performance Tests Complete ===${NC}"
echo
echo "Summary:"
echo "- Large voxel count handling ✓"
echo "- Many groups management ✓"
echo "- Undo/redo performance ✓"
echo "- File I/O performance ✓"
echo "- Multi-resolution stress ✓"
echo
echo -e "${BLUE}Note: Times shown include application startup/shutdown${NC}"