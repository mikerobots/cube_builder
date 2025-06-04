#!/usr/bin/env python3
"""
PPM Color Analyzer
Analyzes a PPM file and returns the count of each unique color.
"""

import sys
from collections import defaultdict

def read_ppm(filename):
    """Read a PPM file and return its data."""
    try:
        with open(filename, 'rb') as f:
            # Read header
            magic = f.readline().decode('ascii').strip()
            if magic not in ['P3', 'P6']:
                print(f"Error: Unsupported PPM format '{magic}'. Only P3 and P6 are supported.")
                return None, None, None
            
            # Skip comments
            line = f.readline().decode('ascii').strip()
            while line.startswith('#'):
                line = f.readline().decode('ascii').strip()
            
            # Parse dimensions
            width, height = map(int, line.split())
            
            # Parse max value
            max_val = int(f.readline().decode('ascii').strip())
            
            # Read pixel data
            if magic == 'P3':
                # ASCII format
                data = f.read().decode('ascii')
                values = list(map(int, data.split()))
                if len(values) != width * height * 3:
                    print(f"Error: Expected {width * height * 3} values, got {len(values)}")
                    return None, None, None
                pixels = []
                for i in range(0, len(values), 3):
                    pixels.append((values[i], values[i+1], values[i+2]))
            else:  # P6
                # Binary format
                pixel_data = f.read()
                if max_val <= 255:
                    # 1 byte per channel
                    if len(pixel_data) != width * height * 3:
                        print(f"Error: Expected {width * height * 3} bytes, got {len(pixel_data)}")
                        return None, None, None
                    pixels = []
                    for i in range(0, len(pixel_data), 3):
                        pixels.append((pixel_data[i], pixel_data[i+1], pixel_data[i+2]))
                else:
                    # 2 bytes per channel (big-endian)
                    if len(pixel_data) != width * height * 6:
                        print(f"Error: Expected {width * height * 6} bytes, got {len(pixel_data)}")
                        return None, None, None
                    pixels = []
                    for i in range(0, len(pixel_data), 6):
                        r = (pixel_data[i] << 8) | pixel_data[i+1]
                        g = (pixel_data[i+2] << 8) | pixel_data[i+3]
                        b = (pixel_data[i+4] << 8) | pixel_data[i+5]
                        pixels.append((r, g, b))
            
            return width, height, pixels
            
    except FileNotFoundError:
        print(f"Error: File '{filename}' not found.")
        return None, None, None
    except Exception as e:
        print(f"Error reading PPM file: {e}")
        return None, None, None

def analyze_colors(pixels):
    """Count occurrences of each color."""
    color_counts = defaultdict(int)
    
    for pixel in pixels:
        color_counts[pixel] += 1
    
    return color_counts

def main():
    if len(sys.argv) != 2:
        print("Usage: analyze_ppm_colors.py <filename.ppm>")
        sys.exit(1)
    
    filename = sys.argv[1]
    
    # Read PPM file
    width, height, pixels = read_ppm(filename)
    if pixels is None:
        sys.exit(1)
    
    print(f"Image dimensions: {width}x{height}")
    print(f"Total pixels: {width * height}")
    print()
    
    # Analyze colors
    color_counts = analyze_colors(pixels)
    
    # Sort by count (descending) then by color values
    sorted_colors = sorted(color_counts.items(), key=lambda x: (-x[1], x[0]))
    
    print("Color distribution:")
    print("Format: (R,G,B) : Count")
    print("-" * 40)
    for color, count in sorted_colors:
        percentage = (count / len(pixels)) * 100
        print(f"({color[0]:3d}, {color[1]:3d}, {color[2]:3d}) : {count:6d} ({percentage:5.2f}%)")
    
    print("-" * 40)
    print(f"Unique colors: {len(color_counts)}")

if __name__ == "__main__":
    main()