#!/bin/bash

echo "=== Ground Plane Grid Color Analysis ==="

# Function to analyze PPM file
analyze_ppm() {
    local file=$1
    echo -e "\nAnalyzing: $file"
    if [ -f "$file" ]; then
        python3 tools/analyze_ppm_colors.py "$file" | head -15
        
        # Check for specific grid colors
        echo -e "\nChecking for expected grid colors (light grey ~180-200):"
        python3 -c "
import sys
from PIL import Image
import numpy as np

img = Image.open('$file')
pixels = np.array(img)

# Count pixels in the grey range we expect for grid lines
grey_range = ((pixels[:,:,0] >= 170) & (pixels[:,:,0] <= 210) &
              (pixels[:,:,1] >= 170) & (pixels[:,:,1] <= 210) &
              (pixels[:,:,2] >= 170) & (pixels[:,:,2] <= 210) &
              (pixels[:,:,0] == pixels[:,:,1]) & 
              (pixels[:,:,1] == pixels[:,:,2]))

grey_count = np.sum(grey_range)
print(f'Light grey pixels (170-210): {grey_count}')

# Check for any non-background, non-white pixels
unique_colors = set()
for y in range(pixels.shape[0]):
    for x in range(pixels.shape[1]):
        color = tuple(pixels[y,x])
        if color != (77,77,77) and color != (255,255,255):
            unique_colors.add(color)

if unique_colors:
    print(f'Other colors found: {len(unique_colors)}')
    for color in list(unique_colors)[:10]:
        print(f'  {color}')
"
    else
        echo "File not found: $file"
    fi
}

# Analyze the grid screenshots
analyze_ppm "test_grid_fixed.ppm.ppm"
analyze_ppm "test_grid_off.ppm.ppm"
analyze_ppm "test_grid_with_voxel.ppm.ppm"

echo -e "\n=== Summary ==="
echo "Expected: Light grey grid lines (RGB ~180-200) with transparency"
echo "Actual: Seeing white pixels (255,255,255) instead"
echo "This suggests alpha blending may not be working correctly"