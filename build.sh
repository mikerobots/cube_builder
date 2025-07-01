#!/bin/bash

# Build script for Voxel Editor CLI
# This script builds the CLI application and creates a symlink for easy access

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Voxel Editor CLI Build Script ===${NC}"
echo

# Check if we're in the root directory
if [ ! -f "CMakeLists.txt" ] || [ ! -d "apps/cli" ]; then
    echo -e "${RED}Error: This script must be run from the project root directory${NC}"
    exit 1
fi

# Default build type
BUILD_TYPE="Release"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --debug    Build in Debug mode (default: Release)"
            echo "  --help     Show this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

BUILD_DIR="build"
if [ "$BUILD_TYPE" = "Debug" ]; then
    BUILD_DIR="build_debug"
fi

echo -e "${YELLOW}Build Configuration:${NC}"
echo "  Build Type: $BUILD_TYPE"
echo "  Build Directory: $BUILD_DIR"
echo

# Configure with CMake if needed
if [ ! -d "$BUILD_DIR" ] || [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo -e "${YELLOW}Configuring CMake...${NC}"
    cmake -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    echo
fi

# Build the CLI
echo -e "${YELLOW}Building CLI...${NC}"
cmake --build "$BUILD_DIR" --target VoxelEditor_CLI

# Check if build succeeded
if [ ! -f "$BUILD_DIR/bin/VoxelEditor_CLI" ]; then
    echo -e "${RED}Error: Build failed - CLI executable not found${NC}"
    exit 1
fi

# Create symlink in root directory
echo
echo -e "${YELLOW}Creating symlink...${NC}"
if [ -L "voxel-cli" ] || [ -e "voxel-cli" ]; then
    echo "Removing existing voxel-cli symlink/file..."
    rm -f voxel-cli
fi

ln -s "$BUILD_DIR/bin/VoxelEditor_CLI" voxel-cli
echo -e "${GREEN}Created symlink: voxel-cli -> $BUILD_DIR/bin/VoxelEditor_CLI${NC}"

# Make the script executable
chmod +x voxel-cli

echo
echo -e "${GREEN}=== Build Complete! ===${NC}"
echo
echo "You can now run the CLI with:"
echo "  ./voxel-cli"
echo
echo "Or use the execute_command.sh wrapper:"
echo "  ./execute_command.sh ./voxel-cli"
echo