#!/bin/bash

# Check Build Status Script

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}=== Build Status Check ===${NC}"
echo

# Check if build is running
if pgrep -f "make|cmake" > /dev/null; then
    echo -e "${YELLOW}⏳ Build is currently running...${NC}"
    echo
    echo "Active build processes:"
    ps aux | grep -E "make|cmake|c\+\+|clang" | grep -v grep | head -5
    echo
fi

# Check build directory
if [ -d "build" ]; then
    echo "Build directory contents:"
    
    # Check for executable
    if [ -f "build/bin/VoxelEditor_CLI" ]; then
        echo -e "${GREEN}✓ CLI executable found!${NC}"
        size=$(du -h build/bin/VoxelEditor_CLI | cut -f1)
        echo "  Size: $size"
        echo "  Created: $(stat -f "%Sm" -t "%Y-%m-%d %H:%M:%S" build/bin/VoxelEditor_CLI 2>/dev/null || date -r build/bin/VoxelEditor_CLI)"
    else
        echo -e "${YELLOW}! CLI executable not found yet${NC}"
    fi
    
    # Count built libraries
    lib_count=$(find build -name "*.a" -o -name "*.so" -o -name "*.dylib" 2>/dev/null | wc -l)
    echo "  Built libraries: $lib_count"
    
    # Count object files
    obj_count=$(find build -name "*.o" 2>/dev/null | wc -l)
    echo "  Object files: $obj_count"
    
    # Check for test executables
    test_count=$(find build -name "*_Tests" -type f 2>/dev/null | wc -l)
    echo "  Test executables: $test_count"
    
    # Check last modified
    echo
    echo "Most recently modified files:"
    find build -type f -name "*.o" -o -name "*.a" -o -name "*_CLI" 2>/dev/null | xargs ls -lt 2>/dev/null | head -5
fi

# Check for errors
if [ -f "build/CMakeFiles/CMakeError.log" ]; then
    error_count=$(grep -c "error:" build/CMakeFiles/CMakeError.log 2>/dev/null || echo "0")
    if [ "$error_count" -gt "0" ]; then
        echo
        echo -e "${RED}⚠️  Found $error_count errors in CMake log${NC}"
        echo "Last few errors:"
        grep "error:" build/CMakeFiles/CMakeError.log | tail -3
    fi
fi

echo
echo -e "${GREEN}=== End Status Check ===${NC}"
echo
echo "To monitor build progress:"
echo "  watch -n 1 ./check_build_status.sh"
echo
echo "To check if build completed:"
echo "  ./build/bin/VoxelEditor_CLI --version"