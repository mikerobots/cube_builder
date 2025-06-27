#version 330 core

// Updated version of basic_voxel_gl21.vert for OpenGL 3.3
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_color;  // Note: vec4 to match RenderEngine

out vec3 FragPos;
out vec3 Normal;
out vec4 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(a_position, 1.0));
    Normal = mat3(transpose(inverse(model))) * a_normal;
    Color = a_color;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}