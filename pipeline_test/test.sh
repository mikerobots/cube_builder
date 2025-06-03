#!/bin/bash

set -e  # Exit on any error

echo "Running Pipeline Test Suite..."

# Check if binary exists
if [ ! -f "build/pipeline_test" ]; then
    echo "Binary not found. Running build first..."
    ./build.sh
fi

cd build

# Run all tests
echo ""
echo "Running all tests..."
./pipeline_test

# Analyze results
echo ""
echo "=== ANALYZING RESULTS ==="

for i in 1 2 3 4; do
    echo ""
    echo "Test $i analysis:"
    
    # Find the screenshot file for this test
    screenshot=$(ls test${i}_*.ppm 2>/dev/null | head -1)
    
    if [ -f "$screenshot" ]; then
        echo "  Screenshot: $screenshot"
        
        # Get file size
        size=$(wc -c < "$screenshot")
        echo "  File size: $size bytes"
        
        # Check for unique colors using Python
        python3 << EOF
import sys
try:
    with open('$screenshot', 'rb') as f:
        data = f.read()
    
    # Find header end
    header_end = data.find(b'255\\n') + 4
    
    # Count unique colors
    unique_colors = set()
    for i in range(header_end, len(data) - 2, 3):
        rgb = data[i:i+3]
        unique_colors.add(tuple(rgb))
    
    print(f"  Unique colors: {len(unique_colors)}")
    
    # Check for background vs non-background
    bg_color = (51, 51, 51)  # Dark gray
    non_bg_pixels = 0
    total_pixels = (len(data) - header_end) // 3
    
    for i in range(header_end, len(data) - 2, 3):
        rgb = tuple(data[i:i+3])
        if rgb != bg_color:
            non_bg_pixels += 1
    
    coverage = (non_bg_pixels * 100.0) / total_pixels if total_pixels > 0 else 0
    print(f"  Coverage: {coverage:.1f}% ({non_bg_pixels}/{total_pixels} pixels)")
    
    if len(unique_colors) > 1:
        print("  ✅ GEOMETRY DETECTED")
    else:
        print("  ❌ NO GEOMETRY DETECTED")

except Exception as e:
    print(f"  Error analyzing: {e}")
EOF
        
    else
        echo "  ❌ No screenshot found"
    fi
done

echo ""
echo "Test suite completed. Check screenshots manually if needed."