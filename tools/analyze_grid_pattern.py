#!/usr/bin/env python3
import sys

if len(sys.argv) != 2:
    print("Usage: analyze_grid_pattern.py <ppm_file>")
    sys.exit(1)

filename = sys.argv[1]

# Read PPM file
with open(filename, 'r') as f:
    magic = f.readline().strip()
    if magic != 'P3':
        print(f"Error: Expected P3 format, got {magic}")
        sys.exit(1)
    
    # Read dimensions
    dims = f.readline().strip()
    width, height = map(int, dims.split())
    
    # Read max value
    max_val = int(f.readline().strip())
    
    # Read pixel data
    pixels = []
    for line in f:
        pixels.extend(map(int, line.split()))

# Convert to 2D array
image = []
for y in range(height):
    row = []
    for x in range(width):
        idx = (y * width + x) * 3
        r, g, b = pixels[idx:idx+3]
        row.append((r, g, b))
    image.append(row)

print(f"Image dimensions: {width}x{height}")

# Analyze horizontal lines
print("\nHorizontal line analysis:")
horizontal_lines = []
for y in range(height):
    # Count non-black pixels in this row
    non_black = sum(1 for x in range(width) if any(c > 0 for c in image[y][x]))
    if non_black > width * 0.1:  # At least 10% of row is non-black
        horizontal_lines.append(y)
        if len(horizontal_lines) <= 5:
            # Sample some pixels from this line
            samples = []
            for x in range(0, min(10, width), 2):
                samples.append(f"{image[y][x]}")
            print(f"  Line at y={height-1-y}: {non_black} pixels, samples: {' '.join(samples[:5])}")

# Analyze vertical lines
print("\nVertical line analysis:")
vertical_lines = []
for x in range(width):
    # Count non-black pixels in this column
    non_black = sum(1 for y in range(height) if any(c > 0 for c in image[y][x]))
    if non_black > height * 0.1:  # At least 10% of column is non-black
        vertical_lines.append(x)
        if len(vertical_lines) <= 5:
            # Sample some pixels from this line
            samples = []
            for y in range(0, min(10, height), 2):
                samples.append(f"{image[y][x]}")
            print(f"  Line at x={x}: {non_black} pixels")

print(f"\nTotal horizontal lines: {len(horizontal_lines)}")
print(f"Total vertical lines: {len(vertical_lines)}")

# Check for grid pattern in center region
center_x = width // 2
center_y = height // 2
window = 50

print(f"\nCenter region analysis ({center_x-window},{center_y-window}) to ({center_x+window},{center_y+window}):")
grid_pixels = 0
for y in range(max(0, center_y-window), min(height, center_y+window)):
    for x in range(max(0, center_x-window), min(width, center_x+window)):
        if any(c > 0 for c in image[y][x]):
            grid_pixels += 1

print(f"Grid pixels in center: {grid_pixels} / {(2*window)*(2*window)} = {100*grid_pixels/((2*window)*(2*window)):.1f}%")

# Print a small ASCII representation of the center
print("\nCenter visualization (# = bright, . = dark):")
for y in range(center_y-10, center_y+10):
    line = ""
    for x in range(center_x-20, center_x+20):
        if 0 <= x < width and 0 <= y < height:
            brightness = sum(image[y][x]) / (3 * 255)
            if brightness > 0.5:
                line += "#"
            elif brightness > 0.1:
                line += "+"
            else:
                line += "."
        else:
            line += " "
    print(line)