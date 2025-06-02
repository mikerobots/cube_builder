#!/bin/bash

# Automated Build Script (non-interactive)

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}=== Voxel Editor Automated Build ===${NC}"
echo

# Configure with CMake
echo -e "${GREEN}Configuring project...${NC}"
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_CLI=ON

# Get number of cores
CORES=$(sysctl -n hw.ncpu)

echo -e "${GREEN}Building with $CORES cores...${NC}"
make -j$CORES

echo
echo -e "${GREEN}Build complete!${NC}"

# Check if executable was created
if [ -f "bin/VoxelEditor_CLI" ]; then
    echo -e "${GREEN}✓ CLI application built successfully!${NC}"
    echo "  Location: $(pwd)/bin/VoxelEditor_CLI"
else
    echo -e "${RED}✗ Build may have failed - executable not found${NC}"
fi