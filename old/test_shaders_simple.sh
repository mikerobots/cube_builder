#!/bin/bash

# Simple shader test script

echo "=== Simple Shader Test Script ==="
echo "Building simple shader test application..."

# Configure and build with Ninja
cmake -B build_ninja -G Ninja
cmake --build build_ninja --target SimpleShaderTest

# Check if build succeeded
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo ""
echo "Running simple shader test..."
echo ""

# Run the test
cd build_ninja/bin && ./SimpleShaderTest

# Return the exit code from the test
exit $?