# Voxel Math Library Design Document

## Overview

This document specifies a comprehensive math library for voxel calculations to eliminate the repeated and error-prone calculations scattered throughout the codebase. The library will provide consistent, well-tested, and performant operations for all voxel-related mathematics.

## Key Problems to Solve

1. **Repeated Calculations**: The same voxel positioning calculations appear in multiple files with slight variations
2. **Coordinate System Confusion**: Mixing of world coordinates (meters) and increment coordinates (1cm integer units)
3. **Bottom-Center Positioning**: Voxels are positioned at bottom-center, requiring careful offset calculations
4. **Bounding Box Calculations**: Repeated manual calculations of voxel bounding boxes from bottom-center positions
5. **Face Direction Handling**: Verbose and error-prone face direction to offset conversions
6. **Grid Snapping**: Multiple implementations of snapping logic with subtle differences
7. **Collision Detection**: Scattered implementations of voxel overlap checking
8. **Performance**: Many calculations could be vectorized for better performance

## Core Components

### 1. VoxelBounds Class

Encapsulates all bounding box calculations for voxels positioned at bottom-center.

```cpp
namespace VoxelEditor {
namespace Math {

class VoxelBounds {
public:
    // Create bounds from bottom-center position and size
    // voxelSize is in meters (e.g., 0.32f for 32cm voxel)
    VoxelBounds(const WorldCoordinates& bottomCenter, float voxelSizeMeters);
    VoxelBounds(const IncrementCoordinates& bottomCenter, float voxelSizeMeters);
    
    // Accessors (all return world coordinates)
    WorldCoordinates min() const { return WorldCoordinates(m_min); }
    WorldCoordinates max() const { return WorldCoordinates(m_max); }
    WorldCoordinates center() const { return WorldCoordinates(m_center); }
    WorldCoordinates bottomCenter() const { return WorldCoordinates(m_bottomCenter); }
    float size() const { return m_size; }  // Size in meters
    
    // Utility methods
    BoundingBox toBoundingBox() const;
    bool contains(const WorldCoordinates& point) const;
    bool intersects(const VoxelBounds& other) const;
    bool intersectsRay(const Ray& ray, float& tMin, float& tMax) const;  // tMin/tMax in meters
    
    // Face calculations
    WorldCoordinates getFaceCenter(FaceDirection face) const;
    Vector3f getFaceNormal(FaceDirection face) const;  // Returns normalized direction vector
    Plane getFacePlane(FaceDirection face) const;
    
private:
    Vector3f m_min;  // Internal storage in meters
    Vector3f m_max;  // Internal storage in meters
    Vector3f m_center;  // Internal storage in meters
    Vector3f m_bottomCenter;  // Internal storage in meters
    float m_size;  // Size in meters
};

} // namespace Math
} // namespace VoxelEditor
```

### 2. VoxelGrid Class

Handles all grid-related calculations and snapping operations.

```cpp
class VoxelGrid {
public:
    // Constants
    static constexpr float CM_TO_METERS = 0.01f;
    static constexpr float METERS_TO_CM = 100.0f;
    static constexpr int MIN_INCREMENT_CM = 1;
    
    // Grid snapping operations
    static IncrementCoordinates snapToIncrementGrid(const WorldCoordinates& world);
    static IncrementCoordinates snapToVoxelGrid(const WorldCoordinates& world, VoxelResolution resolution);
    static IncrementCoordinates snapToVoxelGrid(const IncrementCoordinates& increment, VoxelResolution resolution);
    
    // Grid alignment checks
    static bool isAlignedToGrid(const IncrementCoordinates& pos, VoxelResolution resolution);
    static bool isOnIncrementGrid(const WorldCoordinates& world);
    
    // Voxel size calculations (cached for performance)
    static float getVoxelSizeMeters(VoxelResolution resolution);
    static int getVoxelSizeCm(VoxelResolution resolution);
    
    // Adjacent position calculations
    static IncrementCoordinates getAdjacentPosition(const IncrementCoordinates& pos, 
                                                   FaceDirection direction, 
                                                   VoxelResolution resolution);
    
    // Bulk operations for performance
    static void getAdjacentPositions(const IncrementCoordinates& pos,
                                    VoxelResolution resolution,
                                    IncrementCoordinates adjacentPositions[6]);
                                    
private:
    // Cached voxel sizes for performance
    static constexpr float VOXEL_SIZES_METERS[10] = {
        0.01f, 0.02f, 0.04f, 0.08f, 0.16f,
        0.32f, 0.64f, 1.28f, 2.56f, 5.12f
    };
    
    static constexpr int VOXEL_SIZES_CM[10] = {
        1, 2, 4, 8, 16, 32, 64, 128, 256, 512
    };
};
```

### 3. FaceOperations Class

Centralizes all face-related calculations and operations.

```cpp
class FaceOperations {
public:
    // Face direction utilities
    static Vector3f getFaceNormal(FaceDirection direction);  // Returns normalized direction vector
    static Vector3i getFaceOffset(FaceDirection direction, int voxelSizeCm);  // Offset in increment units
    static FaceDirection getOppositeFace(FaceDirection direction);
    
    // Face detection from ray hit
    static FaceDirection determineFaceFromHit(const WorldCoordinates& hitPoint,
                                            const VoxelBounds& voxelBounds,
                                            float epsilon = 0.01f);  // epsilon in meters
    
    // Face detection from ray direction (for exit faces)
    static FaceDirection determineFaceFromRayDirection(const Vector3f& rayDirection);  // Normalized direction
    
    // Placement calculations
    static IncrementCoordinates calculatePlacementPosition(const IncrementCoordinates& voxelPos,  // In 1cm units
                                                         FaceDirection face,
                                                         VoxelResolution resolution);
    
    // Face visibility checks
    static bool isFaceExposed(const IncrementCoordinates& voxelPos,  // In 1cm units
                             FaceDirection face,
                             const VoxelData::VoxelGrid& grid);
    
    // Bulk operations for performance
    static void getAllFaceNormals(Vector3f normals[6]);
    static void getAllFaceOffsets(int voxelSizeCm, Vector3i offsets[6]);
    
private:
    // Cached face normals for performance
    static constexpr Vector3f FACE_NORMALS[6] = {
        Vector3f(1, 0, 0),   // PositiveX
        Vector3f(-1, 0, 0),  // NegativeX
        Vector3f(0, 1, 0),   // PositiveY
        Vector3f(0, -1, 0),  // NegativeY
        Vector3f(0, 0, 1),   // PositiveZ
        Vector3f(0, 0, -1)   // NegativeZ
    };
};
```

### 4. WorkspaceValidation Class

Handles all workspace bounds checking and validation.

```cpp
class WorkspaceValidation {
public:
    // Workspace bounds in different coordinate systems
    struct WorkspaceBounds {
        IncrementCoordinates minIncrement;  // In 1cm units
        IncrementCoordinates maxIncrement;  // In 1cm units
        WorldCoordinates minWorld;  // In meters
        WorldCoordinates maxWorld;  // In meters
        Vector3f size;  // Size in meters
    };
    
    // Create workspace bounds
    static WorkspaceBounds createBounds(const Vector3f& workspaceSize);  // Size in meters
    
    // Validation methods
    static bool isInBounds(const IncrementCoordinates& pos, const WorkspaceBounds& bounds);
    static bool isInBounds(const WorldCoordinates& pos, const WorkspaceBounds& bounds);
    static bool voxelFitsInBounds(const IncrementCoordinates& pos, 
                                  VoxelResolution resolution,
                                  const WorkspaceBounds& bounds);
    
    // Clamping operations
    static IncrementCoordinates clampToBounds(const IncrementCoordinates& pos,
                                             const WorkspaceBounds& bounds);
    
    // Ground plane constraint
    static bool isAboveGroundPlane(const IncrementCoordinates& pos);
    static IncrementCoordinates clampToGroundPlane(const IncrementCoordinates& pos);
};
```

### 5. VoxelCollision Class

Handles voxel overlap and collision detection.

```cpp
class VoxelCollision {
public:
    // Single voxel collision check
    static bool checkCollision(const IncrementCoordinates& pos1, VoxelResolution res1,
                             const IncrementCoordinates& pos2, VoxelResolution res2);
    
    // Check collision with all voxels in grid
    static bool checkCollisionWithGrid(const IncrementCoordinates& pos,
                                     VoxelResolution resolution,
                                     const VoxelData::VoxelGrid& grid);
    
    // Get all colliding voxels
    static std::vector<VoxelInfo> getCollidingVoxels(const IncrementCoordinates& pos,
                                                    VoxelResolution resolution,
                                                    const VoxelData::VoxelGrid& grid);
    
    // Spatial queries
    static std::vector<VoxelInfo> getVoxelsInRegion(const VoxelBounds& region,
                                                   const VoxelData::VoxelGrid& grid);
    
private:
    // Helper to check if two voxel bounds overlap
    static bool boundsOverlap(const VoxelBounds& bounds1, const VoxelBounds& bounds2);
};
```

### 6. VoxelRaycast Class

Optimized ray-voxel intersection calculations.

```cpp
class VoxelRaycast {
public:
    struct RaycastResult {
        bool hit;
        float distance;  // Distance along ray in meters
        IncrementCoordinates voxelPos;  // Voxel position in 1cm units
        FaceDirection hitFace;
        WorldCoordinates hitPoint;  // Hit point in world coordinates
        Vector3f hitNormal;  // Normalized direction vector
    };
    
    // Single voxel raycast
    static RaycastResult raycastVoxel(const Ray& ray,  // Ray in world coordinates (meters)
                                     const IncrementCoordinates& voxelPos,  // In 1cm units
                                     VoxelResolution resolution);
    
    // Grid raycast with early termination
    static RaycastResult raycastGrid(const Ray& ray,  // Ray in world coordinates (meters)
                                   const VoxelData::VoxelGrid& grid,
                                   VoxelResolution resolution,
                                   float maxDistance = 1000.0f);  // Max distance in meters
    
    // Optimized grid traversal
    static std::vector<IncrementCoordinates> getVoxelsAlongRay(const Ray& ray,  // Ray in world coordinates
                                                              VoxelResolution resolution,
                                                              float maxDistance);  // In meters
    
private:
    // DDA traversal state
    struct TraversalState {
        IncrementCoordinates current;  // Current voxel position in 1cm units
        Vector3f tMax;  // Next intersection times in meters
        Vector3f tDelta;  // Step size in meters
        Vector3i step;  // Step direction in increment units
    };
    
    static void initializeTraversal(const Ray& ray, VoxelResolution resolution,
                                  TraversalState& state);
    static void stepTraversal(TraversalState& state);
};
```

### 7. SIMD Optimization Module

Performance-critical operations using SIMD instructions where available.

```cpp
class VoxelMathSIMD {
public:
    // Bulk coordinate conversions
    static void worldToIncrementBatch(const WorldCoordinates* world, 
                                    IncrementCoordinates* increment,
                                    size_t count);
    
    static void incrementToWorldBatch(const IncrementCoordinates* increment,
                                    WorldCoordinates* world,
                                    size_t count);
    
    // Bulk bounds calculations
    static void calculateBoundsBatch(const IncrementCoordinates* positions,  // Positions in 1cm units
                                   float voxelSizeMeters,  // Size in meters
                                   VoxelBounds* bounds,
                                   size_t count);
    
    // Bulk collision checks
    static void checkCollisionsBatch(const IncrementCoordinates* positions,
                                   const VoxelResolution* resolutions,
                                   const VoxelData::VoxelGrid& grid,
                                   bool* results,
                                   size_t count);
    
    // Check if SIMD is available on this platform
    static bool isSIMDAvailable();
    
private:
    // Platform-specific SIMD implementations
    #ifdef __SSE4_1__
    static void worldToIncrementSSE(const float* world, int* increment, size_t count);
    #endif
    
    #ifdef __AVX2__
    static void worldToIncrementAVX(const float* world, int* increment, size_t count);
    #endif
    
    #ifdef __ARM_NEON
    static void worldToIncrementNEON(const float* world, int* increment, size_t count);
    #endif
};
```

## Usage Examples

### Example 1: Voxel Placement
```cpp
// Instead of scattered calculations:
float halfSize = voxelSize * 0.5f;  // voxelSize in meters
Vector3f minCorner(basePos.x - halfSize, basePos.y, basePos.z - halfSize);
Vector3f maxCorner(basePos.x + halfSize, basePos.y + voxelSize, basePos.z + halfSize);

// Use VoxelBounds:
VoxelBounds bounds(bottomCenterPos, voxelSize);  // voxelSize in meters
BoundingBox box = bounds.toBoundingBox();
```

### Example 2: Face Detection
```cpp
// Instead of manual switch statements:
Vector3i offset(0, 0, 0);
switch (face.getDirection()) {
    case FaceDirection::PositiveX: offset.x = voxelSize_cm; break;
    // ... etc
}

// Use FaceOperations:
Vector3i offset = FaceOperations::getFaceOffset(face.getDirection(), voxelSize_cm);
```

### Example 3: Grid Snapping
```cpp
// Instead of manual calculations:
int snappedX = (increment.x() / voxelSizeCm) * voxelSizeCm;

// Use VoxelGrid:
IncrementCoordinates snapped = VoxelGrid::snapToVoxelGrid(increment, resolution);
```

## Performance Considerations

1. **Caching**: Pre-calculate and cache commonly used values (voxel sizes, face normals)
2. **Batch Operations**: Provide batch versions of operations for better cache utilization
3. **SIMD**: Use SIMD instructions for bulk coordinate conversions and collision checks
4. **Early Termination**: Implement early termination in collision and raycast operations
5. **Spatial Indexing**: Consider adding spatial acceleration structures for large voxel counts

## Testing Strategy

1. **Unit Tests**: Comprehensive tests for each operation with edge cases
2. **Performance Tests**: Benchmark SIMD vs scalar implementations
3. **Integration Tests**: Ensure compatibility with existing code
4. **Validation Tests**: Cross-check results with current implementations
5. **Fuzzing**: Test with random inputs to catch edge cases

## Migration Plan

1. **Phase 1**: Implement core classes with full test coverage
2. **Phase 2**: Gradually replace existing calculations with library calls
3. **Phase 3**: Remove deprecated functions and optimize performance
4. **Phase 4**: Add SIMD optimizations for critical paths

## Benefits

1. **Consistency**: Single source of truth for all voxel mathematics
2. **Performance**: Optimized implementations with SIMD support
3. **Maintainability**: Centralized code is easier to fix and improve
4. **Reliability**: Well-tested library reduces calculation errors
5. **Developer Experience**: Clear APIs make voxel operations simpler