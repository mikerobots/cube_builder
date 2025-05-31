# Voxel Editor Architecture

## Overview
The voxel editor is built with a modular architecture centered around a shared core library. Each subsystem is designed to be independently testable and loosely coupled.

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
├─────────────────┬─────────────────┬─────────────────────────┤
│   Command Line  │   Qt Desktop    │      VR Application     │
│   Application   │   Application   │                         │
└─────────────────┴─────────────────┴─────────────────────────┘
                           │
┌─────────────────────────────────────────────────────────────┐
│                   Core Library                              │
├─────────────────────────────────────────────────────────────┤
│  Input System │ Rendering │ Camera │ Visual Feedback │ I/O  │
├─────────────────────────────────────────────────────────────┤
│   Voxel Data  │  Groups   │ Surface │   Selection     │ Undo │
│   Management  │  System   │  Gen    │   System        │ Redo │
└─────────────────────────────────────────────────────────────┘
                           │
┌─────────────────────────────────────────────────────────────┐
│                  Foundation Layer                           │
├─────────────────────────────────────────────────────────────┤
│    Math Utils │ Memory Pool │ Logger │ Event System │ Config │
└─────────────────────────────────────────────────────────────┘
```

## Core Library Subsystems

### 1. Voxel Data Management (`VoxelDataManager`)

**Purpose**: Manages multi-resolution voxel storage and workspace
**Dependencies**: Math Utils, Memory Pool, Event System

#### Components:
- `VoxelGrid` - Individual resolution level storage
- `MultiResolutionGrid` - Manages all 10 resolution levels
- `SparseOctree` - Memory-efficient voxel storage
- `WorkspaceManager` - Handles workspace bounds and resizing

#### Key Classes:
```cpp
class VoxelDataManager {
public:
    bool setVoxel(const Vector3i& pos, VoxelResolution res, bool value);
    bool getVoxel(const Vector3i& pos, VoxelResolution res) const;
    void setActiveResolution(VoxelResolution res);
    void resizeWorkspace(const Vector3f& size);
    
private:
    std::array<std::unique_ptr<VoxelGrid>, 10> m_grids;
    VoxelResolution m_activeResolution;
    Vector3f m_workspaceSize;
};
```

#### Tests:
- Voxel placement and retrieval
- Resolution switching
- Workspace resizing
- Memory usage validation
- Sparse storage efficiency

### 2. Group Management System (`GroupManager`)

**Purpose**: Handles voxel grouping, operations, and metadata
**Dependencies**: Voxel Data Manager, Event System

#### Components:
- `VoxelGroup` - Individual group with metadata
- `GroupHierarchy` - Manages nested groups
- `GroupOperations` - Move, hide, copy operations

#### Key Classes:
```cpp
class GroupManager {
public:
    GroupId createGroup(const std::string& name, const std::vector<VoxelId>& voxels);
    void hideGroup(GroupId id);
    void showGroup(GroupId id);
    void moveGroup(GroupId id, const Vector3f& offset);
    std::vector<GroupInfo> listGroups() const;
    
private:
    std::unordered_map<GroupId, VoxelGroup> m_groups;
    GroupHierarchy m_hierarchy;
};
```

#### Tests:
- Group creation and deletion
- Visibility toggling
- Group movement operations
- Hierarchy validation
- Metadata persistence

### 3. Surface Generation (`SurfaceGenerator`)

**Purpose**: Converts voxel data to smooth 3D meshes
**Dependencies**: Voxel Data Manager, Math Utils

#### Components:
- `DualContouring` - Core algorithm implementation
- `MeshBuilder` - Constructs output meshes
- `LODManager` - Level-of-detail optimization

#### Key Classes:
```cpp
class SurfaceGenerator {
public:
    Mesh generateSurface(const VoxelGrid& grid, float isoValue = 0.5f);
    Mesh generateLOD(const VoxelGrid& grid, int lodLevel);
    void setQualitySettings(const SurfaceQuality& settings);
    
private:
    DualContouring m_algorithm;
    MeshBuilder m_meshBuilder;
    LODManager m_lodManager;
};
```

#### Tests:
- Mesh generation accuracy
- Edge case handling (empty grids, single voxels)
- LOD quality verification
- Performance benchmarks
- Memory usage validation

### 4. Selection System (`SelectionManager`)

**Purpose**: Manages voxel selection and multi-selection operations
**Dependencies**: Voxel Data Manager, Visual Feedback System

#### Components:
- `SelectionSet` - Manages selected voxels
- `SelectionOperations` - Bulk operations on selections
- `SelectionPersistence` - Save/restore selections

#### Key Classes:
```cpp
class SelectionManager {
public:
    void selectVoxel(const Vector3i& pos, VoxelResolution res);
    void deselectVoxel(const Vector3i& pos, VoxelResolution res);
    void selectAll();
    void clearSelection();
    std::vector<VoxelId> getSelection() const;
    
private:
    SelectionSet m_currentSelection;
    std::stack<SelectionSet> m_selectionHistory;
};
```

#### Tests:
- Single and multi-selection
- Selection persistence
- Bulk operations
- Selection validation
- Performance with large selections

### 5. Camera System (`CameraManager`)

**Purpose**: Manages 3D camera movement and view presets
**Dependencies**: Math Utils, Event System

#### Components:
- `OrbitCamera` - Camera with orbit controls
- `ViewPresets` - Predefined view positions
- `CameraAnimator` - Smooth transitions

#### Key Classes:
```cpp
class CameraManager {
public:
    void setView(ViewType view);
    void zoom(float factor);
    void rotate(float deltaX, float deltaY);
    void reset();
    Matrix4f getViewMatrix() const;
    
private:
    OrbitCamera m_camera;
    ViewPresets m_presets;
    CameraAnimator m_animator;
};
```

#### Tests:
- View transitions
- Zoom limits
- Rotation calculations
- Animation smoothness
- Matrix calculations

### 6. Rendering Engine (`RenderEngine`)

**Purpose**: OpenGL abstraction and rendering pipeline
**Dependencies**: Camera System, Visual Feedback System

#### Components:
- `OpenGLRenderer` - Low-level OpenGL wrapper
- `ShaderManager` - Shader compilation and management
- `RenderState` - OpenGL state management
- `FrameBuffer` - Render target management

#### Key Classes:
```cpp
class RenderEngine {
public:
    void initialize();
    void render(const Scene& scene, const Camera& camera);
    void setRenderMode(RenderMode mode);
    void resize(int width, int height);
    
private:
    OpenGLRenderer m_renderer;
    ShaderManager m_shaders;
    RenderState m_state;
};
```

#### Tests:
- Shader compilation
- Render mode switching
- Performance benchmarks
- Memory usage
- Cross-platform compatibility

### 7. Visual Feedback System (`FeedbackRenderer`)

**Purpose**: Renders visual cues and overlays
**Dependencies**: Rendering Engine, Selection System

#### Components:
- `HighlightRenderer` - Face and voxel highlighting
- `OutlineRenderer` - Green outline previews
- `OverlayRenderer` - UI overlays and indicators

#### Key Classes:
```cpp
class FeedbackRenderer {
public:
    void renderFaceHighlight(const Face& face);
    void renderVoxelPreview(const Vector3i& pos, VoxelResolution res);
    void renderGroupOutlines(const std::vector<GroupId>& groups);
    
private:
    HighlightRenderer m_highlighter;
    OutlineRenderer m_outliner;
    OverlayRenderer m_overlay;
};
```

#### Tests:
- Highlight rendering accuracy
- Preview positioning
- Overlay performance
- Visual correctness
- Depth testing

### 8. Input System (`InputManager`)

**Purpose**: Unified input handling across platforms
**Dependencies**: Event System, Camera System, Selection System

#### Components:
- `MouseHandler` - Mouse input processing
- `KeyboardHandler` - Keyboard input processing
- `TouchHandler` - Touch input processing (Qt)
- `VRInputHandler` - VR hand tracking processing
- `InputMapper` - Maps inputs to actions

#### Key Classes:
```cpp
class InputManager {
public:
    void processMouseInput(const MouseEvent& event);
    void processKeyboardInput(const KeyEvent& event);
    void processVRInput(const VREvent& event);
    void registerHandler(InputType type, InputHandler* handler);
    
private:
    std::map<InputType, InputHandler*> m_handlers;
    InputMapper m_mapper;
};
```

#### Tests:
- Input event processing
- Platform-specific handlers
- Input mapping accuracy
- Event priority handling
- Performance under load

### 9. File I/O System (`FileManager`)

**Purpose**: Handles project save/load and export operations
**Dependencies**: Voxel Data Manager, Group Manager

#### Components:
- `BinaryFormat` - Custom binary format handling
- `STLExporter` - STL file export
- `Compression` - LZ4 compression wrapper
- `FileVersioning` - Format version management

#### Key Classes:
```cpp
class FileManager {
public:
    bool saveProject(const std::string& filename, const Project& project);
    bool loadProject(const std::string& filename, Project& project);
    bool exportSTL(const std::string& filename, const Mesh& mesh);
    
private:
    BinaryFormat m_format;
    STLExporter m_stlExporter;
    Compression m_compression;
};
```

#### Tests:
- Save/load roundtrip integrity
- File format versioning
- Compression efficiency
- Export accuracy
- Error handling

### 10. Undo/Redo System (`HistoryManager`)

**Purpose**: Manages operation history and state restoration
**Dependencies**: Event System, Memory Pool

#### Components:
- `Command` - Base command interface
- `CommandHistory` - Command stack management
- `StateSnapshot` - Efficient state capture

#### Key Classes:
```cpp
class HistoryManager {
public:
    void executeCommand(std::unique_ptr<Command> command);
    bool undo();
    bool redo();
    void setMaxHistorySize(size_t size);
    
private:
    std::vector<std::unique_ptr<Command>> m_history;
    size_t m_currentIndex;
    size_t m_maxSize;
};
```

#### Tests:
- Command execution
- Undo/redo functionality
- Memory limits
- State consistency
- Performance with large operations

## Foundation Layer

### 1. Math Utilities (`MathUtils`)
- Vector and matrix operations
- Intersection calculations
- Geometric utilities

### 2. Memory Pool (`MemoryManager`)
- Object pooling for performance
- Memory pressure detection
- Allocation tracking

### 3. Event System (`EventDispatcher`)
- Decoupled component communication
- Event queuing and processing
- Thread-safe event handling

### 4. Configuration (`ConfigManager`)
- Application settings
- Platform-specific configs
- Runtime parameter adjustment

### 5. Logging (`Logger`)
- Multi-level logging
- Performance profiling
- Debug output management

## Application Layer

### Command Line Application
- `CommandProcessor` - Command parsing and execution
- `AutoComplete` - Tab completion system
- `HelpSystem` - Interactive help
- `MouseInterface` - Window mouse handling

### Qt Desktop Application
- Standard Qt widgets and layouts
- Integration with core library
- Advanced UI features

### VR Application
- OpenXR integration
- Hand tracking interface
- VR-optimized rendering

## Testing Strategy

Each subsystem includes:
- **Unit Tests**: Individual component testing
- **Integration Tests**: Cross-component interaction
- **Performance Tests**: Memory and speed benchmarks
- **Visual Tests**: Rendering correctness validation
- **Platform Tests**: Cross-platform compatibility

## Dependencies and Build

### External Dependencies
- OpenGL 3.3+
- Qt6 (desktop app only)
- OpenXR SDK (VR app only)
- Meta Hand Tracking SDK (VR app only)
- LZ4 compression
- Google Test framework

### Internal Dependencies
- Foundation layer has no internal dependencies
- Core library depends only on foundation layer
- Application layer depends on core library

This architecture ensures:
1. **Testability**: Each subsystem can be tested in isolation
2. **Modularity**: Clear separation of concerns
3. **Reusability**: Core library shared across applications
4. **Maintainability**: Well-defined interfaces and dependencies
5. **Performance**: Optimized data structures and algorithms