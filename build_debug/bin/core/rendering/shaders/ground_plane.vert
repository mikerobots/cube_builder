#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in float isMajorLine;

uniform mat4 mvpMatrix;

out float vIsMajorLine;

void main() {
    gl_Position = mvpMatrix * vec4(position, 1.0);
    vIsMajorLine = isMajorLine;
}