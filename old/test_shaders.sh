#!/bin/bash

# Test script for shader validation

echo "=== Shader Test Script ==="
echo "Building shader test application..."

# Configure and build with Ninja
cmake -B build_ninja -G Ninja
cmake --build build_ninja --target ShaderTest

# Check if build succeeded
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

# Create test output directory
mkdir -p test_output

echo ""
echo "Running shader tests..."
echo ""

# Run all tests with report
./build_ninja/bin/ShaderTest --all --report test_output/shader_report.txt --output test_output

# Show summary
echo ""
echo "Test complete. Results:"
echo "- Report: test_output/shader_report.txt"
echo "- Captures: test_output/*.ppm"

# Check for specific shader issues
echo ""
echo "Checking for shader compilation errors..."
if grep -q "FAILED" test_output/shader_report.txt; then
    echo "⚠️  Some shaders failed validation. See report for details."
    grep "FAILED" test_output/shader_report.txt
else
    echo "✅ All shaders passed validation!"
fi

# List captured images
echo ""
echo "Captured shader outputs:"
ls -la test_output/*.ppm 2>/dev/null || echo "No captures generated"