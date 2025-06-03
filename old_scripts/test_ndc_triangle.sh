#!/bin/bash

echo "Testing with NDC triangle..."

# Create a test that renders a triangle directly in NDC space
cat > test_ndc.cpp << 'EOF'
#include "apps/cli/src/Application.cpp"

// Override the vertex shader to output NDC coordinates directly
const char* TEST_VERTEX_SHADER = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aColor;
out vec3 Color;
void main() {
    Color = vec3(1.0, 0.0, 0.0); // Red
    // Place triangle in center of screen
    if (gl_VertexID % 3 == 0) gl_Position = vec4(-0.5, -0.5, 0.0, 1.0);
    else if (gl_VertexID % 3 == 1) gl_Position = vec4(0.5, -0.5, 0.0, 1.0);
    else gl_Position = vec4(0.0, 0.5, 0.0, 1.0);
}
)";
EOF

# Build and run test
./build/bin/VoxelEditor_CLI << 'EOF'
resolution 128cm
place 0 0 0
screenshot test_ndc.png
quit
EOF

if [ -f test_ndc.ppm ]; then
    echo "Checking for red triangle..."
    python3 -c "
data = open('test_ndc.ppm', 'rb').read()
header_end = data.find(b'255\\n') + 4
pixels = data[header_end:header_end+100000]
red_found = False
for i in range(0, len(pixels)-2, 3):
    r,g,b = pixels[i:i+3]
    if r > 200 and g < 50 and b < 50:
        red_found = True
        break
print('Red triangle found!' if red_found else 'No red pixels')
"
fi