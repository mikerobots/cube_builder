#!/usr/bin/env python3
import sys

def find_white_boxes(filename):
    with open(filename, 'rb') as f:
        # Read PPM header
        magic = f.readline().decode('ascii').strip()
        if magic != 'P6':
            print(f"Error: Not a P6 PPM file")
            return
        
        # Skip comments
        line = f.readline().decode('ascii').strip()
        while line.startswith('#'):
            line = f.readline().decode('ascii').strip()
        
        # Parse dimensions
        width, height = map(int, line.split())
        max_val = int(f.readline().decode('ascii').strip())
        
        print(f"Image: {width}x{height}")
        
        # Read all pixel data
        pixels = f.read()
    
    # Check bottom 20% of image
    bottom_start = int(height * 0.8)
    white_count = 0
    total_bottom = 0
    
    for y in range(bottom_start, height):
        for x in range(width):
            idx = (y * width + x) * 3
            r, g, b = pixels[idx], pixels[idx+1], pixels[idx+2]
            total_bottom += 1
            if r == 255 and g == 255 and b == 255:
                white_count += 1
    
    print(f"\nBottom 20% analysis:")
    print(f"Total pixels: {total_bottom}")
    print(f"White pixels: {white_count} ({white_count/total_bottom*100:.1f}%)")
    
    # Find continuous white regions at very bottom
    print(f"\nBottom row analysis:")
    y = height - 1
    white_regions = []
    in_white = False
    start_x = 0
    
    for x in range(width):
        idx = (y * width + x) * 3
        r, g, b = pixels[idx], pixels[idx+1], pixels[idx+2]
        
        if r == 255 and g == 255 and b == 255:
            if not in_white:
                in_white = True
                start_x = x
        else:
            if in_white:
                white_regions.append((start_x, x-1))
                in_white = False
    
    if in_white:
        white_regions.append((start_x, width-1))
    
    if white_regions:
        print(f"Found {len(white_regions)} white region(s) in bottom row:")
        for i, (start, end) in enumerate(white_regions):
            print(f"  Region {i+1}: x={start} to {end} (width={end-start+1})")
    else:
        print("No white regions in bottom row")
    
    # Check how many rows from bottom have white
    rows_with_white = 0
    for y in range(height-1, -1, -1):
        has_white = False
        for x in range(width):
            idx = (y * width + x) * 3
            r, g, b = pixels[idx], pixels[idx+1], pixels[idx+2]
            if r == 255 and g == 255 and b == 255:
                has_white = True
                break
        if has_white:
            rows_with_white += 1
        else:
            break
    
    print(f"\nContinuous white rows from bottom: {rows_with_white}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python find_white_boxes.py <ppm_file>")
        sys.exit(1)
    find_white_boxes(sys.argv[1])