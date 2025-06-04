#\!/bin/bash
# Simple test to place a voxel and take a screenshot

echo "=== Simple Voxel Test ==="
echo -e "workspace 5 5 5\nresolution 16cm\nplace 2 2 2\nscreenshot test_simple.ppm\nquit" | ./voxel-cli

# Check the result
if [ -f "test_simple.ppm.ppm" ]; then
    python3 -c "
with open('test_simple.ppm.ppm', 'rb') as f:
    f.readline(); dims = f.readline().decode().strip()
    w, h = map(int, dims.split())
    f.readline(); data = f.read()
    red_count = sum(1 for i in range(0, len(data), 3) if data[i] == 255 and data[i+1] == 0 and data[i+2] == 0)
    purple_count = sum(1 for i in range(0, len(data), 3) if data[i] == 128 and data[i+1] == 0 and data[i+2] == 128)
    print(f'Red pixels (voxels): {red_count}')
    print(f'Purple pixels (background): {purple_count}')
    print(f'Voxel visible: {\"YES\" if red_count > 0 else \"NO\"}')"
fi
