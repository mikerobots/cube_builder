#!/bin/bash

# Voxel Editor Build Script

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Voxel Editor Build Script ===${NC}"
echo

# Check if build directory exists
if [ -d "build" ]; then
    echo -e "${YELLOW}Build directory exists. Clean rebuild? (y/n)${NC}"
    read -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Removing build directory..."
        rm -rf build
    fi
fi

# Create build directory
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

# Configure with CMake
echo -e "${GREEN}Configuring project...${NC}"
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_CLI=ON

# Get number of cores for parallel build
if [[ "$OSTYPE" == "darwin"* ]]; then
    CORES=$(sysctl -n hw.ncpu)
else
    CORES=$(nproc)
fi

echo -e "${GREEN}Building with $CORES cores...${NC}"
cmake --build . --parallel $CORES

echo
echo -e "${GREEN}Build complete!${NC}"
echo
echo "To run the CLI application:"
echo "  ./build/bin/VoxelEditor_CLI"
echo
echo "To run tests:"
echo "  cd build && ctest --verbose"
echo

# Make CLI executable location more convenient
if [ -f "bin/VoxelEditor_CLI" ]; then
    echo -e "${GREEN}CLI application built successfully!${NC}"
else
    echo -e "${RED}Warning: CLI application not found in expected location${NC}"
fi