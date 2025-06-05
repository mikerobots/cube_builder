# Rendering Engine Subsystem

## Purpose
Provides OpenGL abstraction layer, manages rendering pipeline, handles multiple render modes, and coordinates with all visual systems.

## Current Implementation Status
The implementation matches the design with core components implemented:
- **RenderEngine** - Main rendering interface implemented
- **OpenGLRenderer** - Low-level OpenGL wrapper implemented
- **ShaderManager** - Shader management implemented (with ShaderManagerSafe variant)
- **RenderState** - OpenGL state tracking implemented
- **RenderStats** - Performance statistics implemented
- **RenderConfig** - Configuration structure implemented
- **RenderTypes** - Type definitions and enums implemented
- **RenderContext** - Additional context management (not in original design)
- **MacOSGLLoader** - Platform-specific OpenGL loading (not in original design)

Components described but not as separate headers (likely integrated):
- **Material** - Defined in RenderTypes.h
- **FrameBuffer** - Not yet implemented (TODO comments in code)
- **VoxelRenderer** - Functionality in RenderEngine
- **InstancedRenderer** - Basic instanced rendering in RenderEngine
- **GPUProfiler** - Not implemented

## Key Components

### RenderEngine
**Responsibility**: Main rendering interface and pipeline coordination
- Initialize and manage OpenGL context
- Coordinate rendering passes
- Manage render targets and framebuffers
- Handle render mode switching

### OpenGLRenderer
**Responsibility**: Low-level OpenGL wrapper and abstraction
- OpenGL state management
- Resource creation and binding
- Command buffer optimization
- Cross-platform OpenGL compatibility

### ShaderManager
**Responsibility**: Shader compilation, management, and caching
- Shader program compilation and linking
- Uniform management and binding
- Shader hot-reloading for development
- Platform-specific shader variants

### RenderState
**Responsibility**: OpenGL state tracking and optimization
- State change minimization
- Render state caching
- Batch similar operations
- Performance profiling

## Interface Design

```cpp
class RenderEngine {
public:
    // Initialization
    bool initialize(const RenderConfig& config);
    void shutdown();
    bool isInitialized() const;
    
    // Frame rendering
    void beginFrame();
    void endFrame();
    void present();
    
    // Scene rendering
    void renderScene(const Scene& scene, const Camera& camera);
    void renderMesh(const Mesh& mesh, const Transform& transform, const Material& material);
    void renderVoxels(const VoxelGrid& grid, VoxelResolution resolution, const RenderSettings& settings);
    
    // Render modes
    void setRenderMode(RenderMode mode);
    RenderMode getRenderMode() const;
    void setWireframeEnabled(bool enabled);
    void setSolidEnabled(bool enabled);
    
    // Viewport and targets
    void setViewport(int x, int y, int width, int height);
    void setRenderTarget(RenderTarget* target);
    void setDefaultRenderTarget();
    
    // Debug and profiling
    void setDebugMode(bool enabled);
    RenderStats getRenderStats() const;
    void captureFrame(const std::string& filename);
    
    // Resource management
    uint32_t createVertexBuffer(const void* data, size_t size);
    uint32_t createIndexBuffer(const uint32_t* indices, size_t count);
    uint32_t createTexture(int width, int height, TextureFormat format, const void* data = nullptr);
    void deleteBuffer(uint32_t bufferId);
    void deleteTexture(uint32_t textureId);
    
private:
    std::unique_ptr<OpenGLRenderer> m_glRenderer;
    std::unique_ptr<ShaderManager> m_shaderManager;
    std::unique_ptr<RenderState> m_renderState;
    std::unique_ptr<FrameBuffer> m_frameBuffer;
    RenderConfig m_config;
    RenderStats m_stats;
    EventDispatcher* m_eventDispatcher;
};
```

## Data Structures

### RenderMode
```cpp
enum class RenderMode {
    Solid,              // Solid surfaces only
    Wireframe,          // Wireframe only
    Combined,           // Solid + wireframe overlay
    Points              // Point cloud rendering
};
```

### RenderConfig
```cpp
struct RenderConfig {
    int windowWidth = 1920;
    int windowHeight = 1080;
    bool fullscreen = false;
    int samples = 4;                    // MSAA samples
    bool vsync = true;
    bool debugContext = false;
    ColorFormat colorFormat = ColorFormat::RGBA8;
    DepthFormat depthFormat = DepthFormat::Depth24Stencil8;
    
    // Performance settings
    bool frustumCulling = true;
    bool occlusionCulling = false;
    int maxLights = 8;
    bool shadowMapping = false;
    
    static RenderConfig Default();
    static RenderConfig HighQuality();
    static RenderConfig Performance();
    static RenderConfig VR();
};
```

### Material
```cpp
struct Material {
    Color albedo = Color::White();
    float metallic = 0.0f;
    float roughness = 0.5f;
    float emission = 0.0f;
    uint32_t albedoTexture = 0;
    uint32_t normalTexture = 0;
    uint32_t metallicRoughnessTexture = 0;
    ShaderId shader = 0;
    
    // Rendering properties
    bool doubleSided = false;
    BlendMode blendMode = BlendMode::Opaque;
    CullMode cullMode = CullMode::Back;
    
    void bind(RenderEngine& engine) const;
};
```

### RenderStats
```cpp
struct RenderStats {
    uint32_t frameCount = 0;
    float frameTime = 0.0f;
    float fps = 0.0f;
    
    // Geometry stats
    uint32_t trianglesRendered = 0;
    uint32_t verticesProcessed = 0;
    uint32_t drawCalls = 0;
    
    // Memory stats
    size_t vertexBufferMemory = 0;
    size_t indexBufferMemory = 0;
    size_t textureMemory = 0;
    
    // Performance stats
    float cpuTime = 0.0f;
    float gpuTime = 0.0f;
    uint32_t stateChanges = 0;
    uint32_t shaderSwitches = 0;
    
    void reset();
    void update(float deltaTime);
};
```

## OpenGL Abstraction

### OpenGLRenderer
```cpp
class OpenGLRenderer {
public:
    // Context management
    bool initializeContext(const RenderConfig& config);
    void destroyContext();
    bool isContextValid() const;
    
    // Buffer operations
    uint32_t createVertexBuffer(const void* data, size_t size, BufferUsage usage = BufferUsage::Static);
    uint32_t createIndexBuffer(const uint32_t* indices, size_t count, BufferUsage usage = BufferUsage::Static);
    void updateBuffer(uint32_t bufferId, const void* data, size_t size, size_t offset = 0);
    void bindVertexBuffer(uint32_t bufferId);
    void bindIndexBuffer(uint32_t bufferId);
    
    // Texture operations
    uint32_t createTexture2D(int width, int height, TextureFormat format, const void* data = nullptr);
    uint32_t createTextureCube(int size, TextureFormat format, const void* data[6] = nullptr);
    void updateTexture(uint32_t textureId, int x, int y, int width, int height, const void* data);
    void bindTexture(uint32_t textureId, int slot = 0);
    
    // Shader operations
    uint32_t createShader(ShaderType type, const std::string& source);
    uint32_t createProgram(const std::vector<uint32_t>& shaders);
    void useProgram(uint32_t programId);
    void setUniform(uint32_t programId, const std::string& name, const UniformValue& value);
    
    // Drawing operations
    void drawArrays(PrimitiveType type, int first, int count);
    void drawElements(PrimitiveType type, int count, IndexType indexType = IndexType::UInt32);
    void drawInstanced(PrimitiveType type, int count, int instanceCount);
    
    // State management
    void setDepthTest(bool enabled);
    void setDepthWrite(bool enabled);
    void setBlending(bool enabled, BlendMode mode = BlendMode::Alpha);
    void setCulling(bool enabled, CullMode mode = CullMode::Back);
    void setViewport(int x, int y, int width, int height);
    void clear(ClearFlags flags, const Color& color = Color::Black(), float depth = 1.0f);
    
private:
    RenderState* m_state;
    uint32_t m_contextHandle;
    std::unordered_map<uint32_t, BufferInfo> m_buffers;
    std::unordered_map<uint32_t, TextureInfo> m_textures;
    std::unordered_map<uint32_t, ShaderInfo> m_shaders;
};
```

### ShaderManager
```cpp
class ShaderManager {
public:
    // Shader compilation
    ShaderId compileShader(const std::string& name, const ShaderSource& source);
    ShaderId loadShaderFromFile(const std::string& path);
    bool reloadShader(ShaderId id);
    
    // Shader access
    ShaderProgram* getShader(ShaderId id);
    ShaderId findShader(const std::string& name);
    std::vector<std::string> getShaderNames() const;
    
    // Built-in shaders
    ShaderId getVoxelShader();
    ShaderId getWireframeShader();
    ShaderId getSelectionShader();
    ShaderId getGroupOutlineShader();
    
    // Uniform management
    void setGlobalUniform(const std::string& name, const UniformValue& value);
    void updatePerFrameUniforms(const Camera& camera, const RenderStats& stats);
    
    // Hot reloading
    void enableHotReload(bool enabled);
    void checkForChanges();
    
private:
    struct ShaderEntry {
        ShaderId id;
        std::string name;
        std::string path;
        std::unique_ptr<ShaderProgram> program;
        std::time_t lastModified;
    };
    
    std::unordered_map<ShaderId, ShaderEntry> m_shaders;
    std::unordered_map<std::string, UniformValue> m_globalUniforms;
    ShaderId m_nextShaderId;
    bool m_hotReloadEnabled;
    OpenGLRenderer* m_glRenderer;
    
    bool compileShaderProgram(ShaderEntry& entry, const ShaderSource& source);
    std::string preprocessShader(const std::string& source, const std::vector<std::string>& defines);
};
```

## Rendering Pipeline

### Frame Rendering
```cpp
void RenderEngine::renderScene(const Scene& scene, const Camera& camera) {
    // Update per-frame uniforms
    m_shaderManager->updatePerFrameUniforms(camera, m_stats);
    
    // Frustum culling
    auto visibleObjects = scene.cull(camera.getFrustum());
    
    // Sort objects by material/shader to minimize state changes
    std::sort(visibleObjects.begin(), visibleObjects.end(), MaterialSorter());
    
    // Render opaque objects
    for (const auto& object : visibleObjects) {
        if (object.material.blendMode == BlendMode::Opaque) {
            renderObject(object, camera);
        }
    }
    
    // Render transparent objects (back to front)
    auto transparentObjects = filterTransparent(visibleObjects);
    std::sort(transparentObjects.begin(), transparentObjects.end(), DepthSorter(camera.getPosition()));
    
    for (const auto& object : transparentObjects) {
        renderObject(object, camera);
    }
    
    // Render debug overlays
    if (m_config.debugMode) {
        renderDebugOverlays(scene, camera);
    }
}
```

### Voxel Rendering
```cpp
void VoxelRenderer::renderVoxels(const VoxelGrid& grid, VoxelResolution resolution, const RenderSettings& settings) {
    // Generate or retrieve cached mesh
    Mesh mesh = m_surfaceGenerator->generateSurface(grid, settings.surfaceSettings);
    
    // Setup material based on render mode
    Material material = createVoxelMaterial(settings.renderMode, resolution);
    
    // Render based on mode
    switch (settings.renderMode) {
        case RenderMode::Solid:
            renderSolidMesh(mesh, material);
            break;
            
        case RenderMode::Wireframe:
            renderWireframeMesh(mesh, material);
            break;
            
        case RenderMode::Combined:
            renderSolidMesh(mesh, material);
            material.wireframe = true;
            material.color.a = 0.3f;
            renderWireframeMesh(mesh, material);
            break;
            
        case RenderMode::Points:
            renderPointCloud(grid, material);
            break;
    }
}
```

## Performance Optimization

### State Management
```cpp
class RenderState {
public:
    void setDepthTest(bool enabled);
    void setBlending(bool enabled, BlendMode mode);
    void bindShader(ShaderId id);
    void bindTexture(uint32_t textureId, int slot);
    
    // Batch state changes
    void flush();
    void reset();
    
    // Statistics
    uint32_t getStateChanges() const { return m_stateChanges; }
    
private:
    struct CurrentState {
        bool depthTest = true;
        bool blending = false;
        BlendMode blendMode = BlendMode::Opaque;
        ShaderId boundShader = 0;
        std::array<uint32_t, 16> boundTextures = {};
    } m_current, m_pending;
    
    uint32_t m_stateChanges = 0;
    bool m_stateDirty = false;
    
    void applyStateChanges();
};
```

### Instanced Rendering
```cpp
class InstancedRenderer {
public:
    void addInstance(const Mesh& mesh, const Transform& transform, const Material& material);
    void renderInstances();
    void clear();
    
private:
    struct InstanceBatch {
        Mesh mesh;
        Material material;
        std::vector<Transform> transforms;
        uint32_t instanceBuffer;
    };
    
    std::vector<InstanceBatch> m_batches;
    
    void updateInstanceBuffer(InstanceBatch& batch);
};
```

## Debug and Profiling

### Debug Rendering
- Wireframe overlays
- Normal visualization
- Bounding box rendering
- Light visualization
- Performance metrics overlay

### GPU Profiling
```cpp
class GPUProfiler {
public:
    void beginSection(const std::string& name);
    void endSection();
    
    struct ProfileData {
        std::string name;
        float gpuTime;
        float cpuTime;
    };
    
    std::vector<ProfileData> getResults() const;
    
private:
    struct ProfileSection {
        std::string name;
        uint32_t startQuery;
        uint32_t endQuery;
        std::chrono::high_resolution_clock::time_point cpuStart;
    };
    
    std::stack<ProfileSection> m_sectionStack;
    std::vector<ProfileData> m_results;
};
```

## Testing Strategy

### Unit Tests
- Shader compilation
- State management
- Resource creation/deletion
- Matrix calculations
- Performance metrics

### Integration Tests
- Rendering pipeline
- Multi-platform compatibility
- Memory management
- Performance benchmarks

### Visual Tests
- Render mode correctness
- Visual artifact detection
- Cross-platform consistency
- Performance profiling

## Dependencies
- **Core/Camera**: View and projection matrices
- **Core/SurfaceGen**: Mesh generation for voxels
- **Core/VisualFeedback**: Overlay rendering
- **Foundation/Math**: Matrix and vector operations
- **Foundation/Events**: Rendering events

## Platform Considerations

### Desktop OpenGL
- OpenGL 3.3+ core profile
- Extension loading and validation
- Multi-context support
- High-DPI display support

### Mobile OpenGL ES
- OpenGL ES 3.0+ support
- Reduced precision handling
- Power management optimization
- Limited texture formats

### VR Rendering
- Stereo rendering support
- Foveated rendering optimization
- Low-latency pipeline
- Temporal upsampling

## Known Issues and Technical Debt

### Issue 1: Missing FrameBuffer Implementation
- **Severity**: High
- **Impact**: No off-screen rendering support, limiting post-processing and render targets
- **Proposed Solution**: Implement FrameBuffer class as designed for render-to-texture support
- **Dependencies**: None

### Issue 2: Incomplete Resource Management
- **Severity**: Medium
- **Impact**: Mix of BufferId, VertexBufferId, IndexBufferId types causing confusion
- **Proposed Solution**: Unify resource handle types or clearly separate them
- **Dependencies**: API redesign decision

### Issue 3: No GPU Profiling
- **Severity**: Medium
- **Impact**: Cannot measure GPU performance, making optimization difficult
- **Proposed Solution**: Implement GPUProfiler class with OpenGL timer queries
- **Dependencies**: OpenGL extension support

### Issue 4: Limited Instanced Rendering
- **Severity**: Medium
- **Impact**: renderMeshInstanced exists but no proper InstancedRenderer for batching
- **Proposed Solution**: Implement full InstancedRenderer with automatic batching
- **Dependencies**: None

### Issue 5: Platform-Specific Code Mixing
- **Severity**: Low
- **Impact**: MacOSGLLoader in core rendering instead of platform layer
- **Proposed Solution**: Move platform-specific code to separate platform layer
- **Dependencies**: Platform abstraction layer

### Issue 6: Thread Safety Unclear
- **Severity**: High
- **Impact**: ShaderManagerSafe suggests threading concerns but unclear thread model
- **Proposed Solution**: Document and enforce thread safety requirements
- **Dependencies**: Application threading model

### Issue 7: Shader Hot-Reloading Not Implemented
- **Severity**: Low
- **Impact**: Must restart to see shader changes, slowing development
- **Proposed Solution**: Implement file watching and shader recompilation
- **Dependencies**: File system monitoring

### Issue 8: Material System Too Simple
- **Severity**: Medium
- **Impact**: Material struct doesn't support PBR or complex shading models
- **Proposed Solution**: Expand Material system for modern rendering techniques
- **Dependencies**: Shader system enhancements

### Issue 9: No Occlusion Culling
- **Severity**: Medium
- **Impact**: Rendering performance issues with complex scenes
- **Proposed Solution**: Implement hierarchical occlusion culling
- **Dependencies**: Scene graph implementation

### Issue 10: VR Support Missing
- **Severity**: High (for VR target)
- **Impact**: No stereo rendering or VR-specific optimizations
- **Proposed Solution**: Implement VR rendering pipeline with OpenXR
- **Dependencies**: OpenXR integration