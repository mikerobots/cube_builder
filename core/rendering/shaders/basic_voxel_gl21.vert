#version 120

attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec3 a_color;

varying vec3 v_fragPos;
varying vec3 v_normal;
varying vec3 v_color;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main() {
    v_fragPos = vec3(u_model * vec4(a_position, 1.0));
    v_normal = mat3(u_model) * a_normal; // Simplified normal transform for now
    v_color = a_color;
    
    gl_Position = u_projection * u_view * vec4(v_fragPos, 1.0);
}