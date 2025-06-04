#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec3 Color;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main() {
    // Simple flat shading with face-based coloring
    vec3 norm = normalize(Normal);
    
    // Base color with face-dependent tinting
    vec3 faceColor = Color;
    
    // Apply different brightness to each face based on normal
    float brightness = 0.5;
    
    // Top face - brightest
    if (abs(norm.y - 1.0) < 0.01) {
        brightness = 1.0;
    }
    // Bottom face - darkest
    else if (abs(norm.y + 1.0) < 0.01) {
        brightness = 0.25;
    }
    // Right face
    else if (abs(norm.x - 1.0) < 0.01) {
        brightness = 0.8;
    }
    // Left face
    else if (abs(norm.x + 1.0) < 0.01) {
        brightness = 0.4;
    }
    // Front face
    else if (abs(norm.z - 1.0) < 0.01) {
        brightness = 0.65;
    }
    // Back face
    else if (abs(norm.z + 1.0) < 0.01) {
        brightness = 0.5;
    }
    
    // Apply brightness
    vec3 result = faceColor * brightness;
    
    FragColor = vec4(result, 1.0);
}