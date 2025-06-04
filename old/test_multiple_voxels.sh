#\!/bin/bash
# Test multiple voxels rendering

echo "=== Testing Multiple Voxels ==="

# Test 1: 3x3 grid of voxels at z=1
echo -e "workspace 10 10 10\nresolution 32cm\nplace 2 2 1\nplace 4 2 1\nplace 6 2 1\nplace 2 4 1\nplace 4 4 1\nplace 6 4 1\nplace 2 6 1\nplace 4 6 1\nplace 6 6 1\ncamera front\nscreenshot test_grid.ppm\nquit" | ./voxel-cli > /dev/null 2>&1

# Check the result
if [ -f "test_grid.ppm.ppm" ]; then
    python3 -c "
with open('test_grid.ppm.ppm', 'rb') as f:
    f.readline(); dims = f.readline().decode().strip()
    w, h = map(int, dims.split())
    f.readline(); data = f.read()
    
    # Find all red pixels and cluster them
    red_pixels = []
    for i in range(0, len(data), 3):
        if data[i] > 200 and data[i+1] < 50 and data[i+2] < 50:  # Red-ish
            pixel_idx = i // 3
            y = pixel_idx // w
            x = pixel_idx % w
            red_pixels.append((x, y))
    
    print(f'Total red pixels: {len(red_pixels)}')
    print(f'Expected pixels per voxel: ~36,864')
    print(f'Estimated visible voxels: {len(red_pixels) // 36864}')"
fi

# Test 2: Line of voxels
echo -e "\n=== Testing Line of Voxels ==="
echo -e "workspace 10 10 10\nresolution 32cm\nplace 1 5 5\nplace 3 5 5\nplace 5 5 5\nplace 7 5 5\nplace 9 5 5\ncamera left\nscreenshot test_line.ppm\nquit" | ./voxel-cli > /dev/null 2>&1

if [ -f "test_line.ppm.ppm" ]; then
    python3 -c "
with open('test_line.ppm.ppm', 'rb') as f:
    f.readline(); dims = f.readline().decode().strip()
    w, h = map(int, dims.split())
    f.readline(); data = f.read()
    
    red_count = sum(1 for i in range(0, len(data), 3) if data[i] > 200 and data[i+1] < 50 and data[i+2] < 50)
    print(f'Total red pixels: {red_count}')
    print(f'Estimated visible voxels: {red_count // 36864}')"
fi
