#version 330 core

in float vIsMajorLine;

uniform vec3 minorLineColor;
uniform vec3 majorLineColor;
uniform float opacity;

out vec4 fragColor;

void main() {
    // Select color based on line type
    vec3 lineColor = mix(minorLineColor, majorLineColor, vIsMajorLine);
    
    // Apply line width effect for major lines (simulated with opacity)
    float finalOpacity = opacity;
    if (vIsMajorLine > 0.5) {
        finalOpacity *= 1.2; // Make major lines slightly more visible
        finalOpacity = min(finalOpacity, 1.0);
    }
    
    fragColor = vec4(lineColor, finalOpacity);
}