# Simplified Render Validation Guide

This simplified validation guide works with the actual types in the codebase.

## Key Findings from Code Analysis

1. **Voxel Storage**: The `SparseOctree` only stores voxel presence (boolean), not color data
2. **Color is Applied During Rendering**: Colors are set at render time, not stored per voxel
3. **Vertex Structure**: Uses `Rendering::Vertex` struct with position, normal, texCoords, and color

## Quick Validation Steps

### 1. Check Voxel Storage
```cpp
// In your render code, add:
size_t voxelCount = octree->getVoxelCount();
LOG_INFO("Voxel count in octree: " + std::to_string(voxelCount));

// List first few voxels
auto allVoxels = octree->getAllVoxelPositions();
int count = 0;
for (const auto& pos : allVoxels) {
    if (count++ < 5) {
        LOG_INFO("Voxel at: (" + std::to_string(pos.x) + ", " + 
                 std::to_string(pos.y) + ", " + std::to_string(pos.z) + ")");
    }
}
```

### 2. Check Mesh Generation
```cpp
// After generating mesh
LOG_INFO("Generated " + std::to_string(mesh.vertices.size()) + " vertices");
LOG_INFO("Expected: " + std::to_string(voxelCount * 24) + " (24 per voxel)");

// Check first vertex
if (!mesh.vertices.empty()) {
    const auto& v = mesh.vertices[0];
    LOG_INFO("First vertex - pos: (" + 
             std::to_string(v.position.x) + ", " +
             std::to_string(v.position.y) + ", " +
             std::to_string(v.position.z) + ")");
}
```

### 3. Check OpenGL State
```cpp
// Add this before drawing
GLint vao, vbo, program;
glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
glGetIntegerv(GL_CURRENT_PROGRAM, &program);

LOG_INFO("OpenGL state - VAO: " + std::to_string(vao) + 
         ", VBO: " + std::to_string(vbo) + 
         ", Program: " + std::to_string(program));

// Check if depth test is enabled
GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
LOG_INFO("Depth test: " + std::string(depthTest ? "enabled" : "disabled"));
```

### 4. Check Matrix Values
```cpp
// Print matrix values
void debugMatrix(const Math::Matrix4f& m, const std::string& name) {
    LOG_INFO(name + " matrix diagonal: " + 
             std::to_string(m.m[0]) + ", " +
             std::to_string(m.m[5]) + ", " +
             std::to_string(m.m[10]) + ", " +
             std::to_string(m.m[15]));
}

debugMatrix(viewMatrix, "View");
debugMatrix(projMatrix, "Projection");
```

### 5. Simple Pixel Test
```cpp
// After glSwapBuffers, before next clear
GLubyte pixel[4];
glReadPixels(400, 300, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
LOG_INFO("Center pixel: R=" + std::to_string(pixel[0]) + 
         " G=" + std::to_string(pixel[1]) + 
         " B=" + std::to_string(pixel[2]) + 
         " A=" + std::to_string(pixel[3]));
```

## Command Line Quick Test

Add this simple validation command:

```cpp
// In Commands.cpp, add a simple validate command
m_commandProcessor->registerCommand({
    "validate",
    "Quick render validation",
    CommandCategory::SYSTEM,
    {},
    {},
    [this](const CommandContext& ctx) {
        auto* octree = m_voxelManager->getOctree();
        
        std::stringstream ss;
        ss << "=== Quick Validation ===\n";
        ss << "Voxels in octree: " << octree->getVoxelCount() << "\n";
        
        // Check OpenGL
        GLint vao;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
        ss << "Current VAO: " << vao << "\n";
        
        // Check viewport
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        ss << "Viewport: " << viewport[2] << "x" << viewport[3] << "\n";
        
        // Sample a pixel
        GLubyte pixel[4];
        glReadPixels(viewport[2]/2, viewport[3]/2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        ss << "Center pixel: (" << (int)pixel[0] << "," << (int)pixel[1] << "," 
           << (int)pixel[2] << "," << (int)pixel[3] << ")\n";
        
        return CommandResult::Success(ss.str());
    }
});
```

## Most Common Issues and Fixes

### 1. No Voxels Visible
- **Check**: Voxel count > 0
- **Check**: Vertex count = voxel count * 24
- **Fix**: Ensure voxels are added with `place` command

### 2. Black Screen
- **Check**: Clear color is not black
- **Check**: Shader program is valid (ID > 0)
- **Fix**: Check shader compilation errors

### 3. Voxels Not Where Expected
- **Check**: Camera position and target
- **Check**: Voxel world positions
- **Fix**: Reset camera with `camera reset`

### 4. Rendering Artifacts
- **Check**: Depth test enabled
- **Check**: Face culling settings
- **Fix**: Ensure `glEnable(GL_DEPTH_TEST)` is called

## Minimal Test Sequence

```bash
# In the voxel editor CLI:
workspace 5 5 5
resolution 8
place 2 0 2
validate
```

This should show:
- 1 voxel in octree
- 24 vertices generated
- Non-black pixel at center if voxel is visible