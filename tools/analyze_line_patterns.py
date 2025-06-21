#!/usr/bin/env python3
"""
PPM Line Pattern Analyzer
Analyzes a PPM file to detect horizontal and vertical line patterns.
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

def analyze_line_patterns(width, height, pixels):
    """Analyze pixels for horizontal and vertical line patterns."""
    # Find non-black pixels
    non_black_pixels = []
    for y in range(height):
        for x in range(width):
            pixel = pixels[y * width + x]
            if pixel != (0, 0, 0):  # Not black
                non_black_pixels.append((x, y, pixel))
    
    if not non_black_pixels:
        return {
            'total_non_black': 0,
            'horizontal_lines': [],
            'vertical_lines': [],
            'pattern_type': 'none'
        }
    
    # Group pixels by rows (for horizontal lines)
    pixels_by_row = defaultdict(list)
    for x, y, color in non_black_pixels:
        pixels_by_row[y].append((x, color))
    
    # Group pixels by columns (for vertical lines)
    pixels_by_col = defaultdict(list)
    for x, y, color in non_black_pixels:
        pixels_by_col[x].append((y, color))
    
    # Analyze horizontal lines (rows with many pixels)
    horizontal_lines = []
    for row, row_pixels in pixels_by_row.items():
        if len(row_pixels) > 5:  # Threshold for considering it a line
            x_positions = [x for x, _ in row_pixels]
            line_length = max(x_positions) - min(x_positions) + 1
            horizontal_lines.append({
                'row': row,
                'pixel_count': len(row_pixels),
                'line_length': line_length,
                'x_range': (min(x_positions), max(x_positions))
            })
    
    # Analyze vertical lines (columns with many pixels)
    vertical_lines = []
    for col, col_pixels in pixels_by_col.items():
        if len(col_pixels) > 5:  # Threshold for considering it a line
            y_positions = [y for y, _ in col_pixels]
            line_length = max(y_positions) - min(y_positions) + 1
            vertical_lines.append({
                'column': col,
                'pixel_count': len(col_pixels),
                'line_length': line_length,
                'y_range': (min(y_positions), max(y_positions))
            })
    
    # Determine pattern type
    pattern_type = 'none'
    if horizontal_lines and vertical_lines:
        pattern_type = 'grid'
    elif horizontal_lines:
        pattern_type = 'horizontal'
    elif vertical_lines:
        pattern_type = 'vertical'
    
    return {
        'total_non_black': len(non_black_pixels),
        'horizontal_lines': horizontal_lines,
        'vertical_lines': vertical_lines,
        'pattern_type': pattern_type,
        'pixels_by_row': dict(pixels_by_row),
        'pixels_by_col': dict(pixels_by_col)
    }

def main():
    if len(sys.argv) != 2:
        print("Usage: analyze_line_patterns.py <filename.ppm>")
        sys.exit(1)
    
    filename = sys.argv[1]
    
    # Read PPM file
    width, height, pixels = read_ppm(filename)
    if pixels is None:
        sys.exit(1)
    
    print(f"Image dimensions: {width}x{height}")
    print(f"Total pixels: {width * height}")
    print()
    
    # Analyze line patterns
    analysis = analyze_line_patterns(width, height, pixels)
    
    print(f"Non-black pixels: {analysis['total_non_black']}")
    print(f"Pattern type: {analysis['pattern_type']}")
    print()
    
    if analysis['horizontal_lines']:
        print(f"Horizontal lines detected: {len(analysis['horizontal_lines'])}")
        for i, line in enumerate(analysis['horizontal_lines'][:5]):  # Show first 5
            print(f"  Line {i+1}: Row {line['row']}, {line['pixel_count']} pixels, length {line['line_length']}, X range {line['x_range']}")
        if len(analysis['horizontal_lines']) > 5:
            print(f"  ... and {len(analysis['horizontal_lines']) - 5} more horizontal lines")
        print()
    
    if analysis['vertical_lines']:
        print(f"Vertical lines detected: {len(analysis['vertical_lines'])}")
        for i, line in enumerate(analysis['vertical_lines'][:5]):  # Show first 5
            print(f"  Line {i+1}: Column {line['column']}, {line['pixel_count']} pixels, length {line['line_length']}, Y range {line['y_range']}")
        if len(analysis['vertical_lines']) > 5:
            print(f"  ... and {len(analysis['vertical_lines']) - 5} more vertical lines")
        print()
    
    # Summary of pixel distribution
    if analysis['pixels_by_row']:
        rows_with_pixels = len(analysis['pixels_by_row'])
        print(f"Rows with non-black pixels: {rows_with_pixels}")
    
    if analysis['pixels_by_col']:
        cols_with_pixels = len(analysis['pixels_by_col'])
        print(f"Columns with non-black pixels: {cols_with_pixels}")

if __name__ == "__main__":
    main()