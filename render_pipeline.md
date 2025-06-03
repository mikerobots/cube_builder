# Voxel Rendering Pipeline Documentation

## Overview

This document details the complete rendering pipeline for voxels in the VoxelEditor CLI application, from data structures to final pixel output.

## Pipeline Flow

```
1. Voxel Data Storage (SparseOctree)
   ↓
2. Mesh Generation (VoxelMeshGenerator)
   ↓
3. GPU Buffer Upload (updateVoxelMesh)
   ↓
4. Shader Program Setup (createShaderProgram)
   ↓
5. Per-Frame Rendering (render)
   ↓
6. OpenGL Draw Commands
   ↓
7. Fragment Processing
   ↓
8. Framebuffer Output
```

## 1. Voxel Data Storage

**Location:** `core/voxel_data/SparseOctree.h`

The voxel data is stored in a sparse octree structure that efficiently handles 3D grid data:

```cpp
struct VoxelPosition {
    Math::Vector3i gridPos;  // Grid coordinates (0,0,0), (1,0,0), etc.
    VoxelData::VoxelResolution resolution;
    // Additional properties...
};
```

**Key Functions:**
- `getAllVoxels(resolution)` - Returns all voxels at specified resolution
- Octree nodes store actual voxel positions to avoid coordinate calculation errors

## 2. Mesh Generation

**Location:** `apps/cli/src/VoxelMeshGenerator.cpp`

Converts voxel grid positions to 3D mesh geometry:

### Coordinate Transformation
```cpp
// Grid position → World position
Math::Vector3f worldPos(
    voxelPos.gridPos.x * voxelSize + voxelSize * 0.5f,
    voxelPos.gridPos.y * voxelSize + voxelSize * 0.5f,
    voxelPos.gridPos.z * voxelSize + voxelSize * 0.5f
);
```

### Cube Generation
Each voxel becomes a cube with:
- **8 vertices** per cube (unit cube template)
- **6 faces** with 4 vertices each = **24 vertices total**
- **12 triangles** (2 per face) = **36 indices**

**Vertex Structure** (`core/rendering/RenderTypes.h`):
```cpp
struct Vertex {
    Math::Vector3f position;  // World coordinates
    Math::Vector3f normal;    // Face normal
    Math::Vector2f texCoords; // Texture coordinates (unused)
    Color color;              // RGBA color (vec4)
};
```

**Cube Template:**
```cpp
// Unit cube vertices (centered at origin)
const float s_cubeVertices[8][3] = {
    {-0.5f, -0.5f, -0.5f}, // 0
    { 0.5f, -0.5f, -0.5f}, // 1
    { 0.5f,  0.5f, -0.5f}, // 2
    {-0.5f,  0.5f, -0.5f}, // 3
    {-0.5f, -0.5f,  0.5f}, // 4
    { 0.5f, -0.5f,  0.5f}, // 5
    { 0.5f,  0.5f,  0.5f}, // 6
    {-0.5f,  0.5f,  0.5f}  // 7
};

// Face definitions (counter-clockwise winding)
const uint32_t s_cubeFaces[6][4] = {
    {0, 1, 2, 3}, // Front (-Z)
    {5, 4, 7, 6}, // Back (+Z)
    {4, 0, 3, 7}, // Left (-X)
    {1, 5, 6, 2}, // Right (+X)
    {4, 5, 1, 0}, // Bottom (-Y)
    {3, 2, 6, 7}  // Top (+Y)
};
```

## 3. GPU Buffer Upload

**Location:** `Application.cpp:updateVoxelMesh()`

### Buffer Creation
```cpp
// Generate OpenGL buffers
glGenBuffers(1, &m_voxelVBO);  // Vertex Buffer Object
glGenBuffers(1, &m_voxelEBO);  // Element Buffer Object

// Upload vertex data
glBindBuffer(GL_ARRAY_BUFFER, m_voxelVBO);
glBufferData(GL_ARRAY_BUFFER, 
             sizeof(Rendering::Vertex) * mesh.vertices.size(),
             mesh.vertices.data(), 
             GL_STATIC_DRAW);

// Upload index data
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_voxelEBO);
glBufferData(GL_ELEMENT_ARRAY_BUFFER,
             sizeof(uint32_t) * mesh.indices.size(),
             mesh.indices.data(),
             GL_STATIC_DRAW);
```

### Vertex Attribute Setup
```cpp
// Vertex position (3 floats)
glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 
                      sizeof(Rendering::Vertex), 
                      (void*)offsetof(Rendering::Vertex, position));

// Vertex normal (3 floats)  
glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE,
                      sizeof(Rendering::Vertex),
                      (void*)offsetof(Rendering::Vertex, normal));

// Vertex color (4 floats, but shader expects 3)
glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE,
                      sizeof(Rendering::Vertex),
                      (void*)offsetof(Rendering::Vertex, color));
```

**Note:** VAOs are not available in OpenGL 2.1, so manual vertex setup is used.

## 4. Shader Program

**Location:** `Application.cpp:createShaderProgram()`

### Vertex Shader (GLSL 120)
```glsl
#version 120
attribute vec3 aPos;     // Vertex position
attribute vec3 aNormal;  // Vertex normal
attribute vec3 aColor;   // Vertex color

varying vec3 FragPos;    // World position for fragment shader
varying vec3 Normal;     // Normal for fragment shader  
varying vec3 Color;      // Color for fragment shader

uniform mat4 model;      // Model matrix (identity)
uniform mat4 view;       // View matrix (camera)
uniform mat4 projection; // Projection matrix (perspective)

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(model) * aNormal;
    Color = aColor;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
```

### Fragment Shader (GLSL 120)
```glsl
#version 120
varying vec3 FragPos;
varying vec3 Normal;
varying vec3 Color;

void main() {
    // Output bright red for debugging
    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
```

**Note:** Currently hardcoded to red for debugging. Should use `Color` varying.

## 5. Per-Frame Rendering

**Location:** `Application.cpp:render()`

### Rendering State Setup
```cpp
// Ensure correct framebuffer
glBindFramebuffer(GL_FRAMEBUFFER, 0);

// Clear screen
glClearColor(0.2f, 0.2f, 0.2f, 1.0f);  // Dark gray
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

// Set viewport
glViewport(0, 0, windowWidth, windowHeight);

// Disable depth testing and culling for debugging
glDisable(GL_DEPTH_TEST);
glDisable(GL_CULL_FACE);
```

### Matrix Setup
```cpp
// Get camera matrices
auto camera = m_cameraController->getCamera();
auto viewMatrix = camera->getViewMatrix();
auto projMatrix = camera->getProjectionMatrix();

// Set shader uniforms
Math::Matrix4f identity;  // Model matrix
glUniformMatrix4fv(modelLoc, 1, GL_FALSE, identity.m);
glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewMatrix.m);
glUniformMatrix4fv(projLoc, 1, GL_FALSE, projMatrix.m);
```

### Camera Configuration
- **Position**: Isometric view at (7.11, 7.11, 7.11)
- **Target**: Workspace center at (2.5, 2.5, 2.5)
- **Distance**: 5.0 units
- **Projection**: Perspective with 60° FOV

### Vertex Data Binding
```cpp
// Manual vertex setup (no VAO)
glBindBuffer(GL_ARRAY_BUFFER, m_voxelVBO);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_voxelEBO);

// Enable vertex attributes
glEnableVertexAttribArray(posLoc);
glEnableVertexAttribArray(normalLoc);
glEnableVertexAttribArray(colorLoc);

// Set vertex attribute pointers
// (see section 3 for details)
```

### Draw Command
```cpp
glDrawElements(GL_TRIANGLES, m_voxelIndexCount, GL_UNSIGNED_INT, 0);
```

## 6. Coordinate Transformations

### World → NDC Transformation
```
World Coordinates → View Space → Clip Space → NDC
        |              |           |         |
    Model Matrix   View Matrix  Proj Matrix  w-divide
```

**Example for voxel at (0,0,0) with 128cm resolution:**
- **Grid Position**: (0, 0, 0)
- **World Position**: (0.64, 0.64, 0.64) meters
- **Vertex Range**: 0.032 to 1.248 meters (64cm ± 61.44cm)

### Matrix Calculation
```cpp
Math::Matrix4f mvp = projMatrix * viewMatrix * modelMatrix;
Math::Vector4f clipPos = mvp * Math::Vector4f(worldPos.x, worldPos.y, worldPos.z, 1.0f);
Math::Vector3f ndc = Math::Vector3f(clipPos.x, clipPos.y, clipPos.z) / clipPos.w;
```

## 7. OpenGL State Management

### Required OpenGL Version
- **Target**: OpenGL 2.1 (for compatibility)
- **GLSL**: Version 120
- **Extensions**: GL_APPLE_vertex_array_object (VAOs not used)

### Buffer Management
- **VBO**: Stores vertex data (position, normal, color)
- **EBO**: Stores triangle indices
- **VAO**: Not used (compatibility fallback)

### Error Handling
```cpp
GLenum err = glGetError();
if (err != GL_NO_ERROR) {
    std::cerr << "OpenGL error: " << err << std::endl;
}
```

## 8. Current Issues & Debugging

### Problem Statement
Despite the complete pipeline executing without errors:
- Shader compiles and links successfully
- Vertex data uploads correctly
- glDrawElements executes with proper index count
- **Result**: Only dark gray background visible, no red fragments

### Debug Information Available
- Vertex transformation to NDC coordinates
- Matrix values printed during first 3 frames
- OpenGL state verification
- Error checking at each step

### Potential Issues
1. **Viewport clipping**: Voxels might be outside visible frustum
2. **Matrix errors**: Transformation matrices might be incorrect
3. **Vertex attribute mismatch**: Color attribute expects vec3 but vertex has vec4
4. **Framebuffer issues**: Drawing to wrong buffer despite checks

### Next Debug Steps
1. Test with hardcoded NDC coordinates (-1 to +1 range)
2. Verify pixel format and framebuffer configuration
3. Check if fragments are being generated but discarded
4. Test with simplified transformation (identity matrices)

## File References

### Core Files
- `apps/cli/src/Application.cpp` - Main rendering logic
- `apps/cli/src/VoxelMeshGenerator.cpp` - Mesh generation
- `core/rendering/RenderTypes.h` - Data structures
- `core/voxel_data/SparseOctree.h` - Voxel storage

### Debug Outputs
- Console logs show each pipeline step
- PPM screenshot files for visual verification
- Matrix and vertex data dumps during first frames

## Memory Layout

### Vertex Structure Size
```cpp
sizeof(Rendering::Vertex) = 44 bytes
- position: 12 bytes (3 * float)
- normal:   12 bytes (3 * float)  
- texCoords: 8 bytes (2 * float)
- color:    16 bytes (4 * float)
```

### Buffer Sizes (Single Voxel)
- **Vertices**: 24 vertices × 44 bytes = 1,056 bytes
- **Indices**: 36 indices × 4 bytes = 144 bytes
- **Total GPU Memory**: ~1.2 KB per voxel