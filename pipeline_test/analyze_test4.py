#!/usr/bin/env python3

import sys

try:
    with open('test4_MainAppVoxel.ppm', 'rb') as f:
        data = f.read()
    
    # Find header end (look for "255\n")
    header_end = data.find(b'255\n') + 4
    
    # Count unique colors
    unique_colors = set()
    for i in range(header_end, len(data) - 2, 3):
        rgb = data[i:i+3]
        unique_colors.add(tuple(rgb))
    
    print(f"Unique colors: {len(unique_colors)}")
    
    # Check for background vs non-background
    bg_color = (51, 51, 51)  # Dark gray
    non_bg_pixels = 0
    total_pixels = (len(data) - header_end) // 3
    
    for i in range(header_end, len(data) - 2, 3):
        rgb = tuple(data[i:i+3])
        if rgb != bg_color:
            non_bg_pixels += 1
    
    coverage = (non_bg_pixels * 100.0) / total_pixels if total_pixels > 0 else 0
    print(f"Coverage: {coverage:.1f}% ({non_bg_pixels}/{total_pixels} pixels)")
    
    # Show some example colors
    print(f"Sample colors from unique set: {list(unique_colors)[:5]}")
    
    if len(unique_colors) > 1:
        print("✅ GEOMETRY DETECTED")
    else:
        print("❌ NO GEOMETRY DETECTED")

except Exception as e:
    print(f"Error analyzing: {e}")