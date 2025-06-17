#!/bin/bash

# Script to run all shader validation tests
# This includes unit tests, visual tests, and integration tests

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build_ninja"

echo "================================================"
echo "Running All Shader Validation Tests"
echo "================================================"

# Build if needed
if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found. Running initial build..."
    cmake -B "$BUILD_DIR" -G Ninja
fi

echo "Building latest changes..."
cmake --build "$BUILD_DIR" --target VoxelEditor_Rendering_Tests

echo ""
echo "1. Running shader unit tests..."
echo "================================================"
cd "$BUILD_DIR"
timeout 60 ./bin/VoxelEditor_Rendering_Tests --gtest_filter="*Shader*" || {
    echo "Shader unit tests failed or timed out"
    exit 1
}

echo ""
echo "2. Running inline shader validation tests..."
echo "================================================"
timeout 30 ./bin/VoxelEditor_Rendering_Tests --gtest_filter="InlineShaderTest.*" || {
    echo "Inline shader tests failed or timed out"
    exit 1
}

echo ""
echo "3. Running file-based shader validation tests..."
echo "================================================"
timeout 30 ./bin/VoxelEditor_Rendering_Tests --gtest_filter="FileBasedShaderTest.*" || {
    echo "File-based shader tests failed or timed out"
    exit 1
}

echo ""
echo "4. Running visual shader validation tests..."
echo "================================================"
timeout 30 ./bin/VoxelEditor_Rendering_Tests --gtest_filter="ShaderVisualTest.*" || {
    echo "Visual shader tests failed or timed out"
    exit 1
}

echo ""
echo "5. Running shader test application..."
echo "================================================"
if [ -f ./apps/shader_test/ShaderTestApp ]; then
    echo "Running shader test app..."
    timeout 30 ./apps/shader_test/ShaderTestApp || {
        echo "Shader test app failed or timed out"
        exit 1
    }
else
    echo "Shader test app not built, skipping..."
fi

echo ""
echo "6. Running CLI shader rendering tests..."
echo "================================================"
cd "$SCRIPT_DIR"

# Test basic shader rendering
if [ -f ./test_shaders.sh ]; then
    echo "Running test_shaders.sh..."
    timeout 60 ./test_shaders.sh || {
        echo "Basic shader test failed"
        exit 1
    }
fi

# Test ground plane shader
if [ -f ./test_ground_plane_shader.sh ]; then
    echo "Running test_ground_plane_shader.sh..."
    timeout 60 ./test_ground_plane_shader.sh || {
        echo "Ground plane shader test failed"
        exit 1
    }
fi

echo ""
echo "================================================"
echo "All Shader Validation Tests Completed Successfully!"
echo "================================================"
echo ""
echo "Summary:"
echo "- Inline shaders: VALIDATED"
echo "- File-based shaders: VALIDATED"
echo "- Visual output: VALIDATED"
echo "- Integration: VALIDATED"
echo ""

# Generate coverage report if available
if command -v llvm-cov &> /dev/null && [ -d "$BUILD_DIR/CMakeFiles/VoxelEditor_Rendering_Tests.dir" ]; then
    echo "Generating shader test coverage report..."
    llvm-cov report "$BUILD_DIR/core/rendering/tests/VoxelEditor_Rendering_Tests" \
        --instr-profile="$BUILD_DIR/default.profdata" \
        -ignore-filename-regex="test_.*\.cpp" \
        -ignore-filename-regex=".*/external/.*" \
        -ignore-filename-regex=".*/tests/.*" \
        | grep -E "(Shader|shader)" || true
fi