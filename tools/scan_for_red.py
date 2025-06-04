#!/usr/bin/env python3
import sys

def scan_ppm_for_red(filename):
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
        
        # Scan entire image for red pixels
        f.seek(header_size)
        red_found = False
        pixel_count = 0
        
        for y in range(height):
            for x in range(width):
                rgb = f.read(3)
                if len(rgb) == 3:
                    r, g, b = rgb[0], rgb[1], rgb[2]
                    if r > 200 and g < 50 and b < 50:
                        if not red_found:
                            print(f"First red pixel found at ({x}, {y})")
                            red_found = True
                        pixel_count += 1
                        if pixel_count < 10:
                            print(f"  Red at ({x}, {y}): RGB({r}, {g}, {b})")
        
        print(f"Total red pixels: {pixel_count}")

if __name__ == "__main__":
    filename = sys.argv[1] if len(sys.argv) > 1 else "gl_errors.ppm.ppm"
    scan_ppm_for_red(filename)