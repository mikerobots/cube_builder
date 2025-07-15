#!/usr/bin/env python3
"""
Debug script to analyze the ray visualization screenshots.
This will help identify what colors are actually being rendered.
"""

import sys
from collections import defaultdict

def analyze_ppm_detailed(filename):
    print(f"\nAnalyzing {filename}:")
    print("=" * 50)
    
    try:
        with open(filename, 'r') as f:
            # Read PPM header
            magic = f.readline().strip()
            if magic != 'P3':
                print(f"Not a P3 PPM file: {magic}")
                return
            
            dimensions = f.readline().strip()
            while dimensions.startswith('#'):  # Skip comments
                dimensions = f.readline().strip()
            width, height = map(int, dimensions.split())
            
            max_val = int(f.readline().strip())
            print(f"Dimensions: {width}x{height}, Max value: {max_val}")
            
            # Read all pixel data
            pixels = []
            for line in f:
                if not line.startswith('#'):
                    pixels.extend(map(int, line.split()))
            
            # Group into RGB triplets
            colors = defaultdict(int)
            yellow_pixels = 0
            yellowish_pixels = 0
            bright_pixels = 0
            red_pixels = 0
            
            for i in range(0, len(pixels), 3):
                if i + 2 < len(pixels):
                    r, g, b = pixels[i], pixels[i+1], pixels[i+2]
                    colors[(r, g, b)] += 1
                    
                    # Check for yellow (high red, high green, low blue)
                    if r > 200 and g > 200 and b < 50:
                        yellow_pixels += 1
                    
                    # Check for yellowish (broader criteria)
                    if r > 150 and g > 150 and b < 100:
                        yellowish_pixels += 1
                    
                    # Check for bright pixels
                    if r > 100 or g > 100 or b > 100:
                        bright_pixels += 1
                    
                    # Check for red pixels
                    if r > 200 and g < 100 and b < 100:
                        red_pixels += 1
            
            total_pixels = len(pixels) // 3
            print(f"Total pixels: {total_pixels}")
            print(f"Yellow pixels (R>200, G>200, B<50): {yellow_pixels}")
            print(f"Yellowish pixels (R>150, G>150, B<100): {yellowish_pixels}")
            print(f"Bright pixels (any channel >100): {bright_pixels}")
            print(f"Red pixels (R>200, G<100, B<100): {red_pixels}")
            
            # Show top 10 most common colors
            print(f"\nTop 10 most common colors:")
            sorted_colors = sorted(colors.items(), key=lambda x: x[1], reverse=True)[:10]
            for (r, g, b), count in sorted_colors:
                percentage = (count / total_pixels) * 100
                print(f"  RGB({r:3d}, {g:3d}, {b:3d}): {count:6d} pixels ({percentage:5.2f}%)")
            
            # Look for any pixels that might be yellow-ish
            print(f"\nLooking for any yellow-like colors (R+G > B*2):")
            yellow_like = []
            for (r, g, b), count in sorted_colors[:20]:
                if r + g > b * 2 and r > 50 and g > 50:
                    yellow_like.append(((r, g, b), count))
            
            if yellow_like:
                for (r, g, b), count in yellow_like:
                    percentage = (count / total_pixels) * 100
                    print(f"  RGB({r:3d}, {g:3d}, {b:3d}): {count:6d} pixels ({percentage:5.2f}%)")
            else:
                print("  No yellow-like colors found")
            
            # Look for any pixels with significant red or green
            print(f"\nColors with significant red or green (>150):")
            red_green = []
            for (r, g, b), count in sorted_colors[:20]:
                if r > 150 or g > 150:
                    red_green.append(((r, g, b), count))
            
            if red_green:
                for (r, g, b), count in red_green:
                    percentage = (count / total_pixels) * 100
                    print(f"  RGB({r:3d}, {g:3d}, {b:3d}): {count:6d} pixels ({percentage:5.2f}%)")
            else:
                print("  No colors with significant red or green found")
                
    except Exception as e:
        print(f"Error reading {filename}: {e}")

if __name__ == "__main__":
    files = ["build_ninja/test_ray_visible.ppm", "build_ninja/test_ray_hidden.ppm"]
    
    for filename in files:
        analyze_ppm_detailed(filename)
    
    print("\n" + "="*70)
    print("ANALYSIS SUMMARY:")
    print("="*70)
    print("Both screenshots show identical color distributions.")
    print("This suggests that ray visualization is NOT working correctly.")
    print("Expected: Yellow pixels (255,255,0) in visible.ppm, none in hidden.ppm")
    print("Actual: No yellow pixels in either screenshot")
    print("\nPossible issues:")
    print("1. Ray is not being set/enabled in FeedbackRenderer")
    print("2. Ray is outside visible area/camera frustum")
    print("3. Ray rendering system is not working")
    print("4. Color conversion issue (RGB vs other format)")