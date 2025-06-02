#!/bin/bash

# Project Structure Validation Script

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}=== Voxel Editor Project Validation ===${NC}"
echo

errors=0
warnings=0

# Function to check if file exists
check_file() {
    if [ -f "$1" ]; then
        echo -e "${GREEN}✓${NC} $1"
    else
        echo -e "${RED}✗${NC} $1 - Missing"
        ((errors++))
    fi
}

# Function to check if directory exists
check_dir() {
    if [ -d "$1" ]; then
        echo -e "${GREEN}✓${NC} $1/"
    else
        echo -e "${RED}✗${NC} $1/ - Missing"
        ((errors++))
    fi
}

# Function to count files
count_files() {
    local count=$(find "$1" -name "$2" -type f 2>/dev/null | wc -l)
    echo "$count"
}

echo -e "${YELLOW}Checking project structure...${NC}"

# Root files
check_file "CMakeLists.txt"
check_file "README.md"
check_file "DESIGN.md"
check_file "ARCHITECTURE.md"
check_file "TODO.md"
check_file "build.sh"

# Foundation directories
echo
echo -e "${YELLOW}Foundation Layer:${NC}"
check_dir "foundation"
check_dir "foundation/math"
check_dir "foundation/events"
check_dir "foundation/memory"
check_dir "foundation/logging"
check_dir "foundation/config"

# Core directories
echo
echo -e "${YELLOW}Core Layer:${NC}"
check_dir "core"
check_dir "core/voxel_data"
check_dir "core/camera"
check_dir "core/rendering"
check_dir "core/input"
check_dir "core/selection"
check_dir "core/undo_redo"
check_dir "core/surface_gen"
check_dir "core/visual_feedback"
check_dir "core/groups"
check_dir "core/file_io"

# Apps directories
echo
echo -e "${YELLOW}Applications:${NC}"
check_dir "apps"
check_dir "apps/cli"
check_file "apps/cli/CMakeLists.txt"
check_file "apps/cli/src/main.cpp"
check_file "apps/cli/src/Application.cpp"
check_file "apps/cli/src/CommandProcessor.cpp"
check_file "apps/cli/src/Commands.cpp"
check_file "apps/cli/src/MouseInteraction.cpp"

# Count files
echo
echo -e "${YELLOW}File Statistics:${NC}"
cpp_count=$(count_files "." "*.cpp")
h_count=$(count_files "." "*.h")
cmake_count=$(count_files "." "CMakeLists.txt")
test_count=$(count_files "." "*test*.cpp")

echo "C++ source files: $cpp_count"
echo "Header files: $h_count"
echo "CMake files: $cmake_count"
echo "Test files: $test_count"

# Check for build directory
echo
echo -e "${YELLOW}Build Status:${NC}"
if [ -d "build" ]; then
    echo -e "${GREEN}✓${NC} Build directory exists"
    
    if [ -f "build/bin/VoxelEditor_CLI" ]; then
        echo -e "${GREEN}✓${NC} CLI executable found"
    else
        echo -e "${YELLOW}!${NC} CLI executable not built yet"
        ((warnings++))
    fi
else
    echo -e "${YELLOW}!${NC} Build directory not found - run ./build.sh"
    ((warnings++))
fi

# Summary
echo
echo -e "${YELLOW}=== Validation Summary ===${NC}"
echo "Errors: $errors"
echo "Warnings: $warnings"

if [ $errors -eq 0 ]; then
    if [ $warnings -eq 0 ]; then
        echo -e "${GREEN}✓ Project structure is complete and valid!${NC}"
    else
        echo -e "${GREEN}✓ Project structure is valid with $warnings warnings${NC}"
    fi
    exit 0
else
    echo -e "${RED}✗ Project structure has $errors errors${NC}"
    exit 1
fi