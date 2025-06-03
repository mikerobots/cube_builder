#!/usr/bin/env python3

# Check for colored squares
with open('test_colored_squares.ppm.ppm', 'rb') as f:
    # Read PPM header
    magic = f.readline().decode().strip()
    line = f.readline().decode().strip()
    while line.startswith('#'):
        line = f.readline().decode().strip()
    width, height = map(int, line.split())
    max_val = int(f.readline().decode().strip())
    
    # Read pixels and count colors
    red_count = green_count = blue_count = yellow_count = 0
    total_pixels = width * height
    pixels = f.read(total_pixels * 3)
    
    for i in range(0, len(pixels), 3):
        r, g, b = pixels[i], pixels[i+1], pixels[i+2]
        if r > 200 and g < 50 and b < 50:
            red_count += 1
        elif g > 200 and r < 50 and b < 50:
            green_count += 1
        elif b > 200 and r < 50 and g < 50:
            blue_count += 1
        elif r > 200 and g > 200 and b < 50:
            yellow_count += 1
    
    print(f'Image size: {width}x{height}')
    print(f'Red pixels: {red_count} ({red_count/total_pixels*100:.2f}%)')
    print(f'Green pixels: {green_count} ({green_count/total_pixels*100:.2f}%)')
    print(f'Blue pixels: {blue_count} ({blue_count/total_pixels*100:.2f}%)')
    print(f'Yellow pixels: {yellow_count} ({yellow_count/total_pixels*100:.2f}%)')
    
    total_colored = red_count + green_count + blue_count + yellow_count
    print(f'\nTotal colored pixels: {total_colored} ({total_colored/total_pixels*100:.2f}%)')
    
    if total_colored > 0:
        print('✓ Immediate mode rendering WORKS!')
    else:
        print('✗ NO RENDERING AT ALL!')