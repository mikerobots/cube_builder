#\!/bin/bash
# Final comprehensive voxel rendering test

echo "=== Comprehensive Voxel Rendering Test ==="

# Test 1: Simple two voxels
echo -e "\n--- Test 1: Two voxels ---"
echo -e "workspace 10 10 10\nresolution 32cm\nplace 2 5 5\nplace 8 5 5\nscreenshot test1.ppm\nquit" | ./voxel-cli > /dev/null 2>&1

python3 -c "
with open('test1.ppm.ppm', 'rb') as f:
    f.readline(); dims = f.readline().decode().strip()
    w, h = map(int, dims.split())
    f.readline(); data = f.read()
    red = sum(1 for i in range(0, len(data), 3) if data[i] == 255 and data[i+1] == 0 and data[i+2] == 0)
    print(f'Test 1: {red} red pixels ({red/36864:.1f} voxels)')"

# Test 2: Vertical stack
echo -e "\n--- Test 2: Vertical stack ---"  
echo -e "workspace 10 10 10\nresolution 32cm\nplace 5 5 2\nplace 5 5 5\nplace 5 5 8\nscreenshot test2.ppm\nquit" | ./voxel-cli > /dev/null 2>&1

python3 -c "
with open('test2.ppm.ppm', 'rb') as f:
    f.readline(); dims = f.readline().decode().strip()
    w, h = map(int, dims.split())
    f.readline(); data = f.read()
    red = sum(1 for i in range(0, len(data), 3) if data[i] == 255 and data[i+1] == 0 and data[i+2] == 0)
    print(f'Test 2: {red} red pixels ({red/36864:.1f} voxels)')"

# Test 3: Large workspace, single voxel
echo -e "\n--- Test 3: Large workspace ---"
echo -e "workspace 20 20 20\nresolution 64cm\nplace 10 10 10\nscreenshot test3.ppm\nquit" | ./voxel-cli > /dev/null 2>&1

python3 -c "
with open('test3.ppm.ppm', 'rb') as f:
    f.readline(); dims = f.readline().decode().strip()
    w, h = map(int, dims.split())
    f.readline(); data = f.read()
    red = sum(1 for i in range(0, len(data), 3) if data[i] == 255 and data[i+1] == 0 and data[i+2] == 0)
    print(f'Test 3: {red} red pixels ({red/36864:.1f} voxels)')"

# Clean up
rm -f test1.ppm.ppm test2.ppm.ppm test3.ppm.ppm
