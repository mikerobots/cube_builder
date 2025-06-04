# Analyze the screenshot for red pixels
import sys
print('Analyzing screenshot...')

try:
    with open('test_voxel_debug.ppm.ppm', 'rb') as f:
        # Read PPM header
        magic = f.readline().decode().strip()
        if magic != 'P6':
            print(f'Invalid PPM magic: {magic}')
            sys.exit(1)
        
        # Skip comments
        line = f.readline().decode().strip()
        while line.startswith('#'):
            line = f.readline().decode().strip()
        
        # Get dimensions
        width, height = map(int, line.split())
        max_val = int(f.readline().decode().strip())
        
        print(f'Image size: {width}x{height}, max value: {max_val}')
        
        # Read pixels
        red_count = 0
        total_pixels = width * height
        pixels = f.read(total_pixels * 3)
        
        for i in range(0, len(pixels), 3):
            r, g, b = pixels[i], pixels[i+1], pixels[i+2]
            if r > 200 and g < 50 and b < 50:
                red_count += 1
        
        print(f'Red pixels found: {red_count}/{total_pixels} ({red_count/total_pixels*100:.2f}%)')
        
        if red_count == 0:
            print('NO RED PIXELS FOUND - Voxel is not visible!')
        else:
            print('Red voxel is visible')
            
except Exception as e:
    print(f'Error reading file: {e}')