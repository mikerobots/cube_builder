with open('build/bin/test_debug.ppm.ppm', 'rb') as f:
    header = f.readline()
    dims = f.readline()
    maxval = f.readline()
    width, height = map(int, dims.decode().strip().split())
    data = f.read()

print(f"Image size: {width}x{height}")

# Find all yellow pixels
yellow_coords = []
for y in range(height):
    for x in range(width):
        idx = (y * width + x) * 3
        if idx < len(data) - 2:
            r, g, b = data[idx], data[idx+1], data[idx+2]
            if r > 200 and g > 200 and b < 50:
                yellow_coords.append((x, y))

print(f"\nFound {len(yellow_coords)} yellow pixels")

if yellow_coords:
    # Find bounds
    min_x = min(x for x, y in yellow_coords)
    max_x = max(x for x, y in yellow_coords)
    min_y = min(y for x, y in yellow_coords)
    max_y = max(y for x, y in yellow_coords)
    
    print(f"Yellow pixel bounds: ({min_x},{min_y}) to ({max_x},{max_y})")
    print(f"Size: {max_x - min_x + 1} x {max_y - min_y + 1} pixels")
    print(f"Center: ({(min_x + max_x) // 2}, {(min_y + max_y) // 2})")
    
    # Show first few pixels
    print("\nFirst 10 yellow pixels:")
    for i, (x, y) in enumerate(yellow_coords[:10]):
        idx = (y * width + x) * 3
        r, g, b = data[idx], data[idx+1], data[idx+2]
        print(f"  ({x},{y}): R={r}, G={g}, B={b}")