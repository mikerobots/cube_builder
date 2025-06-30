#!/bin/bash

# Script to update all test CMakeLists.txt files to use dynamic test discovery

set -euo pipefail

# Color codes
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo "Updating all test CMakeLists.txt files to use dynamic test discovery..."

# Find all CMakeLists.txt files in test directories
find . -path "*/tests/CMakeLists.txt" -type f | while read -r cmake_file; do
    dir=$(dirname "$cmake_file")
    
    # Check if this directory contains test files
    if ls "$dir"/test_unit_*.cpp >/dev/null 2>&1 || ls "$dir"/test_integration_*.cpp >/dev/null 2>&1; then
        echo -e "${YELLOW}Processing: $cmake_file${NC}"
        
        # Extract the library name from the current CMakeLists.txt
        # Look for target_link_libraries lines
        library_name=""
        if grep -q "target_link_libraries" "$cmake_file"; then
            # Try to extract VoxelEditor_* library name
            library_name=$(grep -A5 "target_link_libraries" "$cmake_file" | grep -o "VoxelEditor_[A-Za-z_]*" | head -1 || true)
        fi
        
        if [ -z "$library_name" ]; then
            echo -e "${RED}  Warning: Could not determine library name for $cmake_file${NC}"
            echo -e "${RED}  Please update manually${NC}"
            continue
        fi
        
        # Create the new CMakeLists.txt content
        # Use a temporary file to avoid permission issues
        temp_file=$(mktemp)
        cat > "$temp_file" << EOF
# Include the test helpers
include(\${CMAKE_SOURCE_DIR}/cmake/TestHelpers.cmake)

# Automatically create test executables for all test_unit_*.cpp files
create_unit_tests(TARGET_LINK_LIBRARIES $library_name)
EOF

        # Check if there are integration tests too
        if ls "$dir"/test_integration_*.cpp >/dev/null 2>&1; then
            cat >> "$cmake_file" << EOF

# Automatically create test executables for all test_integration_*.cpp files
create_integration_tests(TARGET_LINK_LIBRARIES $library_name)
EOF
        fi
        
        echo -e "${GREEN}  Updated with library: $library_name${NC}"
    else
        echo -e "${YELLOW}  Skipping $cmake_file (no test files found)${NC}"
    fi
done

echo -e "${GREEN}Done! Now rebuild with: cmake -B build_ninja -G Ninja -DCMAKE_BUILD_TYPE=Debug${NC}"