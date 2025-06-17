import sys

def analyze_white_pixels(filename):
    with open(filename, 'rb') as f:
        # Read PPM header
        magic = f.readline().decode('ascii').strip()
        if magic \!= 'P6':
            print(f"Error: Not a P6 PPM file (found {magic})")
            return
        
        # Skip comments
        line = f.readline().decode('ascii').strip()
        while line.startswith('#'):
            line = f.readline().decode('ascii').strip()
        
        # Parse dimensions
        width, height = map(int, line.split())
        max_val = int(f.readline().decode('ascii').strip())
        
        print(f"Image dimensions: {width}x{height}")
        
        # Read pixel data
        pixels = f.read()
    
    # Find white pixels at bottom
    white_rows = []
    for y in range(height):
        row_has_white = False
        for x in range(width):
            idx = (y * width + x) * 3
            r, g, b = pixels[idx], pixels[idx+1], pixels[idx+2]
            if r == 255 and g == 255 and b == 255:
                row_has_white = True
                break
        if row_has_white:
            white_rows.append(y)
    
    # Check if white pixels are at bottom
    if white_rows:
        print(f"\nRows with white pixels: {len(white_rows)} rows")
        print(f"First white row: {white_rows[0]} (y={white_rows[0]})")
        print(f"Last white row: {white_rows[-1]} (y={white_rows[-1]})")
        
        # Check if concentrated at bottom
        bottom_start = height * 0.8  # Bottom 20% of image
        bottom_whites = [r for r in white_rows if r >= bottom_start]
        print(f"\nWhite rows in bottom 20%: {len(bottom_whites)}")
        
        # Sample some white pixel locations
        print(f"\nSampling white pixel locations:")
        count = 0
        for y in range(height-1, -1, -1):  # Start from bottom
            for x in range(width):
                idx = (y * width + x) * 3
                r, g, b = pixels[idx], pixels[idx+1], pixels[idx+2]
                if r == 255 and g == 255 and b == 255:
                    print(f"  White pixel at ({x}, {y})")
                    count += 1
                    if count >= 10:
                        return

if __name__ == "__main__":
    analyze_white_pixels(sys.argv[1])
