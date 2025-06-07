#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec4 Color;

out vec4 FragColor;

uniform vec3 lightPos = vec3(10.0, 10.0, 10.0);
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
uniform vec3 viewPos = vec3(0.0, 0.0, 5.0);

void main() {
    // Improved edge highlighting using multiple techniques
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // 1. Screen-space derivatives for geometric edge detection
    // These detect discontinuities in the normal and position
    vec3 normalDx = dFdx(norm);
    vec3 normalDy = dFdy(norm);
    vec3 posDx = dFdx(FragPos);
    vec3 posDy = dFdy(FragPos);
    
    // Calculate edge strength from derivatives
    float normalChange = length(normalDx) + length(normalDy);
    float posChange = length(posDx) + length(posDy);
    
    // Normalize and combine edge factors
    float geometricEdge = smoothstep(0.0, 1.0, normalChange * 2.0);
    float positionEdge = smoothstep(0.0, 0.5, posChange * 0.5);
    
    // 2. Enhanced rim lighting for silhouette edges
    float fresnel = 1.0 - max(dot(norm, viewDir), 0.0);
    float rimStrength = pow(fresnel, 1.5); // Wider rim
    
    // 3. Multiple light sources for better definition
    vec3 lightDir1 = normalize(vec3(1.0, 2.0, 1.0));
    vec3 lightDir2 = normalize(vec3(-1.0, 0.5, 0.5));
    vec3 lightDir3 = normalize(vec3(0.0, -1.0, 0.5)); // Bottom light
    
    float diff1 = max(dot(norm, lightDir1), 0.0);
    float diff2 = max(dot(norm, lightDir2), 0.0);
    float diff3 = max(dot(norm, lightDir3), 0.0);
    
    // 4. Face-dependent base lighting
    float faceBrightness = 0.5;
    vec3 faceModulation = vec3(1.0);
    
    // Detect face orientation and apply distinct brightness/color
    if (abs(norm.y - 1.0) < 0.01) {
        // Top face - brightest with slight warm tint
        faceBrightness = 1.0;
        faceModulation = vec3(1.0, 0.98, 0.95);
    }
    else if (abs(norm.y + 1.0) < 0.01) {
        // Bottom face - darkest with cool tint
        faceBrightness = 0.3;
        faceModulation = vec3(0.9, 0.9, 1.0);
    }
    else if (abs(norm.x - 1.0) < 0.01) {
        // Right face - bright with slight red tint
        faceBrightness = 0.85;
        faceModulation = vec3(1.0, 0.95, 0.95);
    }
    else if (abs(norm.x + 1.0) < 0.01) {
        // Left face - medium dark with slight blue tint
        faceBrightness = 0.5;
        faceModulation = vec3(0.95, 0.95, 1.0);
    }
    else if (abs(norm.z - 1.0) < 0.01) {
        // Front face - medium bright with slight green tint
        faceBrightness = 0.7;
        faceModulation = vec3(0.95, 1.0, 0.95);
    }
    else if (abs(norm.z + 1.0) < 0.01) {
        // Back face - medium
        faceBrightness = 0.6;
        faceModulation = vec3(1.0, 1.0, 1.0);
    }
    
    // 5. Combine lighting components
    float ambientStrength = 0.25;
    vec3 ambient = ambientStrength * lightColor;
    
    // Weighted diffuse from multiple lights
    vec3 diffuse = (diff1 * 0.6 + diff2 * 0.3 + diff3 * 0.1) * lightColor;
    
    // Face-specific lighting
    vec3 faceLight = faceBrightness * lightColor * 0.35;
    
    // Enhanced specular for sharp highlights
    vec3 reflectDir = reflect(-lightDir1, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = 0.3 * spec * lightColor;
    
    // 6. Calculate base lit color
    vec3 baseColor = Color.rgb * faceModulation;
    vec3 litColor = (ambient + diffuse + faceLight + specular) * baseColor;
    
    // 7. Apply edge enhancements
    // Combine different edge detection methods
    float edgeIntensity = max(geometricEdge * 0.6, max(positionEdge * 0.3, rimStrength * 0.4));
    
    // Create edge color - dark but not black, with slight color
    vec3 edgeColor = baseColor * 0.2;
    
    // Apply rim lighting as brightening instead of darkening
    vec3 rimLight = lightColor * rimStrength * 0.5;
    litColor += rimLight;
    
    // Apply geometric edge darkening
    litColor = mix(litColor, edgeColor, edgeIntensity * 0.7);
    
    // 8. Additional edge line detection
    // Sharp edge detection for cube edges
    float sharpEdge = 0.0;
    if (normalChange > 0.5) {
        sharpEdge = smoothstep(0.5, 0.7, normalChange);
    }
    
    // Apply sharp edge darkening
    if (sharpEdge > 0.0) {
        litColor *= (1.0 - sharpEdge * 0.5);
    }
    
    // 9. Final adjustments
    // Slight contrast boost
    litColor = pow(litColor, vec3(0.9));
    
    // Ensure we don't go too dark or too bright
    litColor = clamp(litColor, vec3(0.05), vec3(1.0));
    
    FragColor = vec4(litColor, Color.a);
}