#!/bin/bash

set -e  # Exit on any error

echo "Building Pipeline Test Suite..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Build
make -j$(sysctl -n hw.ncpu)

echo "Build completed successfully!"
echo "Executable: build/pipeline_test"