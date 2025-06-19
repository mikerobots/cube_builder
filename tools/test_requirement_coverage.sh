#!/bin/bash

# Simple test of requirement coverage analysis on one subsystem
# This demonstrates the approach without running all tests

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-build_ninja}"

echo "========================================="
echo "Requirement Coverage Test (VoxelData only)"
echo "========================================="

# Test with VoxelData subsystem only
cd "$PROJECT_ROOT"

echo "Running VoxelData tests to demonstrate requirement tracking..."

# Run just VoxelData tests with verbose output
cd "$BUILD_DIR"
timeout 10s ctest -R "VoxelEditor_VoxelData_Tests" --verbose > /tmp/voxel_test_output.log 2>&1 || true

echo "Analyzing test output for requirement IDs..."

# Extract requirements from test names
echo "Requirements found in test execution:"
grep -o 'REQ_[0-9_]*' /tmp/voxel_test_output.log | sed 's/_/\./g' | sort | uniq

echo ""
echo "Test results with requirements:"
grep -E '\[ +(RUN|OK|FAILED) +\].*REQ_[0-9_]+' /tmp/voxel_test_output.log | head -10

echo ""
echo "Summary of approach:"
echo "1. ✅ Test names already contain REQ-X.X.X patterns"
echo "2. ✅ GoogleTest outputs test results with names"  
echo "3. ✅ We can parse test status (PASSED/FAILED) automatically"
echo "4. ✅ No code changes needed - just parse existing output"

echo ""
echo "Full test output saved to: /tmp/voxel_test_output.log"