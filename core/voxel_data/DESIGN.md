# Voxel Data Management Subsystem

## Purpose
Manages multi-resolution voxel storage, workspace bounds, and provides efficient access to voxel data across all 10 resolution levels.

## Key Components

### VoxelDataManager
**Responsibility**: Main interface for voxel operations
- Coordinate voxel placement/removal across resolutions
- Manage active resolution level
- Handle workspace resizing
- Memory pressure monitoring

### MultiResolutionGrid
**Responsibility**: Container for all resolution levels
- Maintains array of 10 VoxelGrid instances
- Handles resolution switching
- Coordinates data between levels

### VoxelGrid
**Responsibility**: Single resolution level storage
- Sparse octree-based storage
- Efficient memory usage
- Fast spatial queries
- Streaming support for large grids

### SparseOctree
**Responsibility**: Memory-efficient voxel storage
- Hierarchical space partitioning
- Automatic memory optimization
- Fast insertion/deletion
- Spatial locality preservation

### WorkspaceManager
**Responsibility**: Workspace bounds and scaling
- Enforce 2m³ - 8m³ limits
- Dynamic resizing validation
- Coordinate system management
- Bounds checking

## Interface Design

```cpp
class VoxelDataManager {
public:
    // Voxel operations
    bool setVoxel(const Vector3i& pos, VoxelResolution res, bool value);
    bool getVoxel(const Vector3i& pos, VoxelResolution res) const;
    bool hasVoxel(const Vector3i& pos, VoxelResolution res) const;
    
    // Resolution management
    void setActiveResolution(VoxelResolution res);
    VoxelResolution getActiveResolution() const;
    
    // Workspace management
    bool resizeWorkspace(const Vector3f& newSize);
    Vector3f getWorkspaceSize() const;
    bool isValidPosition(const Vector3i& pos, VoxelResolution res) const;
    
    // Bulk operations
    void clearAll();
    void clearResolution(VoxelResolution res);
    size_t getVoxelCount(VoxelResolution res) const;
    
    // Memory management
    size_t getMemoryUsage() const;
    void optimizeMemory();
    
private:
    std::array<std::unique_ptr<VoxelGrid>, 10> m_grids;
    VoxelResolution m_activeResolution;
    std::unique_ptr<WorkspaceManager> m_workspace;
    EventDispatcher* m_eventDispatcher;
};
```

## Data Structures

### Voxel Resolution Enum
```cpp
enum class VoxelResolution : uint8_t {
    Size_1cm = 0,   // 1cm voxels
    Size_2cm = 1,   // 2cm voxels
    Size_4cm = 2,   // 4cm voxels
    Size_8cm = 3,   // 8cm voxels
    Size_16cm = 4,  // 16cm voxels
    Size_32cm = 5,  // 32cm voxels
    Size_64cm = 6,  // 64cm voxels
    Size_128cm = 7, // 128cm voxels
    Size_256cm = 8, // 256cm voxels
    Size_512cm = 9  // 512cm voxels
};
```

### Voxel Position
```cpp
struct VoxelPosition {
    Vector3i gridPos;
    VoxelResolution resolution;
    
    // Conversion utilities
    Vector3f toWorldSpace(const Vector3f& workspaceSize) const;
    static VoxelPosition fromWorldSpace(const Vector3f& worldPos, 
                                      VoxelResolution res,
                                      const Vector3f& workspaceSize);
};
```

## Memory Management

### Sparse Storage Strategy
- Empty regions consume no memory
- Octree nodes created on-demand
- Automatic pruning of empty branches
- Memory pooling for frequent allocations

### Memory Pressure Handling
- Monitor total memory usage
- Trigger cleanup when approaching limits
- Prioritize higher resolution data
- Background compression for inactive levels

### Performance Targets
- Voxel set/get: <1μs
- Resolution switching: <100ms
- Memory usage: <2GB total for VR
- Workspace resize: <50ms

## Events

### VoxelChanged Event
```cpp
struct VoxelChangedEvent {
    Vector3i position;
    VoxelResolution resolution;
    bool oldValue;
    bool newValue;
};
```

### ResolutionChanged Event
```cpp
struct ResolutionChangedEvent {
    VoxelResolution oldResolution;
    VoxelResolution newResolution;
};
```

### WorkspaceResized Event
```cpp
struct WorkspaceResizedEvent {
    Vector3f oldSize;
    Vector3f newSize;
};
```

## Testing Strategy

### Unit Tests
- Voxel placement accuracy across all resolutions
- Workspace bounds validation
- Memory usage verification
- Resolution switching correctness
- Coordinate space conversions

### Performance Tests
- Memory usage under various voxel densities
- Set/get operation speed
- Resolution switching performance
- Large workspace handling

### Stress Tests
- Maximum voxel count handling
- Memory pressure scenarios
- Rapid resolution switching
- Concurrent access patterns

## Dependencies
- **Foundation/Math**: Vector3i, Vector3f operations
- **Foundation/Memory**: Memory pool for octree nodes
- **Foundation/Events**: Event dispatching for changes
- **Foundation/Config**: Memory limits and thresholds

## File I/O Integration
- Serialize/deserialize all resolution levels
- Compress sparse data efficiently
- Maintain version compatibility
- Support incremental saves