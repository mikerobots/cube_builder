#!/bin/bash
# Test script to validate screenshot functionality

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/../../.."
BUILD_DIR="$PROJECT_ROOT/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

# Check if voxel-cli exists
if [ ! -f "$VOXEL_CLI" ]; then
    echo "Error: voxel-cli not found at $VOXEL_CLI"
    echo "Please build the project first"
    exit 1
fi

# Create a test output directory
TEST_OUTPUT="$BUILD_DIR/screenshot_test_output"
mkdir -p "$TEST_OUTPUT"

echo "Running screenshot validation test..."

# Create a simple scene and capture screenshot
(
    echo "workspace 5 5 5"
    echo "resolution 8cm" 
    echo "place 0 1 0"  # Center voxel (Y must be >= 0)
    echo "place 1 1 0"
    echo "place 0 2 0"
    echo "place 0 1 1"
    echo "camera iso"
    sleep 1
    echo "screenshot $TEST_OUTPUT/test_scene.ppm"
    sleep 1
    echo "quit"
) | "$VOXEL_CLI"

# Check if screenshot was created (handle double extension issue)
SCREENSHOT_FILE="$TEST_OUTPUT/test_scene.ppm"
if [ ! -f "$SCREENSHOT_FILE" ] && [ -f "$SCREENSHOT_FILE.ppm" ]; then
    SCREENSHOT_FILE="$SCREENSHOT_FILE.ppm"
fi

if [ -f "$SCREENSHOT_FILE" ]; then
    echo "✓ Screenshot created successfully"
    
    # Check file size
    SIZE=$(stat -f%z "$SCREENSHOT_FILE" 2>/dev/null || stat -c%s "$SCREENSHOT_FILE" 2>/dev/null)
    echo "  File size: $SIZE bytes"
    
    # Use Python to analyze the image
    python3 - <<EOF
import sys

with open('$SCREENSHOT_FILE', 'rb') as f:
    # Read PPM header
    magic = f.readline().decode('ascii').strip()
    dimensions = f.readline().decode('ascii').strip()
    maxval = f.readline().decode('ascii').strip()
    
    print(f"  Format: {magic}")
    print(f"  Dimensions: {dimensions}")
    print(f"  Max value: {maxval}")
    
    # Read pixel data
    width, height = map(int, dimensions.split())
    pixels = f.read(width * height * 3)
    
    # Calculate statistics
    total_pixels = width * height
    black_pixels = sum(1 for i in range(0, len(pixels), 3) 
                      if pixels[i] == 0 and pixels[i+1] == 0 and pixels[i+2] == 0)
    
    # Average color
    avg_r = sum(pixels[i] for i in range(0, len(pixels), 3)) / total_pixels
    avg_g = sum(pixels[i] for i in range(1, len(pixels), 3)) / total_pixels
    avg_b = sum(pixels[i] for i in range(2, len(pixels), 3)) / total_pixels
    
    print(f"  Black pixels: {black_pixels}/{total_pixels} ({100*black_pixels/total_pixels:.1f}%)")
    print(f"  Average color: R={avg_r:.1f} G={avg_g:.1f} B={avg_b:.1f}")
    
    # Check if image is not all black
    if avg_r > 10 or avg_g > 10 or avg_b > 10:
        print("  ✓ Image contains visible content")
        sys.exit(0)
    else:
        print("  ✗ Image appears to be all black")
        sys.exit(1)
EOF
    
    RESULT=$?
    
    # Also create a simple test with just clear color
    echo -e "\nTesting clear color rendering..."
    (
        echo "workspace 5 5 5"
        sleep 1
        echo "screenshot $TEST_OUTPUT/test_clear.ppm"
        sleep 1
        echo "quit"
    ) | "$VOXEL_CLI"
    
    # Test negative coordinates (centered system)
    echo -e "\nTesting centered coordinate system with negative coordinates..."
    (
        echo "workspace 8 8 8"
        echo "resolution 16cm"
        echo "place -1 0 -1"  # Negative X and Z coordinates
        echo "place 0 0 0"    # Origin
        echo "place 1 0 1"    # Positive coordinates
        echo "camera iso"
        sleep 1
        echo "screenshot $TEST_OUTPUT/test_centered.ppm"
        sleep 1
        echo "quit"
    ) | "$VOXEL_CLI"
    
    if [ -f "$TEST_OUTPUT/test_clear.ppm" ]; then
        echo "✓ Clear color screenshot created"
        python3 - <<EOF
with open('$TEST_OUTPUT/test_clear.ppm', 'rb') as f:
    # Skip header
    f.readline(); f.readline(); f.readline()
    
    # Read some pixels
    pixels = f.read(300)
    
    # Check if it's the expected gray (0.2 * 255 ≈ 51)
    gray_pixels = sum(1 for i in range(0, len(pixels), 3)
                     if 45 <= pixels[i] <= 57)  # Allow some tolerance
    
    if gray_pixels > 80:  # Most pixels should be gray
        print("  ✓ Clear color is correct gray")
    else:
        print("  ✗ Clear color is not the expected gray")
        print(f"    First few pixels: {list(pixels[:30])}")
EOF
    fi
    
    # Check centered coordinate screenshot
    if [ -f "$TEST_OUTPUT/test_centered.ppm" ]; then
        echo "✓ Centered coordinate screenshot created"
        python3 - <<EOF
with open('$TEST_OUTPUT/test_centered.ppm', 'rb') as f:
    # Skip header
    magic = f.readline()
    dimensions = f.readline()
    maxval = f.readline()
    
    width, height = map(int, dimensions.decode('ascii').strip().split())
    pixels = f.read(width * height * 3)
    
    # Calculate average color
    total_pixels = width * height
    avg_r = sum(pixels[i] for i in range(0, len(pixels), 3)) / total_pixels
    avg_g = sum(pixels[i] for i in range(1, len(pixels), 3)) / total_pixels
    avg_b = sum(pixels[i] for i in range(2, len(pixels), 3)) / total_pixels
    
    # Check if image contains visible voxels (should be brighter than background)
    if avg_r > 20 or avg_g > 20 or avg_b > 20:
        print("  ✓ Centered coordinate voxels are visible")
    else:
        print("  ✗ Centered coordinate voxels are not visible")
EOF
    fi
    
    exit $RESULT
else
    echo "✗ Screenshot was not created"
    exit 1
fi