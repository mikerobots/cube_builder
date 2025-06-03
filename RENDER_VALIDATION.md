# Voxel Rendering Pipeline Validation Guide

## Overview
This document provides a systematic approach to validate each stage of the voxel rendering pipeline to identify where rendering failures occur.

## 1. Voxel Data Storage Validation

### What to Validate
- Voxel exists in octree at correct position
- Voxel has valid color data
- Octree node structure is intact

### How to Validate
```cpp
// Add to SparseOctree::setVoxel after insertion
void validateVoxelStorage(const Vector3i& pos, const Voxel& voxel) {
    // Check voxel was stored
    auto* retrievedVoxel = getVoxel(pos);
    if (!retrievedVoxel) {
        LOG_ERROR("Voxel not stored at position ({}, {}, {})", pos.x, pos.y, pos.z);
        return;
    }
    
    // Validate color
    if (retrievedVoxel->color != voxel.color) {
        LOG_ERROR("Color mismatch: stored {:08X} vs expected {:08X}", 
                  retrievedVoxel->color, voxel.color);
    }
    
    // Validate position encoding
    uint32_t morton = mortonEncode3D(pos);
    auto it = m_nodes.find(morton);
    if (it == m_nodes.end()) {
        LOG_ERROR("Morton code {} not found in octree", morton);
    }
    
    LOG_DEBUG("✓ Voxel storage validated at ({}, {}, {}), color: {:08X}", 
              pos.x, pos.y, pos.z, voxel.color);
}
```

### Expected Values
- Non-null voxel pointer after insertion
- Matching color values
- Valid morton code in node map

### Debug Output
```cpp
// Add to setVoxel
LOG_INFO("Storing voxel at ({}, {}, {}) with color {:08X}", 
         position.x, position.y, position.z, voxel.color);
LOG_DEBUG("Morton code: {}, Total voxels: {}", 
          mortonEncode3D(position), getTotalVoxelCount());
```

## 2. Mesh Generation Validation

### What to Validate
- Correct number of vertices generated (24 per voxel)
- Correct vertex positions
- Proper normal vectors
- Valid color attributes

### How to Validate
```cpp
// Add to VoxelMeshGenerator::generateMesh
void validateMeshGeneration(const std::vector<VoxelVertex>& vertices, 
                           const Vector3i& voxelPos, 
                           const Voxel& voxel) {
    // Check vertex count
    if (vertices.size() != 24) {
        LOG_ERROR("Invalid vertex count: {} (expected 24)", vertices.size());
        return;
    }
    
    // Validate each face
    const float voxelSize = 1.0f;
    const Vector3f basePos(voxelPos.x, voxelPos.y, voxelPos.z);
    
    // Check front face vertices (0-3)
    auto validateFace = [&](int startIdx, const Vector3f& expectedNormal, 
                           const std::string& faceName) {
        for (int i = 0; i < 4; i++) {
            const auto& v = vertices[startIdx + i];
            
            // Check normal
            if (v.normal != expectedNormal) {
                LOG_ERROR("{} face vertex {} has wrong normal: ({}, {}, {})", 
                         faceName, i, v.normal.x, v.normal.y, v.normal.z);
            }
            
            // Check color
            if (v.color != voxel.color) {
                LOG_ERROR("{} face vertex {} has wrong color: {:08X}", 
                         faceName, i, v.color);
            }
        }
    };
    
    validateFace(0, Vector3f(0, 0, 1), "Front");
    validateFace(4, Vector3f(0, 0, -1), "Back");
    validateFace(8, Vector3f(0, 1, 0), "Top");
    validateFace(12, Vector3f(0, -1, 0), "Bottom");
    validateFace(16, Vector3f(-1, 0, 0), "Left");
    validateFace(20, Vector3f(1, 0, 0), "Right");
    
    LOG_DEBUG("✓ Mesh generation validated: {} vertices for voxel at ({}, {}, {})",
              vertices.size(), voxelPos.x, voxelPos.y, voxelPos.z);
}
```

### Expected Values
- 24 vertices per voxel (4 per face × 6 faces)
- Vertex positions within voxel bounds
- Normal vectors: ±1 in one axis, 0 in others
- Color matching voxel color

### Debug Output
```cpp
// Print first few vertices
for (size_t i = 0; i < std::min(size_t(4), vertices.size()); i++) {
    const auto& v = vertices[i];
    LOG_DEBUG("Vertex {}: pos({:.2f}, {:.2f}, {:.2f}), normal({:.1f}, {:.1f}, {:.1f}), color:{:08X}",
              i, v.position.x, v.position.y, v.position.z,
              v.normal.x, v.normal.y, v.normal.z, v.color);
}
```

## 3. GPU Buffer Upload Validation

### What to Validate
- VAO/VBO creation success
- Data upload size matches
- OpenGL error state after upload
- Buffer binding state

### How to Validate
```cpp
// Add to uploadMesh or render function
void validateGPUBuffers(GLuint vao, GLuint vbo, size_t vertexCount) {
    // Check VAO
    if (!glIsVertexArray(vao)) {
        LOG_ERROR("Invalid VAO: {}", vao);
        return;
    }
    
    // Check VBO
    if (!glIsBuffer(vbo)) {
        LOG_ERROR("Invalid VBO: {}", vbo);
        return;
    }
    
    // Bind and check buffer size
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GLint bufferSize = 0;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
    
    size_t expectedSize = vertexCount * sizeof(VoxelVertex);
    if (bufferSize != expectedSize) {
        LOG_ERROR("Buffer size mismatch: {} bytes (expected {})", 
                  bufferSize, expectedSize);
    }
    
    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        LOG_ERROR("OpenGL error after buffer upload: 0x{:04X}", error);
    }
    
    // Validate vertex attributes
    glBindVertexArray(vao);
    GLint enabled = 0;
    glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
    if (!enabled) {
        LOG_ERROR("Position attribute (0) not enabled");
    }
    
    LOG_DEBUG("✓ GPU buffers validated: VAO={}, VBO={}, size={} bytes",
              vao, vbo, bufferSize);
}
```

### Expected Values
- Valid VAO/VBO IDs (non-zero)
- Buffer size = vertex count × sizeof(VoxelVertex)
- No OpenGL errors
- Vertex attributes enabled

### Debug Output
```cpp
LOG_INFO("Uploading {} vertices ({} bytes) to GPU", 
         vertexCount, vertexCount * sizeof(VoxelVertex));
LOG_DEBUG("VAO: {}, VBO: {}", vao, vbo);
```

## 4. Shader Compilation/Linking Validation

### What to Validate
- Shader compilation success
- Program linking success
- Uniform locations valid
- Attribute bindings correct

### How to Validate
```cpp
// Add comprehensive shader validation
void validateShaderProgram(GLuint program) {
    // Check program is valid
    if (!glIsProgram(program)) {
        LOG_ERROR("Invalid shader program: {}", program);
        return;
    }
    
    // Check link status
    GLint linkStatus = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(logLength, '\0');
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        LOG_ERROR("Shader link failed: {}", log);
        return;
    }
    
    // Validate uniform locations
    struct UniformInfo {
        const char* name;
        GLint location;
    };
    
    UniformInfo uniforms[] = {
        {"mvpMatrix", glGetUniformLocation(program, "mvpMatrix")},
        {"modelMatrix", glGetUniformLocation(program, "modelMatrix")},
        {"normalMatrix", glGetUniformLocation(program, "normalMatrix")},
        {"lightDirection", glGetUniformLocation(program, "lightDirection")},
        {"viewPos", glGetUniformLocation(program, "viewPos")},
        {"ambientStrength", glGetUniformLocation(program, "ambientStrength")},
        {"specularStrength", glGetUniformLocation(program, "specularStrength")},
        {"shininess", glGetUniformLocation(program, "shininess")}
    };
    
    for (const auto& u : uniforms) {
        if (u.location == -1) {
            LOG_WARN("Uniform '{}' not found or optimized out", u.name);
        } else {
            LOG_DEBUG("Uniform '{}' at location {}", u.name, u.location);
        }
    }
    
    // Validate attributes
    GLint maxAttribs = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);
    LOG_DEBUG("Max vertex attributes: {}", maxAttribs);
    
    // Check attribute locations
    GLint posLoc = glGetAttribLocation(program, "position");
    GLint normalLoc = glGetAttribLocation(program, "normal");
    GLint colorLoc = glGetAttribLocation(program, "color");
    
    LOG_DEBUG("Attribute locations - position: {}, normal: {}, color: {}",
              posLoc, normalLoc, colorLoc);
    
    LOG_DEBUG("✓ Shader program validated: ID={}", program);
}
```

### Expected Values
- Valid program ID
- Link status = GL_TRUE
- Non-negative uniform locations for used uniforms
- Attribute locations: position=0, normal=1, color=2

### Debug Output
```cpp
LOG_INFO("Shader program created: ID={}", program);
LOG_DEBUG("Vertex shader: ID={}, Fragment shader: ID={}", vertexShader, fragmentShader);
```

## 5. Matrix Transformation Validation

### What to Validate
- Matrix values are finite
- Projection matrix has correct perspective
- View matrix represents camera correctly
- MVP matrix combination is correct

### How to Validate
```cpp
// Add matrix validation
void validateMatrices(const Matrix4f& model, const Matrix4f& view, 
                     const Matrix4f& projection) {
    auto isFinite = [](const Matrix4f& m) {
        for (int i = 0; i < 16; i++) {
            if (!std::isfinite(m.data[i])) {
                return false;
            }
        }
        return true;
    };
    
    // Check for NaN/Inf
    if (!isFinite(model)) {
        LOG_ERROR("Model matrix contains non-finite values");
        printMatrix("Model", model);
    }
    if (!isFinite(view)) {
        LOG_ERROR("View matrix contains non-finite values");
        printMatrix("View", view);
    }
    if (!isFinite(projection)) {
        LOG_ERROR("Projection matrix contains non-finite values");
        printMatrix("Projection", projection);
    }
    
    // Validate projection matrix (perspective)
    // Check for reasonable FOV (aspect ratio in m[0], FOV in m[5])
    float aspect = projection.m[5] / projection.m[0];
    if (aspect < 0.1f || aspect > 10.0f) {
        LOG_WARN("Unusual aspect ratio: {}", aspect);
    }
    
    // Check near/far planes
    float near = (2.0f * projection.m[14]) / (2.0f * projection.m[10] - 2.0f);
    float far = ((projection.m[10] - 1.0f) * near) / (projection.m[10] + 1.0f);
    if (near <= 0 || far <= near) {
        LOG_ERROR("Invalid near/far planes: near={}, far={}", near, far);
    }
    
    // Test a point transformation
    Vector4f testPoint(0, 0, -5, 1);  // Point 5 units in front
    Matrix4f mvp = projection * view * model;
    Vector4f transformed = mvp * testPoint;
    
    // After perspective divide
    if (transformed.w != 0) {
        Vector3f ndc(transformed.x / transformed.w,
                     transformed.y / transformed.w,
                     transformed.z / transformed.w);
        LOG_DEBUG("Test point (0,0,-5) -> NDC: ({:.2f}, {:.2f}, {:.2f})",
                  ndc.x, ndc.y, ndc.z);
        
        // Should be within NDC bounds
        if (std::abs(ndc.x) > 1.0f || std::abs(ndc.y) > 1.0f || 
            ndc.z < -1.0f || ndc.z > 1.0f) {
            LOG_WARN("Test point outside NDC bounds");
        }
    }
    
    LOG_DEBUG("✓ Matrices validated");
}

void printMatrix(const std::string& name, const Matrix4f& m) {
    LOG_DEBUG("{} Matrix:", name);
    for (int i = 0; i < 4; i++) {
        LOG_DEBUG("  [{:7.3f} {:7.3f} {:7.3f} {:7.3f}]",
                  m.m[i*4], m.m[i*4+1], m.m[i*4+2], m.m[i*4+3]);
    }
}
```

### Expected Values
- All matrix values finite (no NaN/Inf)
- Projection: reasonable aspect ratio, positive near/far
- View: represents camera position/orientation
- Test points transform to valid NDC coordinates

### Debug Output
```cpp
// Before rendering
LOG_DEBUG("Camera: pos({:.1f}, {:.1f}, {:.1f}), target({:.1f}, {:.1f}, {:.1f})",
          camPos.x, camPos.y, camPos.z, target.x, target.y, target.z);
printMatrix("MVP", mvpMatrix);
```

## 6. OpenGL State Validation

### What to Validate
- Depth testing enabled and configured
- Culling settings correct
- Viewport dimensions
- Clear color set
- Blending state

### How to Validate
```cpp
// Add comprehensive state validation
void validateOpenGLState() {
    struct GLState {
        GLenum cap;
        const char* name;
        GLboolean expected;
    };
    
    GLState states[] = {
        {GL_DEPTH_TEST, "GL_DEPTH_TEST", GL_TRUE},
        {GL_CULL_FACE, "GL_CULL_FACE", GL_TRUE},
        {GL_BLEND, "GL_BLEND", GL_FALSE},
        {GL_SCISSOR_TEST, "GL_SCISSOR_TEST", GL_FALSE},
        {GL_STENCIL_TEST, "GL_STENCIL_TEST", GL_FALSE}
    };
    
    for (const auto& state : states) {
        GLboolean enabled = glIsEnabled(state.cap);
        if (enabled != state.expected) {
            LOG_WARN("{} is {} (expected {})", 
                     state.name, 
                     enabled ? "enabled" : "disabled",
                     state.expected ? "enabled" : "disabled");
        }
    }
    
    // Check depth function
    GLint depthFunc = 0;
    glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
    if (depthFunc != GL_LESS) {
        LOG_WARN("Depth function is 0x{:04X} (expected GL_LESS)", depthFunc);
    }
    
    // Check cull face mode
    GLint cullMode = 0;
    glGetIntegerv(GL_CULL_FACE_MODE, &cullMode);
    if (cullMode != GL_BACK) {
        LOG_WARN("Cull mode is 0x{:04X} (expected GL_BACK)", cullMode);
    }
    
    // Check viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    LOG_DEBUG("Viewport: ({}, {}) {}x{}", 
              viewport[0], viewport[1], viewport[2], viewport[3]);
    
    if (viewport[2] <= 0 || viewport[3] <= 0) {
        LOG_ERROR("Invalid viewport dimensions: {}x{}", viewport[2], viewport[3]);
    }
    
    // Check clear color
    GLfloat clearColor[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
    LOG_DEBUG("Clear color: ({:.2f}, {:.2f}, {:.2f}, {:.2f})",
              clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    
    // Check for errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        LOG_ERROR("OpenGL error state: 0x{:04X}", error);
    }
    
    LOG_DEBUG("✓ OpenGL state validated");
}
```

### Expected Values
- Depth test: enabled with GL_LESS
- Face culling: enabled with GL_BACK
- Viewport: positive dimensions
- No OpenGL errors

### Debug Output
```cpp
// Before rendering
validateOpenGLState();
LOG_DEBUG("Rendering {} vertices", vertexCount);
```

## 7. Draw Call Validation

### What to Validate
- Correct primitive type
- Valid vertex count
- Draw call executes without error
- Pixels actually rendered

### How to Validate
```cpp
// Add draw call validation
void validateDrawCall(GLenum mode, GLsizei count) {
    // Pre-draw state
    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    if (currentProgram == 0) {
        LOG_ERROR("No shader program active for draw call");
        return;
    }
    
    GLint currentVAO = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
    if (currentVAO == 0) {
        LOG_ERROR("No VAO bound for draw call");
        return;
    }
    
    // Clear errors before draw
    while (glGetError() != GL_NO_ERROR) {}
    
    // Record pixels before draw (small sample)
    GLubyte pixelsBefore[4];
    glReadPixels(400, 300, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixelsBefore);
    
    // Execute draw call
    LOG_DEBUG("Drawing {} vertices with mode 0x{:04X}", count, mode);
    glDrawArrays(mode, 0, count);
    
    // Check for errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        LOG_ERROR("Draw call error: 0x{:04X}", error);
        return;
    }
    
    // Check if pixels changed
    GLubyte pixelsAfter[4];
    glReadPixels(400, 300, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixelsAfter);
    
    bool pixelsChanged = memcmp(pixelsBefore, pixelsAfter, 4) != 0;
    if (!pixelsChanged && count > 0) {
        LOG_WARN("Draw call executed but no pixels changed");
        LOG_DEBUG("Pixel before: ({}, {}, {}, {})", 
                  pixelsBefore[0], pixelsBefore[1], pixelsBefore[2], pixelsBefore[3]);
        LOG_DEBUG("Pixel after: ({}, {}, {}, {})", 
                  pixelsAfter[0], pixelsAfter[1], pixelsAfter[2], pixelsAfter[3]);
    }
    
    // Use a query to check if any samples passed
    GLuint query;
    glGenQueries(1, &query);
    glBeginQuery(GL_SAMPLES_PASSED, query);
    glDrawArrays(mode, 0, count);
    glEndQuery(GL_SAMPLES_PASSED);
    
    GLint samplesPassed = 0;
    glGetQueryObjectiv(query, GL_QUERY_RESULT, &samplesPassed);
    glDeleteQueries(1, &query);
    
    LOG_DEBUG("Samples passed depth test: {}", samplesPassed);
    
    LOG_DEBUG("✓ Draw call validated: {} vertices drawn", count);
}
```

### Expected Values
- Valid shader program bound
- Valid VAO bound
- No OpenGL errors after draw
- Some pixels changed (for visible geometry)
- Some samples pass depth test

### Debug Output
```cpp
// Around draw call
LOG_DEBUG("Executing draw call: mode=GL_TRIANGLES, count={}", vertexCount);
validateDrawCall(GL_TRIANGLES, vertexCount);
```

## 8. Fragment Output Validation

### What to Validate
- Framebuffer completeness
- Correct render target
- Output color values
- Depth buffer written

### How to Validate
```cpp
// Add fragment output validation
void validateFragmentOutput() {
    // Check framebuffer status
    GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fbStatus != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Framebuffer incomplete: 0x{:04X}", fbStatus);
        return;
    }
    
    // Sample multiple points
    struct SamplePoint {
        int x, y;
        const char* description;
    };
    
    SamplePoint points[] = {
        {400, 300, "center"},
        {100, 100, "top-left"},
        {700, 500, "bottom-right"},
        {400, 100, "top-center"},
        {400, 500, "bottom-center"}
    };
    
    for (const auto& pt : points) {
        GLubyte pixel[4];
        glReadPixels(pt.x, pt.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        
        GLfloat depth;
        glReadPixels(pt.x, pt.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
        
        LOG_DEBUG("Pixel at {} ({}, {}): RGBA({}, {}, {}, {}), depth={:.3f}",
                  pt.description, pt.x, pt.y,
                  pixel[0], pixel[1], pixel[2], pixel[3], depth);
        
        // Check if this looks like our clear color or rendered content
        bool isClearColor = (pixel[0] == 51 && pixel[1] == 51 && 
                            pixel[2] == 51 && pixel[3] == 255);
        bool isLikelyVoxel = (pixel[0] > 0 || pixel[1] > 0 || pixel[2] > 0) && 
                            !isClearColor;
        
        if (isLikelyVoxel) {
            LOG_INFO("Found likely voxel pixel at {}: RGB({}, {}, {})",
                    pt.description, pixel[0], pixel[1], pixel[2]);
        }
    }
    
    // Check depth range
    GLfloat depthRange[2];
    glGetFloatv(GL_DEPTH_RANGE, depthRange);
    LOG_DEBUG("Depth range: [{:.3f}, {:.3f}]", depthRange[0], depthRange[1]);
    
    LOG_DEBUG("✓ Fragment output validated");
}
```

### Expected Values
- Framebuffer complete
- Some pixels not clear color
- Depth values between 0 and 1
- Alpha = 255 (opaque)

### Debug Output
```cpp
// After swap buffers, before clear
validateFragmentOutput();
```

## Complete Validation Function

```cpp
void performCompleteValidation(Application* app) {
    LOG_INFO("=== Beginning Complete Render Pipeline Validation ===");
    
    // 1. Validate voxel storage
    auto* octree = app->getVoxelDataManager()->getOctree();
    Vector3i testPos(2, 0, 2);
    Voxel testVoxel{0xFF0000FF}; // Red voxel
    octree->setVoxel(testPos, testVoxel);
    validateVoxelStorage(testPos, testVoxel);
    
    // 2. Validate mesh generation
    VoxelMeshGenerator generator;
    auto mesh = generator.generateMesh(octree);
    LOG_INFO("Generated mesh with {} vertices", mesh.vertices.size());
    if (!mesh.vertices.empty()) {
        validateMeshGeneration(mesh.vertices, testPos, testVoxel);
    }
    
    // 3. Validate GPU buffers
    GLuint vao = app->getRenderWindow()->getCurrentVAO();
    GLuint vbo = app->getRenderWindow()->getCurrentVBO();
    validateGPUBuffers(vao, vbo, mesh.vertices.size());
    
    // 4. Validate shader program
    GLuint program = app->getRenderWindow()->getShaderProgram();
    validateShaderProgram(program);
    
    // 5. Validate matrices
    auto* camera = app->getRenderWindow()->getCamera();
    Matrix4f model = Matrix4f::identity();
    Matrix4f view = camera->getViewMatrix();
    Matrix4f proj = camera->getProjectionMatrix();
    validateMatrices(model, view, proj);
    
    // 6. Validate OpenGL state
    validateOpenGLState();
    
    // 7. Validate draw call
    if (!mesh.vertices.empty()) {
        validateDrawCall(GL_TRIANGLES, mesh.vertices.size());
    }
    
    // 8. Validate fragment output
    validateFragmentOutput();
    
    LOG_INFO("=== Render Pipeline Validation Complete ===");
}
```

## Usage

Add this validation at key points:

1. **During initialization:**
   ```cpp
   // In Application::initialize()
   validateShaderProgram(m_renderWindow->getShaderProgram());
   validateOpenGLState();
   ```

2. **After adding voxels:**
   ```cpp
   // In command processing
   octree->setVoxel(position, voxel);
   validateVoxelStorage(position, voxel);
   ```

3. **Before rendering:**
   ```cpp
   // In RenderWindow::render()
   if (m_debugValidation) {
       performCompleteValidation(this);
   }
   ```

4. **On render issues:**
   ```cpp
   // Add command to trigger validation
   if (command == "validate") {
       performCompleteValidation(app);
   }
   ```

## Interpreting Results

1. **Storage Validation Fails**: Octree implementation issue
2. **Mesh Generation Fails**: Vertex generation logic error
3. **GPU Buffer Fails**: OpenGL context or upload issue
4. **Shader Validation Fails**: Shader compilation/linking problem
5. **Matrix Validation Fails**: Camera or projection setup issue
6. **State Validation Fails**: OpenGL state management problem
7. **Draw Call Fails**: Rendering pipeline configuration error
8. **Fragment Output Fails**: Shader output or framebuffer issue

This systematic validation will pinpoint exactly where the rendering pipeline fails.