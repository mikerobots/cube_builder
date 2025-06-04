#!/usr/bin/env python3
import sys

def check_ppm_corners(filename):
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
        
        # Calculate header size
        header_size = len("P6\n") + len(f"{width} {height}\n") + len(f"{max_val}\n")
        
        # Check top-right corner (where we drew the red square)
        corner_x = width - 100
        corner_y = 100  # Top in screen coords
        
        red_pixels = 0
        
        for y in range(50):  # Check top 50 rows
            pixel_offset = y * width + corner_x
            f.seek(header_size + 3 * pixel_offset)
            for x in range(100):  # Check rightmost 100 pixels
                rgb = f.read(3)
                if len(rgb) == 3:
                    r, g, b = rgb[0], rgb[1], rgb[2]
                    if r > 200 and g < 50 and b < 50:
                        red_pixels += 1
        
        print(f"Red pixels in top-right corner: {red_pixels}")

if __name__ == "__main__":
    filename = sys.argv[1] if len(sys.argv) > 1 else "gl_errors.ppm.ppm"
    check_ppm_corners(filename)