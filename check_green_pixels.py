#!/usr/bin/env python3
import sys

def check_ppm_for_colors(filename):
    with open(filename, 'rb') as f:
        # Read header
        magic = f.readline().decode().strip()
        if magic != 'P6':
            print(f"Not a binary PPM file: {magic}")
            return
        
        # Read dimensions
        dims = f.readline().decode().strip()
        width, height = map(int, dims.split())
        max_val = int(f.readline().decode().strip())
        
        print(f"Image: {width}x{height}, max value: {max_val}")
        
        # Sample center area
        center_x = width // 2
        center_y = height // 2
        sample_size = 100
        
        red_pixels = 0
        green_pixels = 0
        blue_pixels = 0
        
        # Calculate header size
        header_size = len("P6\n") + len(f"{width} {height}\n") + len(f"{max_val}\n")
        
        # Read center area
        for y in range(center_y - sample_size//2, center_y + sample_size//2):
            pixel_offset = y * width + center_x - sample_size//2
            f.seek(header_size + 3 * pixel_offset)
            for x in range(sample_size):
                rgb = f.read(3)
                if len(rgb) == 3:
                    r, g, b = rgb[0], rgb[1], rgb[2]
                    if r > 200 and g < 50 and b < 50:
                        red_pixels += 1
                    elif r < 50 and g > 200 and b < 50:
                        green_pixels += 1
                    elif r < 50 and g < 50 and b > 100:
                        blue_pixels += 1
        
        print(f"In center {sample_size}x{sample_size} area:")
        print(f"  Red pixels: {red_pixels}")
        print(f"  Green pixels: {green_pixels}")
        print(f"  Blue pixels: {blue_pixels}")

if __name__ == "__main__":
    filename = sys.argv[1] if len(sys.argv) > 1 else "triangle_test.ppm.ppm"
    check_ppm_for_colors(filename)