# Surface Generation Subsystem - Voxel Math Migration Analysis

## Overview
This document identifies all changes needed to migrate the surface_gen subsystem to use the new voxel math library.

## Key Files Requiring Changes

### 1. SurfaceTypes.h
- **Current**: Uses foundation math types (Vector3f, Vector3i, BoundingBox, CoordinateTypes)
- **Changes Needed**:
  - Replace `Math::BoundingBox` with `VoxelBounds` where dealing with voxel bounds
  - Keep `Math::Vector3f` for normals and directions (as per voxel math design)
  - Replace coordinate calculations with `VoxelGrid` operations
  - Use `WorldCoordinates` and `IncrementCoordinates` from voxel math

### 2. DualContouring.cpp/h
- **Current**: Manual voxel position calculations and bounds computation
- **Changes Needed**:
  - Use `VoxelBounds` for all voxel bounding box calculations
  - Replace manual grid snapping with `VoxelGrid::snapToVoxelGrid()`
  - Use `FaceOperations` for face normal calculations
  - Replace manual voxel size calculations with `VoxelGrid::getVoxelSizeMeters()`

### 3. DualContouringSparse.cpp/h
- **Current**: Custom sparse voxel traversal and position calculations
- **Changes Needed**:
  - Use `VoxelGrid` for adjacent position calculations
  - Replace collision detection with `VoxelCollision` methods
  - Use `VoxelBounds` for intersection tests

### 4. MeshBuilder.cpp/h
- **Current**: Manual vertex positioning from voxel coordinates
- **Changes Needed**:
  - Use `VoxelBounds` to get accurate vertex positions
  - Replace coordinate conversions with voxel math converters
  - Use `FaceOperations` for face-based mesh generation

### 5. MeshValidator.cpp/h
- **Current**: Manual bounds checking and validation
- **Changes Needed**:
  - Use `VoxelBounds` for mesh bounds validation
  - Replace manual coordinate checks with `WorkspaceValidation`

### 6. SurfaceGenerator.cpp/h
- **Current**: Orchestrates surface generation with manual coordinate handling
- **Changes Needed**:
  - Use `VoxelGrid` for resolution-based calculations
  - Replace bounds calculations with `VoxelBounds`
  - Use `WorkspaceValidation` for workspace checks

### 7. TopologyPreserver.cpp/h
- **Current**: Preserves mesh topology during smoothing
- **Changes Needed**:
  - Minimal changes - mostly deals with mesh data, not voxel positions
  - May benefit from `VoxelBounds` for feature size calculations

### 8. MeshSmoother.cpp/h
- **Current**: Smoothing algorithms on mesh vertices
- **Changes Needed**:
  - Minimal changes - operates on mesh data after voxel conversion
  - Keep using Vector3f for mesh vertices as they're not voxel positions

## Key Patterns to Replace

### 1. Voxel Size Calculations
**Before:**
```cpp
float voxelSize = VoxelSizes::getSizeInMeters(resolution);
int voxelSizeCm = static_cast<int>(voxelSize * 100.0f);
```

**After:**
```cpp
float voxelSize = VoxelGrid::getVoxelSizeMeters(resolution);
int voxelSizeCm = VoxelGrid::getVoxelSizeCm(resolution);
```

### 2. Voxel Bounds Creation
**Before:**
```cpp
float halfSize = voxelSize * 0.5f;
Vector3f min(pos.x - halfSize, pos.y, pos.z - halfSize);
Vector3f max(pos.x + halfSize, pos.y + voxelSize, pos.z + halfSize);
BoundingBox bounds(min, max);
```

**After:**
```cpp
VoxelBounds bounds(bottomCenterPos, voxelSize);
BoundingBox box = bounds.toBoundingBox();
```

### 3. Face Normal Calculations
**Before:**
```cpp
Vector3f normal;
switch(face) {
    case FaceDirection::PositiveX: normal = Vector3f(1, 0, 0); break;
    // ... etc
}
```

**After:**
```cpp
Vector3f normal = FaceOperations::getFaceNormal(face);
```

### 4. Adjacent Voxel Positions
**Before:**
```cpp
Vector3i offset = getFaceOffset(face);
Vector3i adjacentPos = currentPos + offset * voxelSizeCm;
```

**After:**
```cpp
IncrementCoordinates adjacentPos = VoxelGrid::getAdjacentPosition(currentPos, face, resolution);
```

## Implementation Order

1. **Phase 1**: Update includes and basic types
   - Add voxel math includes
   - Update coordinate type usage

2. **Phase 2**: Replace calculations
   - Voxel bounds calculations
   - Grid operations
   - Face operations

3. **Phase 3**: Update algorithms
   - Dual contouring traversal
   - Sparse voxel handling
   - Mesh building

4. **Phase 4**: Testing and validation
   - Run unit tests
   - Fix any issues
   - Verify performance

## Notes

- The voxel math library is focused on voxel-specific calculations
- Keep using Vector3f for pure geometric operations (normals, mesh vertices)
- WorldCoordinates should be used for positions in world space
- IncrementCoordinates should be used for voxel grid positions
- The library provides optimized batch operations that should be used where possible