#version 120

varying vec3 v_fragPos;
varying vec3 v_normal;
varying vec3 v_color;

uniform vec3 u_lightPos;
uniform vec3 u_lightColor;
uniform vec3 u_viewPos;

void main() {
    // Simple ambient + diffuse lighting
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * u_lightColor;
    
    // Diffuse lighting
    vec3 norm = normalize(v_normal);
    vec3 lightDir = normalize(u_lightPos - v_fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * u_lightColor;
    
    // Combine results
    vec3 result = (ambient + diffuse) * v_color;
    gl_FragColor = vec4(result, 1.0);
}