# STL Export Pipeline Summary

This document traces the complete pipeline from voxel data to STL file export, documenting each function and its assumptions.

## 1. Entry Point: CLI Command

### Function: `CLIApplication::executeCommand("surface-export")`
**Location**: `apps/cli/CLIApplication.cpp`
**Purpose**: Handle user command to export surface mesh
**Key Actions**:
- Parse filename argument
- Call `exportSurface(filename)`
**Assumptions**:
- Filename is valid and writable
- User has already placed voxels

## 2. Surface Export Handler

### Function: `CLIApplication::exportSurface()`
**Location**: `apps/cli/CLIApplication.cpp`
**Purpose**: Coordinate surface generation and file export
**Key Actions**:
- Get VoxelDataManager instance
- Call `m_surfaceGenerator->generateMultiResMesh(*voxelManager, targetRes)`
- Pass mesh to STLExporter
**Assumptions**:
- VoxelDataManager contains valid voxel data
- SurfaceGenerator is properly initialized
- Export settings specify smoothing level 0 (for SimpleMesher)

## 3. Multi-Resolution Mesh Generation

### Function: `SurfaceGenerator::generateMultiResMesh()`
**Location**: `core/surface_gen/SurfaceGenerator.cpp`
**Purpose**: Generate meshes for all voxel resolutions and combine them
**Key Actions**:
```cpp
// Find all resolutions with data
for each resolution (1cm to 512cm):
    if grid has voxels:
        mesh = generateSurface(*grid, settings)
        meshes.push_back(mesh)

// Combine meshes
return MeshBuilder::combineMeshes(meshes)
```
**Assumptions**:
- Each resolution's voxels are stored in separate grids
- All grids use the same coordinate system
- Meshes from different resolutions can be safely combined

## 4. Single Grid Surface Generation

### Function: `SurfaceGenerator::generateSurface()`
**Location**: `core/surface_gen/SurfaceGenerator.cpp`
**Purpose**: Generate mesh for a single voxel grid
**Key Actions**:
- Check cache for existing mesh
- Call `generateInternal(grid, settings, LOD0)`
**Assumptions**:
- Grid contains voxels of uniform resolution
- Settings are appropriate for the grid resolution

## 5. Internal Mesh Generation

### Function: `SurfaceGenerator::generateInternal()`
**Location**: `core/surface_gen/SurfaceGenerator.cpp`
**Purpose**: Route to appropriate mesh generator based on settings
**Key Actions**:
```cpp
if (settings.smoothingLevel == 0) {
    // Map voxel resolution to mesh resolution
    SimpleMesher::MeshResolution meshRes = mapResolution(grid.getResolution());
    mesh = m_simpleMesher->generateMesh(grid, settings, meshRes);
} else {
    // Use DualContouring for smoothed meshes
}
```
**Assumptions**:
- SimpleMesher supports all required mesh resolutions
- Voxel resolution maps correctly to mesh subdivision resolution

## 6. SimpleMesher Mesh Generation

### Function: `SimpleMesher::generateMesh()`
**Location**: `core/surface_gen/SimpleMesher.cpp`
**Purpose**: Generate exact box meshes for voxels
**Key Actions**:
```cpp
// Extract voxel positions and sizes
for each voxel in grid:
    voxels.push_back({position, size})

// Build spatial index
for each voxel:
    spatialIndex.insert(id, position, size)

// Generate mesh for each voxel (parallel)
for each voxel:
    generateVoxelMesh(id, position, size, ...)
```
**Assumptions**:
- Voxel positions are in IncrementCoordinates (1cm units)
- Voxel sizes are in centimeters
- All voxels in grid have the same resolution

## 7. Individual Voxel Mesh Generation

### Function: `SimpleMesher::generateVoxelMesh()`
**Location**: `core/surface_gen/SimpleMesher.cpp`
**Purpose**: Generate mesh for one voxel with face occlusion
**Key Actions**:
```cpp
for each face (6 faces):
    // Find adjacent voxels
    neighbors = spatialIndex.getNeighbors(position, size)
    
    // Track occlusions
    for each neighbor:
        if faceIsAdjacent(position, size, face, neighborPos, neighborSize):
            overlap = calculateOverlap(...)
            occlusionTracker.addOcclusion(overlap)
    
    // Generate visible regions
    visibleRects = occlusionTracker.computeVisibleRectangles()
    faceData = createFaceData(position, size, face, visibleRects)
    generateFace(faceData, ...)
```
**Assumptions**:
- Face occlusion is calculated in face-local 2D coordinates (cm)
- Adjacent faces are completely removed
- Partial overlaps create subdivided faces

## 8. Face Data Creation

### Function: `SimpleMesher::createFaceData()`
**Location**: `core/surface_gen/SimpleMesher.cpp`
**Purpose**: Create coordinate system for face generation
**Key Actions**:
```cpp
// Convert voxel position to world coordinates
WorldCoordinates worldPos = CoordinateConverter::incrementToWorld(voxelPos);
float size = voxelSize * 0.01f; // Convert cm to meters

// Set face origin and directions based on face
switch (face) {
    case POS_X:
        faceData.origin = WorldCoordinates(worldPos.x() + size, worldPos.y(), worldPos.z());
        faceData.uDir = Vector3f(0, 0, 1);   // Z axis
        faceData.vDir = Vector3f(0, 1, 0);   // Y axis
        ...
}
```
**Assumptions**:
- IncrementCoordinates → WorldCoordinates conversion is correct
- Face size in world units matches voxel size
- Face coordinate system is consistent

## 9. Face Generation

### Function: `SimpleMesher::generateFace()`
**Location**: `core/surface_gen/SimpleMesher.cpp`
**Purpose**: Generate triangles for visible face regions
**Key Actions**:
```cpp
for each visibleRect:
    if rect.width > meshResolution || rect.height > meshResolution:
        triangulateRectangle(rect, faceData, ...)
    else:
        // Direct quad generation
        u0 = rect.x * 0.01f; // Convert cm to meters
        v0 = rect.y * 0.01f;
        // Create vertices at corners
        p0 = faceData.origin + (faceData.uDir * u0 + faceData.vDir * v0)
        ...
```
**Assumptions**:
- Rectangle coordinates are in centimeters
- Conversion factor 0.01 correctly converts cm to meters
- Mesh subdivision based on meshResolution parameter

## 10. Mesh Combination

### Function: `MeshBuilder::combineMeshes()`
**Location**: `core/surface_gen/MeshBuilder.cpp`
**Purpose**: Combine multiple meshes into one
**Key Actions**:
```cpp
for each mesh:
    vertexOffset = builder.vertices.size()
    
    // Add all vertices
    for each vertex:
        builder.addVertex(vertex)
    
    // Add indices with offset
    for each triangle:
        builder.addTriangle(i0 + offset, i1 + offset, i2 + offset)
```
**Assumptions**:
- Meshes don't share vertices
- Simple concatenation preserves mesh integrity
- No deduplication needed between meshes

## 11. STL Export

### Function: `STLExporter::exportMesh()`
**Location**: `core/file_io/STLExporter.cpp`
**Purpose**: Write mesh to STL file
**Key Actions**:
```cpp
// Convert vertices from meters to millimeters
for each vertex:
    x_mm = vertex.x() * 1000.0f
    y_mm = vertex.y() * 1000.0f
    z_mm = vertex.z() * 1000.0f
    
// Write triangles
for each triangle:
    writeTriangle(v0_mm, v1_mm, v2_mm)
```
**Assumptions**:
- Mesh vertices are in meters (WorldCoordinates)
- STL expects millimeters
- Conversion factor 1000 is correct



Current problem:

Place a 64cm voxel at 0,0,64
Place a 16cm voxel at 24,64,88 (on the corner of the 64cm voxel)

export to STL file, and the 16cm voxel gets moved to the center of the 64cm voxel

Please note the design for SimpleMesher can be found:
./core/surface_gen/SimpleMesher.md
