# Voxel Editor Architecture
*Updated: June 18, 2025*

## Overview
This document provides the technical architecture and implementation details for the voxel editor. For project overview, design decisions, and rationale, see [DESIGN.md](DESIGN.md).

The system is built with a modular architecture centered around a shared core library. Each subsystem is designed to be independently testable and loosely coupled.

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
- **Coordinate System**: Centered at (0,0,0) with Y-up orientation
- **Precision**: 1cm increment positioning with ground plane constraint (Y ≥ 0)
- **Collision Detection**: Prevents overlapping placements across all resolutions

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
- Voxel placement and retrieval with 1cm precision
- Position validation (Y ≥ 0 constraint, workspace bounds)
- Collision detection across multiple resolutions
- Resolution switching performance (<100ms)
- Workspace resizing and coordinate system integrity
- Memory usage validation and sparse storage efficiency
- Performance with 10,000+ voxels

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

**Purpose**: OpenGL 3.3 Core Profile abstraction and high-performance rendering pipeline
**Dependencies**: Camera System, Visual Feedback System

#### Components:
- `OpenGLRenderer` - OpenGL 3.3 Core Profile wrapper with performance optimization
- `ShaderManager` - GLSL 330 shader compilation and management
- `RenderState` - Modern OpenGL state management (no fixed function)
- `FrameBuffer` - Render target management with FBO support
- `GroundPlaneGrid` - Grid mesh generation and rendering
- **OpenGL Version**: 3.3 Core Profile exclusively
  - GLSL version 330 core for all shaders
  - Forward-compatible context on macOS
  - No legacy GL 2.1 or fixed-function pipeline
- **Performance**: 60+ FPS minimum, optimized for 10,000+ voxels
- **Grid Support**: Dynamic grid scaling with workspace bounds

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

**Purpose**: Renders visual cues, overlays, and ground plane grid
**Dependencies**: Rendering Engine, Selection System, Input System

#### Components:
- `HighlightRenderer` - Face highlighting (yellow) with single-face selection
- `OutlineRenderer` - Green/red outline previews with real-time updates
- `OverlayRenderer` - Ground plane grid and UI overlays
- `FaceDetector` - Ray-casting for face selection
- `PreviewManager` - Coordinates preview state and snapping
- **Ground Plane Grid**: 32cm squares, dynamic opacity (35%/65%)
- **Performance**: <16ms preview updates, 60+ FPS grid rendering

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
- Face highlighting accuracy (single face, yellow color)
- Preview positioning with 1cm precision snapping
- Grid rendering performance (60+ FPS with large workspaces)
- Dynamic opacity changes during interaction
- Visual correctness across all camera angles
- Real-time update performance (<16ms)
- Invalid placement visualization (red previews)

### 8. Input System (`InputManager`)

**Purpose**: Unified input handling with precision placement and validation
**Dependencies**: Event System, Camera System, Selection System, Visual Feedback System

#### Components:
- `MouseHandler` - Mouse input processing with ray-casting
- `KeyboardHandler` - Keyboard input processing (Shift key for snap override)
- `TouchHandler` - Touch input processing (Qt)
- `VRInputHandler` - VR hand tracking processing
- `InputMapper` - Maps inputs to actions
- `PlacementValidation` - Pre-placement validation and collision detection
- `PlaneDetector` - Placement plane persistence logic
- **Placement Logic**: Same-size auto-snap, 1cm increment precision
- **Performance**: <16ms response time for input processing

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
- Input event processing with real-time response
- Ray-casting accuracy for face detection
- Position snapping to 1cm increments
- Placement validation (overlap prevention, bounds checking)
- Modifier key handling (Shift for snap override)
- Placement plane persistence logic
- Platform-specific handlers (desktop, Qt, VR)
- Performance under load (<16ms response time)

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

*For complete CLI command reference, see [apps/cli/CLI_GUIDE.md](apps/cli/CLI_GUIDE.md)*

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
- **OpenGL 3.3 Core Profile**: Core rendering API (consolidated standard)
  - GLFW 3.3.8: Window and context creation
  - GLAD: OpenGL function loading
  - GLM 0.9.9.8: OpenGL Mathematics library
- **Qt6**: Desktop application framework (desktop app only)
- **OpenXR SDK**: VR platform abstraction (VR app only)
- **Meta Hand Tracking SDK**: Hand tracking support (VR app only)
- **LZ4**: Fast compression for file I/O
- **Google Test**: Unit testing framework

### Internal Dependencies
- Foundation layer has no internal dependencies
- Core library depends only on foundation layer
- Application layer depends on core library

### Build System
- **CMake 3.16+**: Cross-platform build configuration
- **Ninja**: Recommended build tool for fast compilation
- **Platform-specific**: MSVC (Windows), GCC/Clang (Linux), Clang (macOS)

## Requirements Traceability

Each subsystem has detailed requirements documented in individual `requirements.md` files:

- **core/voxel_data/requirements.md** - 22 requirements for position validation, collision detection, multi-resolution support
- **core/visual_feedback/requirements.md** - 24 requirements for grid rendering, previews, highlighting
- **core/input/requirements.md** - 23 requirements for mouse/keyboard handling, ray-casting, precision snapping
- **core/rendering/requirements.md** - 11 requirements for OpenGL abstraction, shaders, performance optimization
- **core/undo_redo/requirements.md** - 3 requirements for command pattern and history management
- **core/camera/requirements.md** - 3 requirements for view matrices and transformations
- **core/selection/requirements.md** - Selection management and multi-selection operations
- **core/groups/requirements.md** - Group operations, hierarchy, and metadata management
- **core/surface_gen/requirements.md** - Mesh generation, dual contouring, and LOD support
- **core/file_io/requirements.md** - Save/load, export, compression, and versioning

See the main `requirements.md` file for the complete specification with 52 detailed requirements across all subsystems.

## Architecture Benefits

This architecture ensures:
1. **Testability**: Each subsystem can be tested in isolation
2. **Modularity**: Clear separation of concerns
3. **Reusability**: Core library shared across applications
4. **Maintainability**: Well-defined interfaces and dependencies
5. **Performance**: Optimized data structures and algorithms
6. **Requirements Traceability**: Every requirement mapped to specific subsystems
7. **Precision**: 1cm increment positioning with coordinate system integrity