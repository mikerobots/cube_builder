# Core Library Design Document
*Updated: June 22, 2025*

## Overview

The Core Library implements the main voxel editing functionality through 10 major subsystems. It builds on the Foundation Layer to provide comprehensive voxel data management, rendering, user interaction, and advanced editing features.

This layer serves as the shared engine for all applications (CLI, Qt Desktop, VR) and contains the complete business logic for voxel manipulation and 3D visualization.

## Architecture Principles

### Modular Design
- 10 independent subsystems with clear responsibilities
- Well-defined interfaces and minimal coupling
- Event-driven communication through Foundation event system

### Performance Focus
- Real-time voxel editing across 10 resolution levels
- VR-optimized for Meta Quest 3 (<4GB memory, 90+ FPS)
- Sparse data structures for memory efficiency
- Header-only critical paths for maximum performance

### Comprehensive Testing
- 895+ unit tests across all subsystems
- Integration tests for cross-component functionality
- Visual validation tests with screenshot analysis
- Performance benchmarks and stress testing

## Core Subsystems

### 1. Voxel Data Management (`core/voxel_data/`)

**Purpose**: Multi-resolution voxel storage with sparse optimization and workspace management

#### Key Components:
- **VoxelDataManager**: Central coordination for all voxel operations
- **SparseOctree**: Memory-efficient hierarchical voxel storage
- **VoxelGrid**: Single-resolution voxel storage layer
- **WorkspaceManager**: Workspace bounds and resizing management
- **VoxelTypes**: Type definitions and utilities for voxel operations

#### Design Features:
- **10 resolution levels**: 1cm, 2cm, 4cm, 8cm, 16cm, 32cm, 64cm, 128cm, 256cm, 512cm
- **Centered coordinate system**: Origin at (0,0,0) with Y-up orientation
- **1cm precision**: All positions aligned to 1cm grid increments
- **Sparse storage**: Memory-efficient octree with on-demand allocation
- **Collision detection**: Prevents overlapping voxels across all resolutions
- **Workspace bounds**: Configurable 2m³ to 8m³ workspace (default 5m³)

#### API Pattern:
```cpp
class VoxelDataManager {
public:
    bool setVoxel(const Vector3i& position, VoxelResolution resolution, bool value);
    bool getVoxel(const Vector3i& position, VoxelResolution resolution) const;
    
    void setActiveResolution(VoxelResolution resolution);
    VoxelResolution getActiveResolution() const;
    
    bool validatePosition(const Vector3i& position) const;
    std::vector<VoxelInfo> getVoxelsInRegion(const BoundingBox& region) const;
    
    void resizeWorkspace(const Vector3f& newSize);
    Vector3f getWorkspaceSize() const;
};
```

#### Critical Performance:
- Sub-100ms resolution switching
- Memory usage <2GB for large projects
- Real-time collision detection
- Efficient spatial queries for rendering

#### Test Coverage:
- **7 unit tests** covering placement, retrieval, collision detection, and performance
- **106/107 tests passing** (one performance test may fail on slower hardware)

### 2. Rendering Engine (`core/rendering/`)

**Purpose**: High-performance OpenGL 3.3 Core Profile rendering with modern pipeline

#### Key Components:
- **RenderEngine**: Main rendering pipeline coordinator
- **OpenGLRenderer**: Low-level OpenGL abstraction and optimization
- **ShaderManager/ShaderManagerSafe**: Shader compilation with thread-safe variant
- **RenderState**: OpenGL state tracking and optimization
- **GroundPlaneGrid**: Reference grid rendering with dynamic scaling
- **MacOSGLLoader**: Platform-specific OpenGL function loading

#### Design Features:
- **OpenGL 3.3 Core Profile**: No legacy fixed-function pipeline
- **GLSL 330 core**: All shaders use modern GLSL syntax
- **12 shader programs**: Specialized shaders for different rendering modes
- **Forward-compatible context**: Future-proof OpenGL context creation
- **60+ FPS target**: Performance optimization for real-time editing
- **Thread safety**: ShaderManagerSafe for multi-threaded environments

#### Shader Pipeline:
- **voxel.vert/frag**: Basic voxel cube rendering
- **highlight.vert/frag**: Face highlighting with yellow tint
- **outline.vert/frag**: Green/red outline previews
- **grid.vert/frag**: Ground plane grid with dynamic opacity
- **preview.vert/frag**: Real-time voxel placement previews

#### API Pattern:
```cpp
class RenderEngine {
public:
    bool initialize(int width, int height);
    void render(const Scene& scene, const Camera& camera);
    
    void setRenderMode(RenderMode mode);
    void resize(int width, int height);
    
    bool isInitialized() const;
    RenderStatistics getStatistics() const;
};

class OpenGLRenderer {
public:
    void renderVoxels(const std::vector<VoxelInstance>& voxels);
    void renderGrid(const GridParameters& params);
    void renderHighlights(const std::vector<FaceHighlight>& highlights);
};
```

#### Test Coverage:
- **20+ comprehensive unit tests** covering all rendering aspects
- **Shader compilation validation** for all 12 shaders
- **Cross-platform compatibility** testing
- **Performance benchmarks** for rendering throughput

### 3. Camera System (`core/camera/`)

**Purpose**: 3D camera management with orbit controls and view presets

#### Key Components:
- **Camera**: Abstract base class with matrix calculations
- **OrbitCamera**: Spherical coordinate orbit implementation
- **CameraController**: Input handling and mouse interaction
- **Viewport**: Screen-space coordinate transformations

#### Design Features:
- **Header-only implementation**: Maximum performance for critical operations
- **Lazy matrix computation**: Matrices calculated only when needed
- **7 view presets**: Front, Back, Left, Right, Top, Bottom, Isometric
- **Smooth transitions**: Animated camera movements
- **Mouse interaction**: Click-and-drag orbit, zoom, and pan controls

#### API Pattern:
```cpp
class OrbitCamera : public Camera {
public:
    void setTarget(const Vector3f& target);
    void setDistance(float distance);
    void setRotation(float azimuth, float elevation);
    
    void rotate(float deltaAzimuth, float deltaElevation);
    void zoom(float deltaDistance);
    
    Matrix4f getViewMatrix() const override;
    Matrix4f getProjectionMatrix() const override;
};

enum class ViewPreset {
    Front, Back, Left, Right, Top, Bottom, Isometric
};
```

#### Test Coverage:
- **108 tests across 7 test suites**: Comprehensive coverage of all functionality
- **Matrix calculation validation**: Precision testing for view/projection matrices
- **View preset accuracy**: Validation of predefined camera positions
- **Performance benchmarks**: Matrix computation performance

### 4. Input System (`core/input/`)

**Purpose**: Unified input handling with precision placement and multi-platform support

#### Key Components:
- **InputManager**: Centralized input coordination
- **MouseHandler/KeyboardHandler/TouchHandler**: Platform-specific input processing
- **VRInputHandler**: VR controller and hand tracking support
- **PlacementValidation**: Pre-placement validation and collision detection
- **PlaneDetector**: Placement plane persistence for consistent interaction

#### Design Features:
- **Ray-casting**: Accurate mouse-to-3D-world coordinate conversion
- **1cm precision snapping**: All placements aligned to grid increments
- **Shift override**: Modifier key to disable automatic snapping
- **Same-size auto-snap**: Automatic snapping to voxels of the same resolution
- **Placement validation**: Real-time collision detection and bounds checking
- **<16ms response time**: Performance target for input processing

#### API Pattern:
```cpp
class InputManager {
public:
    void processMouseInput(const MouseEvent& event);
    void processKeyboardInput(const KeyEvent& event);
    void processVRInput(const VREvent& event);
    
    void registerHandler(InputType type, std::unique_ptr<InputHandler> handler);
    void setPlacementMode(PlacementMode mode);
    
    Vector3i getSnappedPosition(const Vector3f& worldPos, VoxelResolution resolution) const;
};

class PlacementValidation {
public:
    bool validatePlacement(const Vector3i& position, VoxelResolution resolution) const;
    bool checkCollision(const Vector3i& position, VoxelResolution resolution) const;
    bool isWithinBounds(const Vector3i& position) const;
};
```

#### Test Coverage:
- **9 unit tests** covering all input processing aspects
- **Ray-casting accuracy**: Precision validation for mouse picking
- **Placement validation**: Collision detection and bounds checking
- **Performance testing**: Input processing latency measurement

### 5. Visual Feedback System (`core/visual_feedback/`)

**Purpose**: Real-time visual cues, highlighting, and user interaction feedback

#### Key Components:
- **FeedbackRenderer**: Main visual feedback coordinator
- **HighlightManager/HighlightRenderer**: Face highlighting with yellow tint
- **OutlineRenderer**: Green/red outline previews for valid/invalid placements
- **OverlayRenderer**: Ground plane grid and UI overlay rendering
- **PreviewManager**: Real-time placement preview coordination
- **FaceDetector**: Ray-casting for accurate face selection

#### Design Features:
- **Single-face highlighting**: Precise face selection with yellow highlighting
- **Real-time previews**: Green outlines for valid placements, red for invalid
- **Ground plane grid**: 32cm squares with dynamic opacity (35%/65%)
- **<16ms preview updates**: Performance target for responsive feedback
- **1cm precision snapping**: Visual alignment with grid increments
- **Dynamic opacity**: Grid fades during interaction for better visibility

#### API Pattern:
```cpp
class FeedbackRenderer {
public:
    void renderFaceHighlight(const FaceInfo& face);
    void renderVoxelPreview(const Vector3i& position, VoxelResolution resolution, bool isValid);
    void renderGroupOutlines(const std::vector<GroupId>& groups);
    void renderGrid(const GridParameters& params);
    
    void setPreviewMode(PreviewMode mode);
    void updateGridOpacity(float opacity);
};

class FaceDetector {
public:
    std::optional<FaceInfo> detectFace(const Ray& ray, const VoxelGrid& grid) const;
    Vector3f getFaceNormal(const FaceInfo& face) const;
    Vector3f getFaceCenter(const FaceInfo& face) const;
};
```

#### Test Coverage:
- **10 unit tests** covering all visual feedback functionality
- **Face highlighting accuracy**: Single-face selection validation
- **Preview positioning**: 1cm precision snapping verification
- **Grid rendering performance**: 60+ FPS with large workspaces
- **Visual correctness**: Cross-platform rendering consistency

### 6. Selection System (`core/selection/`)

**Purpose**: Efficient voxel selection with multi-selection operations and persistence

#### Key Components:
- **SelectionManager**: Selection state management and operations
- **SelectionSet**: Efficient storage of selected voxels
- **BoxSelector/SphereSelector/FloodFillSelector**: Different selection tools
- **SelectionRenderer**: Visual representation of selections

#### Design Features:
- **Multiple selection modes**: Individual, box, sphere, and flood-fill selection
- **Selection persistence**: Save and restore selection states
- **Bulk operations**: Efficient operations on large selections
- **Performance optimization**: Specialized data structures for large selections
- **Visual feedback**: Real-time selection highlighting

#### API Pattern:
```cpp
class SelectionManager {
public:
    void selectVoxel(const Vector3i& position, VoxelResolution resolution);
    void deselectVoxel(const Vector3i& position, VoxelResolution resolution);
    void selectRegion(const BoundingBox& region);
    void selectAll();
    void clearSelection();
    
    std::vector<VoxelId> getSelection() const;
    bool isSelected(const Vector3i& position, VoxelResolution resolution) const;
    
    void saveSelection(const std::string& name);
    void restoreSelection(const std::string& name);
};
```

#### Test Coverage:
- **7 unit tests** covering all selection functionality
- **Performance benchmarks**: Large selection performance validation
- **Selection persistence**: Save/restore accuracy testing

### 7. Groups System (`core/groups/`)

**Purpose**: Hierarchical voxel grouping with metadata and bulk operations

#### Key Components:
- **GroupManager**: Hierarchical group management
- **VoxelGroup**: Individual group data structure and operations
- **GroupOperations**: Group manipulation algorithms (move, hide, copy)
- **GroupHierarchy**: Tree-based group organization

#### Design Features:
- **Hierarchical groups**: Nested group support with tree structure
- **Group metadata**: Names, descriptions, and custom properties
- **Bulk operations**: Efficient operations on entire groups
- **Visibility control**: Show/hide groups for complex scene management
- **Group persistence**: Save and restore group hierarchies

#### API Pattern:
```cpp
class GroupManager {
public:
    GroupId createGroup(const std::string& name, const std::vector<VoxelId>& voxels);
    void deleteGroup(GroupId id);
    
    void hideGroup(GroupId id);
    void showGroup(GroupId id);
    void moveGroup(GroupId id, const Vector3f& offset);
    
    void addVoxelToGroup(GroupId groupId, const VoxelId& voxelId);
    void removeVoxelFromGroup(GroupId groupId, const VoxelId& voxelId);
    
    std::vector<GroupInfo> listGroups() const;
    GroupId getParentGroup(GroupId id) const;
};
```

#### Test Coverage:
- **6 unit tests** covering all group functionality
- **Hierarchy validation**: Tree structure integrity testing
- **Group operations**: Move, hide, copy operation validation

### 8. Surface Generation (`core/surface_gen/`)

**Purpose**: Advanced mesh generation from voxel data using dual contouring

#### Key Components:
- **SurfaceGenerator**: Main surface mesh generation interface
- **DualContouring**: Advanced isosurface extraction algorithm
- **MeshBuilder**: Triangle mesh construction utilities
- **SurfaceTypes**: Type definitions for surface generation pipeline

#### Design Features:
- **Dual contouring algorithm**: Superior feature preservation compared to marching cubes
- **Multi-resolution LOD**: Adaptive quality based on view distance
- **Real-time preview**: Simplified meshes during editing
- **High-quality export**: Full detail for final mesh output
- **Memory efficiency**: Streaming mesh generation for large voxel sets

#### API Pattern:
```cpp
class SurfaceGenerator {
public:
    Mesh generateSurface(const VoxelGrid& grid, const SurfaceParameters& params);
    Mesh generateLOD(const VoxelGrid& grid, int lodLevel);
    
    void setQualitySettings(const SurfaceQuality& settings);
    SurfaceStatistics getStatistics() const;
    
    bool generatePreview(const VoxelGrid& grid, Mesh& outMesh);
};

class DualContouring {
public:
    void extractSurface(const VoxelGrid& grid, std::vector<Triangle>& triangles);
    void setIsoValue(float isoValue);
    void setFeatureThreshold(float threshold);
};
```

#### Test Coverage:
- **5 unit tests** covering all surface generation functionality
- **Mesh accuracy**: Generated mesh quality validation
- **Performance benchmarks**: Large voxel set processing performance
- **LOD validation**: Level-of-detail quality verification

### 9. File I/O System (`core/file_io/`)

**Purpose**: Project persistence, import/export, and format version management

#### Key Components:
- **FileManager**: Project file management coordination
- **BinaryFormat/BinaryIO**: Efficient binary serialization
- **Compression**: LZ4 compression for storage efficiency
- **STLExporter**: 3D mesh export functionality
- **Project**: Project structure management
- **FileVersioning**: Backward compatibility handling

#### Design Features:
- **Custom binary format**: Optimized for voxel data storage
- **LZ4 compression**: Fast compression with good ratios
- **Version management**: Forward and backward compatibility
- **Complete state preservation**: Entire editing session persistence
- **Multiple export formats**: STL, OBJ, and custom formats

#### API Pattern:
```cpp
class FileManager {
public:
    bool saveProject(const std::string& filename, const Project& project);
    bool loadProject(const std::string& filename, Project& project);
    
    bool exportSTL(const std::string& filename, const Mesh& mesh);
    bool exportOBJ(const std::string& filename, const Mesh& mesh);
    
    FileVersion getFileVersion(const std::string& filename) const;
    bool isCompatible(FileVersion version) const;
};

class Project {
public:
    void addVoxelData(const VoxelDataManager& voxelData);
    void addGroups(const GroupManager& groups);
    void addCameraState(const Camera& camera);
    
    VoxelDataManager getVoxelData() const;
    GroupManager getGroups() const;
    Camera getCameraState() const;
};
```

#### Test Coverage:
- **9 unit tests** covering all file I/O functionality
- **Save/load roundtrip**: Data integrity validation
- **Compression efficiency**: Storage size optimization testing
- **Export accuracy**: Mesh export format validation

### 10. Undo/Redo System (`core/undo_redo/`)

**Purpose**: Comprehensive command history with atomic operations and transactions

#### Key Components:
- **HistoryManager**: Command history management
- **Command**: Abstract command interface with execute/undo/redo
- **CompositeCommand**: Multi-command atomic transactions
- **PlacementCommands/VoxelCommands/SelectionCommands**: Specific command implementations
- **StateSnapshot**: Efficient state preservation for undo operations
- **Transaction**: Atomic operation grouping

#### Design Features:
- **Command pattern**: Comprehensive undo/redo for all operations
- **Atomic transactions**: Group multiple operations into single undo/redo
- **State snapshots**: Efficient delta-based state preservation
- **Memory management**: Configurable history size limits
- **Performance optimization**: Lazy state capture and restoration

#### API Pattern:
```cpp
class HistoryManager {
public:
    void executeCommand(std::unique_ptr<Command> command);
    bool undo();
    bool redo();
    
    void beginTransaction();
    void commitTransaction();
    void rollbackTransaction();
    
    void setMaxHistorySize(size_t size);
    size_t getHistorySize() const;
    bool canUndo() const;
    bool canRedo() const;
};

class Command {
public:
    virtual ~Command() = default;
    virtual bool execute() = 0;
    virtual bool undo() = 0;
    virtual bool redo() = 0;
    virtual std::string getDescription() const = 0;
};
```

#### Test Coverage:
- **5 unit tests** covering all undo/redo functionality
- **Command execution**: Execute/undo/redo validation
- **Transaction handling**: Atomic operation testing
- **Memory management**: History size limit validation

## Cross-Cutting Design Patterns

### Event-Driven Architecture
All subsystems communicate through Foundation event system:
- **Loose coupling**: Components don't directly depend on each other
- **Real-time updates**: Changes propagated immediately to interested parties
- **Performance**: Asynchronous event processing prevents blocking

### Performance Optimization
Critical performance patterns throughout Core Library:
- **Header-only critical paths**: Camera system uses header-only for performance
- **Object pooling**: Memory pools for frequently allocated objects
- **Sparse data structures**: Memory-efficient storage for large voxel sets
- **Lazy computation**: Matrices and complex calculations computed on-demand

### Memory Management
VR-optimized memory usage patterns:
- **<4GB total memory**: Optimized for Meta Quest 3 constraints
- **Sparse storage**: Octree and compressed data structures
- **Memory pressure handling**: Automatic cleanup when memory is low
- **Allocation tracking**: Monitor memory usage across all subsystems

### Coordinate System Consistency
Standardized coordinate system throughout:
- **Centered at origin**: (0,0,0) at workspace center
- **Y-up orientation**: Consistent with industry standards
- **1cm precision**: All operations aligned to grid increments
- **Consistent transformations**: Matrix calculations use same coordinate system

## Testing Strategy

### Comprehensive Coverage
- **895+ unit tests**: Extensive coverage across all subsystems
- **Integration tests**: Cross-component functionality validation
- **Visual tests**: Screenshot-based rendering validation
- **Performance tests**: Benchmarks for critical operations

### Test Categories
- **Unit tests**: Individual component testing with mocking
- **Integration tests**: Multi-component workflow validation
- **Performance tests**: Memory and speed benchmarks
- **Visual tests**: Rendering correctness with automated validation
- **Stress tests**: Large dataset and prolonged operation testing

### Quality Assurance
- **Continuous integration**: Automated testing on multiple platforms
- **Code coverage**: High coverage requirements for critical components
- **Performance monitoring**: Regression detection for performance-critical code
- **Visual validation**: Automated screenshot comparison for rendering changes

## Future Extensibility

### Multi-Platform Support
Core Library designed for cross-platform deployment:
- **Platform abstraction**: Platform-specific code isolated in Foundation
- **Consistent APIs**: Same interface across desktop, mobile, and VR
- **Performance scaling**: Adaptive quality based on platform capabilities

### Advanced Features
Architecture supports future advanced features:
- **Procedural generation**: Plugin system for procedural voxel content
- **Collaboration**: Multi-user editing with conflict resolution
- **Animation**: Keyframe-based voxel animation support
- **Physics simulation**: Integration with physics engines

### Performance Scaling
Designed to scale with hardware improvements:
- **GPU acceleration**: OpenGL compute shader integration potential
- **Multi-threading**: Thread-safe components where beneficial
- **Memory scaling**: Adaptive memory usage based on available resources
- **Level-of-detail**: Automatic quality scaling for performance

The Core Library provides a comprehensive, well-architected foundation for advanced voxel editing across multiple platforms, with particular optimization for VR deployment on Meta Quest 3.