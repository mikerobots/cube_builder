#version 330 core

// Updated version of test_fixed_color.frag for OpenGL 3.3
out vec4 fragColor;

void main() {
    // Fixed red color for testing
    fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}