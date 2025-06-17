#!/usr/bin/env python3
"""
Analyzes PPM images from geometric shader tests to validate rendering correctness.
"""

import sys
import os
import numpy as np
from collections import Counter

def read_ppm(filename):
    """Read a PPM file and return the image as a numpy array."""
    with open(filename, 'rb') as f:
        # Read header
        magic = f.readline().decode('ascii').strip()
        if magic != 'P6':
            raise ValueError(f"Not a binary PPM file: {magic}")
        
        # Skip comments
        line = f.readline().decode('ascii')
        while line.startswith('#'):
            line = f.readline().decode('ascii')
        
        # Read dimensions
        width, height = map(int, line.split())
        
        # Read max value
        maxval = int(f.readline().decode('ascii'))
        
        # Read pixel data
        data = np.frombuffer(f.read(), dtype=np.uint8)
        data = data.reshape((height, width, 3))
        
    return data

def analyze_symmetry(image, axis='vertical'):
    """Analyze symmetry of the image along the specified axis."""
    h, w, _ = image.shape
    
    if axis == 'vertical':
        # Compare left and right halves
        left_half = image[:, :w//2]
        right_half = image[:, w//2:]
        
        # Flip right half for comparison
        right_half_flipped = np.fliplr(right_half)
        
        # Calculate difference
        diff = np.abs(left_half.astype(float) - right_half_flipped.astype(float))
        avg_diff = np.mean(diff)
        
        return {
            'average_difference': avg_diff,
            'max_difference': np.max(diff),
            'symmetric_pixels': np.sum(diff.sum(axis=2) < 10),  # Pixels with total RGB diff < 10
            'total_pixels': left_half.shape[0] * left_half.shape[1],
            'symmetry_score': 1.0 - (avg_diff / 255.0)
        }
    
    elif axis == 'horizontal':
        # Compare top and bottom halves
        top_half = image[:h//2, :]
        bottom_half = image[h//2:, :]
        
        # Flip bottom half for comparison
        bottom_half_flipped = np.flipud(bottom_half)
        
        # Calculate difference
        diff = np.abs(top_half.astype(float) - bottom_half_flipped.astype(float))
        avg_diff = np.mean(diff)
        
        return {
            'average_difference': avg_diff,
            'max_difference': np.max(diff),
            'symmetric_pixels': np.sum(diff.sum(axis=2) < 10),
            'total_pixels': top_half.shape[0] * top_half.shape[1],
            'symmetry_score': 1.0 - (avg_diff / 255.0)
        }

def analyze_grid_pattern(image, expected_lines=10):
    """Analyze if the image contains a grid pattern."""
    h, w, _ = image.shape
    
    # Convert to grayscale
    gray = np.mean(image, axis=2)
    
    # Detect horizontal lines
    horizontal_profile = np.mean(gray, axis=1)
    h_peaks = []
    for i in range(1, h-1):
        if horizontal_profile[i] > horizontal_profile[i-1] and horizontal_profile[i] > horizontal_profile[i+1]:
            if horizontal_profile[i] > np.mean(horizontal_profile) + np.std(horizontal_profile):
                h_peaks.append(i)
    
    # Detect vertical lines
    vertical_profile = np.mean(gray, axis=0)
    v_peaks = []
    for i in range(1, w-1):
        if vertical_profile[i] > vertical_profile[i-1] and vertical_profile[i] > vertical_profile[i+1]:
            if vertical_profile[i] > np.mean(vertical_profile) + np.std(vertical_profile):
                v_peaks.append(i)
    
    return {
        'horizontal_lines': len(h_peaks),
        'vertical_lines': len(v_peaks),
        'expected_lines': expected_lines,
        'grid_score': min(len(h_peaks) + len(v_peaks), expected_lines * 2) / (expected_lines * 2)
    }

def analyze_checkerboard(image, expected_colors=2):
    """Analyze if the image contains a checkerboard pattern."""
    h, w, _ = image.shape
    
    # Find non-black pixels
    non_black_mask = np.sum(image, axis=2) > 30
    non_black_pixels = image[non_black_mask]
    
    if len(non_black_pixels) == 0:
        return {'error': 'No non-black pixels found'}
    
    # Cluster colors
    # Simple approach: check if pixels are more red or more blue
    red_pixels = 0
    blue_pixels = 0
    
    for pixel in non_black_pixels:
        if pixel[0] > pixel[2]:  # More red than blue
            red_pixels += 1
        else:  # More blue than red
            blue_pixels += 1
    
    total_colored = red_pixels + blue_pixels
    red_ratio = red_pixels / total_colored if total_colored > 0 else 0
    blue_ratio = blue_pixels / total_colored if total_colored > 0 else 0
    
    # Check for alternating pattern
    # Sample grid positions
    grid_size = 4  # Expected 4x4 grid
    cell_w = w // grid_size
    cell_h = h // grid_size
    
    pattern_correct = 0
    pattern_total = 0
    
    for i in range(grid_size):
        for j in range(grid_size):
            # Sample center of each cell
            x = j * cell_w + cell_w // 2
            y = i * cell_h + cell_h // 2
            
            if x < w and y < h:
                pixel = image[y, x]
                if np.sum(pixel) > 30:  # Not black
                    pattern_total += 1
                    expected_red = (i + j) % 2 == 0
                    is_red = pixel[0] > pixel[2]
                    if is_red == expected_red:
                        pattern_correct += 1
    
    return {
        'red_pixels': red_pixels,
        'blue_pixels': blue_pixels,
        'red_ratio': red_ratio,
        'blue_ratio': blue_ratio,
        'balance_score': 1.0 - abs(red_ratio - 0.5) * 2,
        'pattern_score': pattern_correct / pattern_total if pattern_total > 0 else 0,
        'overall_score': ((1.0 - abs(red_ratio - 0.5) * 2) + 
                         (pattern_correct / pattern_total if pattern_total > 0 else 0)) / 2
    }

def main():
    if len(sys.argv) < 2:
        print("Usage: python analyze_geometric_test.py <image.ppm> [test_type]")
        print("Test types: symmetry, grid, checkerboard")
        sys.exit(1)
    
    filename = sys.argv[1]
    test_type = sys.argv[2] if len(sys.argv) > 2 else 'auto'
    
    if not os.path.exists(filename):
        print(f"Error: File '{filename}' not found.")
        sys.exit(1)
    
    # Read image
    try:
        image = read_ppm(filename)
    except Exception as e:
        print(f"Error reading PPM file: {e}")
        sys.exit(1)
    
    print(f"Image: {filename}")
    print(f"Dimensions: {image.shape[1]}x{image.shape[0]}")
    print()
    
    # Auto-detect test type from filename
    if test_type == 'auto':
        if 'cube' in filename:
            test_type = 'symmetry'
        elif 'grid' in filename:
            test_type = 'grid'
        elif 'checkerboard' in filename:
            test_type = 'checkerboard'
        else:
            test_type = 'all'
    
    # Run analysis
    if test_type in ['symmetry', 'all']:
        print("=== Symmetry Analysis ===")
        v_sym = analyze_symmetry(image, 'vertical')
        print(f"Vertical symmetry score: {v_sym['symmetry_score']:.3f}")
        print(f"Average pixel difference: {v_sym['average_difference']:.1f}")
        print(f"Symmetric pixels: {v_sym['symmetric_pixels']}/{v_sym['total_pixels']} "
              f"({100*v_sym['symmetric_pixels']/v_sym['total_pixels']:.1f}%)")
        print()
    
    if test_type in ['grid', 'all']:
        print("=== Grid Pattern Analysis ===")
        grid = analyze_grid_pattern(image)
        print(f"Horizontal lines detected: {grid['horizontal_lines']}")
        print(f"Vertical lines detected: {grid['vertical_lines']}")
        print(f"Grid score: {grid['grid_score']:.3f}")
        print()
    
    if test_type in ['checkerboard', 'all']:
        print("=== Checkerboard Analysis ===")
        checker = analyze_checkerboard(image)
        if 'error' in checker:
            print(f"Error: {checker['error']}")
        else:
            print(f"Red pixels: {checker['red_pixels']} ({checker['red_ratio']*100:.1f}%)")
            print(f"Blue pixels: {checker['blue_pixels']} ({checker['blue_ratio']*100:.1f}%)")
            print(f"Balance score: {checker['balance_score']:.3f}")
            print(f"Pattern score: {checker['pattern_score']:.3f}")
            print(f"Overall score: {checker['overall_score']:.3f}")
        print()
    
    # Color statistics
    print("=== Color Statistics ===")
    pixels = image.reshape(-1, 3)
    unique_colors = len(np.unique(pixels, axis=0))
    
    # Get most common colors
    pixel_tuples = [tuple(p) for p in pixels]
    color_counts = Counter(pixel_tuples)
    
    print(f"Unique colors: {unique_colors}")
    print(f"Most common colors:")
    for color, count in color_counts.most_common(5):
        percentage = 100 * count / len(pixels)
        print(f"  RGB{color}: {count} pixels ({percentage:.1f}%)")

if __name__ == '__main__':
    main()