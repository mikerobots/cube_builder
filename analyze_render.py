#!/usr/bin/env python3

import struct
import sys
import os

def analyze_ppm(filename):
    with open(filename, 'rb') as f:
        # Read header
        magic = f.readline().decode().strip()
        if magic != 'P6':
            print('Not a binary PPM file:', magic)
            return
        
        # Skip comments
        line = f.readline().decode().strip()
        while line.startswith('#'):
            line = f.readline().decode().strip()
        
        # Parse dimensions
        dims = line.split()
        width, height = int(dims[0]), int(dims[1])
        
        # Read max value
        maxval = int(f.readline().decode().strip())
        
        print('PPM Image: {}x{}, max={}'.format(width, height, maxval))
        
        # Read some pixel data to analyze
        pixels_to_read = min(1000, width * height)
        pixel_data = f.read(pixels_to_read * 3)
        
        # Analyze first few pixels
        print('First 10 pixels (RGB):')
        for i in range(min(10, len(pixel_data) // 3)):
            r, g, b = pixel_data[i*3], pixel_data[i*3+1], pixel_data[i*3+2]
            print('  Pixel {}: ({}, {}, {})'.format(i, r, g, b))
        
        # Calculate color statistics
        if len(pixel_data) >= 300:  # At least 100 pixels
            r_vals = [pixel_data[i] for i in range(0, len(pixel_data), 3)]
            g_vals = [pixel_data[i] for i in range(1, len(pixel_data), 3)]
            b_vals = [pixel_data[i] for i in range(2, len(pixel_data), 3)]
            
            print('Color analysis (first {} pixels):'.format(len(r_vals)))
            print('  Red: avg={:.1f}, min={}, max={}'.format(sum(r_vals)/len(r_vals), min(r_vals), max(r_vals)))
            print('  Green: avg={:.1f}, min={}, max={}'.format(sum(g_vals)/len(g_vals), min(g_vals), max(g_vals)))
            print('  Blue: avg={:.1f}, min={}, max={}'.format(sum(b_vals)/len(b_vals), min(b_vals), max(b_vals)))

if __name__ == '__main__':
    if len(sys.argv) > 1:
        analyze_ppm(sys.argv[1])
    else:
        print('Usage: python3 analyze_render.py <ppm_file>')