#\!/bin/bash
# Test rendering with detailed debug output

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build_ninja"
VOXEL_CLI="$BUILD_DIR/bin/VoxelEditor_CLI"

echo "=== Testing Voxel Rendering with Debug Output ==="

# Create a named pipe for commands
FIFO="/tmp/voxel_test_$$"
mkfifo "$FIFO"

# Start the application with the FIFO
"$VOXEL_CLI" < "$FIFO" 2>&1 &
CLI_PID=$\!

# Give it time to start
sleep 2

# Send commands
{
    echo "workspace 5 5 5"
    sleep 0.5
    echo "resolution 8cm"
    sleep 0.5
    echo "place 2 2 2"
    sleep 1
    echo "screenshot test_render_1.ppm"
    sleep 1
    echo "place 3 2 2"
    sleep 1
    echo "screenshot test_render_2.ppm"
    sleep 1
    echo "place 2 3 2"
    sleep 1
    echo "screenshot test_render_3.ppm"
    sleep 2
    echo "quit"
} > "$FIFO" &

# Wait for the CLI to exit
wait $CLI_PID

# Clean up
rm -f "$FIFO"

echo -e "\n=== Checking screenshots ==="
for i in 1 2 3; do
    if [ -f "test_render_$i.ppm.ppm" ]; then
        echo "test_render_$i.ppm.ppm exists"
        python3 -c "
with open('test_render_$i.ppm.ppm', 'rb') as f:
    f.readline()  # magic
    dims = f.readline().decode().strip()
    w, h = map(int, dims.split())
    f.readline()  # maxval
    data = f.read(w*h*3)
    
    # Check for non-gray pixels
    colors = {}
    for i in range(0, min(len(data), 10000*3), 3):
        color = (data[i], data[i+1], data[i+2])
        colors[color] = colors.get(color, 0) + 1
    
    print(f'  Top 5 colors in first 10k pixels:')
    for color, count in sorted(colors.items(), key=lambda x: -x[1])[:5]:
        print(f'    RGB{color}: {count} pixels')
"
    fi
done
