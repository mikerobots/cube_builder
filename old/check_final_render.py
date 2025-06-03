#!/usr/bin/env python3

data = open('test_voxels_fixed.ppm', 'rb').read()
header_end = data.find(b'255\n') + 4
w, h = 2560, 1440  # High DPI display

# Count unique colors
unique_colors = set()
for i in range(0, len(data) - header_end - 2, 3):
    rgb = data[header_end + i:header_end + i + 3]
    unique_colors.add(tuple(rgb))

print(f"Unique colors found: {len(unique_colors)}")
for color in sorted(unique_colors)[:30]:  # Show first 30 colors
    print(f"  RGB{color}")

# Check for voxel-like colors
dark_gray = sum(1 for i in range(0, len(data) - header_end - 2, 3) 
                if data[header_end + i:header_end + i + 3] == b'\x33\x33\x33')
print(f"\nDark gray background pixels (51,51,51): {dark_gray}")

# Count non-background pixels
non_bg = sum(1 for i in range(0, len(data) - header_end - 2, 3) 
             if data[header_end + i:header_end + i + 3] != b'\x33\x33\x33')
print(f"Non-background pixels: {non_bg} ({non_bg*100.0/(w*h):.1f}%)")

# Sample center area
print(f"\nSampling center area:")
for y in range(350, 370, 2):
    for x in range(630, 650, 2):
        idx = header_end + (y * w + x) * 3
        rgb = tuple(data[idx:idx+3])
        if rgb != (51, 51, 51):
            print(f"  ({x},{y}): RGB{rgb}")