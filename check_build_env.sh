#!/bin/bash

# Build Environment Check Script

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}=== Build Environment Check ===${NC}"
echo

# Check CMake
echo -n "CMake: "
if command -v cmake &> /dev/null; then
    version=$(cmake --version | head -n1 | cut -d' ' -f3)
    echo -e "${GREEN}✓${NC} Found version $version"
    
    # Check if version is sufficient (3.16+)
    major=$(echo $version | cut -d. -f1)
    minor=$(echo $version | cut -d. -f2)
    if [ "$major" -gt 3 ] || ([ "$major" -eq 3 ] && [ "$minor" -ge 16 ]); then
        echo -e "  ${GREEN}✓${NC} Version is sufficient (3.16+ required)"
    else
        echo -e "  ${RED}✗${NC} Version too old (3.16+ required)"
    fi
else
    echo -e "${RED}✗${NC} Not found"
fi

# Check C++ compiler
echo -n "C++ Compiler: "
if command -v g++ &> /dev/null; then
    compiler="g++"
    version=$(g++ --version | head -n1)
    echo -e "${GREEN}✓${NC} Found g++"
    echo "  Version: $version"
elif command -v clang++ &> /dev/null; then
    compiler="clang++"
    version=$(clang++ --version | head -n1)
    echo -e "${GREEN}✓${NC} Found clang++"
    echo "  Version: $version"
else
    echo -e "${RED}✗${NC} No C++ compiler found"
    compiler=""
fi

# Check C++20 support
if [ ! -z "$compiler" ]; then
    echo -n "C++20 Support: "
    echo "int main() { return 0; }" > test_cpp20.cpp
    if $compiler -std=c++20 test_cpp20.cpp -o test_cpp20 2>/dev/null; then
        echo -e "${GREEN}✓${NC} Supported"
        rm -f test_cpp20 test_cpp20.cpp
    else
        echo -e "${YELLOW}!${NC} May not be fully supported"
        rm -f test_cpp20.cpp
    fi
fi

# Check Make
echo -n "Make: "
if command -v make &> /dev/null; then
    version=$(make --version | head -n1)
    echo -e "${GREEN}✓${NC} Found"
    echo "  Version: $version"
else
    echo -e "${RED}✗${NC} Not found"
fi

# Check Git
echo -n "Git: "
if command -v git &> /dev/null; then
    version=$(git --version)
    echo -e "${GREEN}✓${NC} Found - $version"
else
    echo -e "${RED}✗${NC} Not found"
fi

# Check OpenGL
echo -n "OpenGL: "
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    if [ -d "/System/Library/Frameworks/OpenGL.framework" ]; then
        echo -e "${GREEN}✓${NC} Found (macOS framework)"
    else
        echo -e "${YELLOW}!${NC} OpenGL framework not found"
    fi
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux
    if ldconfig -p | grep -q libGL.so; then
        echo -e "${GREEN}✓${NC} Found (libGL.so)"
    else
        echo -e "${RED}✗${NC} Not found - install libgl1-mesa-dev"
    fi
else
    echo -e "${YELLOW}!${NC} Platform detection needed"
fi

# Check for existing build
echo
echo -e "${YELLOW}Build Status:${NC}"
if [ -d "build" ]; then
    echo -e "${GREEN}✓${NC} Build directory exists"
    
    if [ -f "build/CMakeCache.txt" ]; then
        echo -e "${GREEN}✓${NC} CMake configuration found"
    else
        echo -e "${YELLOW}!${NC} CMake configuration missing"
    fi
    
    if [ -f "build/bin/VoxelEditor_CLI" ]; then
        echo -e "${GREEN}✓${NC} CLI executable found"
        size=$(du -h build/bin/VoxelEditor_CLI | cut -f1)
        echo "  Size: $size"
    else
        echo -e "${YELLOW}!${NC} CLI executable not found"
    fi
else
    echo -e "${YELLOW}!${NC} No build directory"
fi

echo
echo -e "${GREEN}=== Environment Check Complete ===${NC}"