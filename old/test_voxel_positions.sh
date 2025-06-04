#\!/bin/bash
# Test voxel positions and visibility

echo "=== Testing Voxel Positions ==="

# Create voxels in a diagonal line
echo -e "workspace 10 10 10\nresolution 32cm\nplace 1 1 1\nplace 2 2 2\nplace 3 3 3\nplace 4 4 4\nplace 5 5 5\ncamera orbit 45 45 15\nscreenshot test_diagonal.ppm\nquit" | ./voxel-cli 2>&1 | grep -E "(Voxel placed|world pos|Camera position|Target)" | tail -20

# Analyze the screenshot
if [ -f "test_diagonal.ppm.ppm" ]; then
    python3 -c "
import sys
with open('test_diagonal.ppm.ppm', 'rb') as f:
    f.readline()  # magic
    dims = f.readline().decode().strip()
    w, h = map(int, dims.split())
    f.readline()  # maxval
    
    # Read limited data to avoid memory issues
    chunk_size = w * h * 3
    data = f.read(chunk_size)
    
    # Find red pixel clusters
    red_regions = []
    current_region = []
    
    for y in range(h):
        for x in range(w):
            idx = (y * w + x) * 3
            if idx + 2 < len(data):
                r, g, b = data[idx], data[idx+1], data[idx+2]
                if r > 200 and g < 50 and b < 50:  # Red
                    current_region.append((x, y))
                elif current_region:
                    if len(current_region) > 100:  # Significant region
                        min_x = min(p[0] for p in current_region)
                        max_x = max(p[0] for p in current_region)
                        min_y = min(p[1] for p in current_region)
                        max_y = max(p[1] for p in current_region)
                        center_x = (min_x + max_x) // 2
                        center_y = (min_y + max_y) // 2
                        red_regions.append({
                            'pixels': len(current_region),
                            'center': (center_x, center_y),
                            'size': (max_x - min_x + 1, max_y - min_y + 1)
                        })
                    current_region = []
    
    print(f'\\nImage size: {w} x {h}')
    print(f'Found {len(red_regions)} distinct red regions')
    for i, region in enumerate(red_regions[:5]):
        print(f'  Region {i}: {region[\"pixels\"]} pixels at {region[\"center\"]}, size {region[\"size\"]}')"
fi
