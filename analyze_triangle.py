#!/usr/bin/env python3
import sys

img = open('test_render_output.ppm', 'rb').read()
lines = img.decode('latin-1').split('\n')
w, h = map(int, lines[1].split())
pixel_data = '\n'.join(lines[3:])

# Analyze center area for triangle
center_x, center_y = w//2, h//2
box_size = 200

colors = {}
for y in range(center_y - box_size//2, center_y + box_size//2):
    for x in range(center_x - box_size//2, center_x + box_size//2):
        idx = (y * w + x) * 3
        if idx+2 < len(pixel_data):
            r, g, b = ord(pixel_data[idx]), ord(pixel_data[idx+1]), ord(pixel_data[idx+2])
            color = (r, g, b)
            colors[color] = colors.get(color, 0) + 1

# Find non-background colors
bg_color = (26, 26, 77)
triangle_colors = [(c, cnt) for c, cnt in colors.items() if c != bg_color]
triangle_colors.sort(key=lambda x: x[1], reverse=True)

print(f'Center {box_size}x{box_size} area analysis:')
print(f'Background pixels: {colors.get(bg_color, 0)}')
print(f'Triangle pixels: {sum(cnt for c, cnt in triangle_colors)}')
print('\nTop triangle colors:')
for color, count in triangle_colors[:10]:
    r, g, b = color
    print(f'  RGB({r:3},{g:3},{b:3}): {count:4} pixels')

# Check if we have the expected vertex colors
red_pixels = sum(1 for (r,g,b), _ in colors.items() if r > 200 and g < 100 and b < 100)
green_pixels = sum(1 for (r,g,b), _ in colors.items() if r < 100 and g > 200 and b < 100)
blue_pixels = sum(1 for (r,g,b), _ in colors.items() if r < 100 and g < 100 and b > 200)

print(f'\nVertex color analysis:')
print(f'  Red-ish pixels: {red_pixels}')
print(f'  Green-ish pixels: {green_pixels}')
print(f'  Blue-ish pixels: {blue_pixels}')