#!/usr/bin/env python3

data = open('test_voxels_clean.ppm', 'rb').read()
header_end = data.find(b'255\n') + 4
w, h = 1280, 720

# Count unique colors
unique_colors = set()
for i in range(0, len(data) - header_end - 2, 3):
    rgb = data[header_end + i:header_end + i + 3]
    unique_colors.add(tuple(rgb))

print(f"Unique colors found: {len(unique_colors)}")
for color in sorted(unique_colors)[:20]:  # Show first 20 colors
    print(f"  RGB{color}")

# Check if background is dark gray (0.2, 0.2, 0.2) = (51, 51, 51)
dark_gray_count = sum(1 for i in range(0, len(data) - header_end - 2, 3) 
                     if data[header_end + i:header_end + i + 3] == b'\x33\x33\x33')
print(f"\nDark gray pixels (51,51,51): {dark_gray_count}")

# Check for brownish voxel color (0.3, 0.24, 0.18) = (76, 61, 46)
voxel_color_count = sum(1 for i in range(0, len(data) - header_end - 2, 3) 
                       if data[header_end + i:header_end + i + 3] == b'\x4c\x3d\x2e')
print(f"Voxel color pixels (76,61,46): {voxel_color_count}")

# Sample some pixels
print(f"\nPixel samples:")
print(f"  Top-left (0,0): RGB{tuple(data[header_end:header_end+3])}")
print(f"  Center (640,360): RGB{tuple(data[header_end + (360*w + 640)*3:header_end + (360*w + 640)*3 + 3])}")
print(f"  Bottom-right (1279,719): RGB{tuple(data[header_end + (719*w + 1279)*3:header_end + (719*w + 1279)*3 + 3])}")