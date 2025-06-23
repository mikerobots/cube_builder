#!/bin/bash
# Test script with larger voxels for better visibility

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/../../.."
BUILD_DIR="$PROJECT_ROOT/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

# Create a test output directory
TEST_OUTPUT="$BUILD_DIR/large_voxel_test"
mkdir -p "$TEST_OUTPUT"

echo "Testing with larger voxels..."

# Test with 64cm voxels (much larger)
echo -e "\n=== Test with 64cm voxels ==="
(
    echo "workspace 5 5 5"
    echo "resolution 64cm"  # Much larger voxels
    echo "place 0cm 0cm 0cm"      # Center of workspace (origin)
    echo "place 64cm 0cm 0cm"      # Adjacent voxel in +X
    echo "place 0cm 64cm 0cm"      # Adjacent voxel in +Y
    echo "place 0cm 0cm 64cm"      # Adjacent voxel in +Z
    echo "camera front"
    echo "zoom 2.0"  # Zoom in
    sleep 1
    echo "screenshot $TEST_OUTPUT/large_voxels_front.ppm"
    echo "camera iso"
    sleep 1
    echo "screenshot $TEST_OUTPUT/large_voxels_iso.ppm"
    sleep 1
    echo "quit"
) | timeout 10 "$VOXEL_CLI"

# Test with filled area
echo -e "\n=== Test with filled area ==="
(
    echo "workspace 5 5 5"
    echo "resolution 32cm"  # 32cm voxels
    # Place a few voxels instead of using fill (which might be slow)
    echo "place 0cm 0cm 0cm"
    echo "place 32cm 0cm 0cm"
    echo "place -32cm 0cm 0cm"
    echo "place 0cm 32cm 0cm"
    echo "place 0cm 0cm 32cm"
    echo "place 0cm 0cm -32cm"
    echo "camera iso"
    echo "zoom 1.5"
    sleep 1
    echo "screenshot $TEST_OUTPUT/filled_area.ppm"
    sleep 1
    echo "quit"
) | timeout 10 "$VOXEL_CLI"

# Analyze results
echo -e "\n=== Analyzing screenshots ==="

# Check if screenshots were created
SUCCESS=true
for expected_file in "large_voxels_front.ppm.ppm" "large_voxels_iso.ppm.ppm" "filled_area.ppm.ppm"; do
    if [ -f "$TEST_OUTPUT/$expected_file" ]; then
        echo "✓ $expected_file created"
        # Check file size (should be > 1MB for a real screenshot)
        size=$(stat -f%z "$TEST_OUTPUT/$expected_file" 2>/dev/null || stat -c%s "$TEST_OUTPUT/$expected_file" 2>/dev/null)
        if [ "$size" -gt 1000000 ]; then
            echo "  File size: $(($size / 1048576)) MB - OK"
        else
            echo "  File size: $size bytes - TOO SMALL"
            SUCCESS=false
        fi
    else
        echo "✗ $expected_file NOT created"
        SUCCESS=false
    fi
done

echo -e "\n=== Done ==="

if $SUCCESS; then
    echo "✅ Large voxel test passed!"
    exit 0
else
    echo "❌ Large voxel test failed!"
    exit 1
fi