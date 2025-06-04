#!/bin/bash
# Create a patched version without texture sampling

echo "=== Patching shader to remove texture sampling ==="

# Backup original
cp core/rendering/RenderEngine.cpp core/rendering/RenderEngine.cpp.bak

# Create patch
cat > shader_patch.txt << 'EOF'
--- a/core/rendering/RenderEngine.cpp
+++ b/core/rendering/RenderEngine.cpp
@@ -487,11 +487,8 @@
         uniform float u_emission;
         
         uniform vec4 u_ambientLight;
         uniform vec3 u_lightDirection;
         uniform vec4 u_lightColor;
         uniform int u_enableLighting;
         
-        uniform sampler2D u_albedoTexture;
-        uniform int u_hasAlbedoTexture;
         
         void main() {
             vec4 albedo = u_albedo * v_color;
-            if (u_hasAlbedoTexture > 0) {
-                albedo *= texture2D(u_albedoTexture, v_texCoord);
-            }
             
             if (u_enableLighting > 0) {
                 vec3 normal = normalize(v_normal);
EOF

# Apply patch
patch -p1 < shader_patch.txt

echo "Patch applied. Building..."