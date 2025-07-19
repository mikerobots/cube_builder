# Simple Mesh Algorithm Design (Revised)

## Overview

The Simple Mesh Algorithm generates exact box meshes for voxels, preserving all voxel geometry before smoothing. Every voxel is represented as a complete box mesh with intelligent face removal at boundaries.

## Core Requirements

### 1. Exact Voxel Representation
- **REQ-SM-1.1**: Every voxel generates a box mesh of its exact size
- **REQ-SM-1.2**: No voxel geometry is lost or simplified
- **REQ-SM-1.3**: Support all voxel sizes (1cm to 512cm)
- **REQ-SM-1.4**: Support voxels placed at any 1cm increment

### 2. Face Removal Rules
- **REQ-SM-2.1**: Remove faces where two voxels share the exact same face
- **REQ-SM-2.2**: Keep faces where voxels only partially overlap
- **REQ-SM-2.3**: Handle multiple small voxels on a single large face

### 3. Mesh Quality
- **REQ-SM-3.1**: Generate watertight meshes (no holes except at boundaries)
- **REQ-SM-3.2**: Correct triangle winding for proper normals (CCW for outward)
- **REQ-SM-3.3**: No duplicate vertices within tolerance (0.1mm)
- **REQ-SM-3.4**: No T-junctions (vertices on other triangles' edges)
- **REQ-SM-3.5**: Support mesh subdivision for smoothing preparation

### 4. Performance
- **REQ-SM-4.1**: O(n log n) or better performance for n voxels
- **REQ-SM-4.2**: Memory usage proportional to output mesh size
- **REQ-SM-4.3**: Handle 10,000+ voxels efficiently

### 5. Constraints
- **REQ-SM-5.1**: Voxels cannot overlap in space
- **REQ-SM-5.2**: All voxel sizes > 0
- **REQ-SM-5.3**: Mesh resolution limited to: 1cm, 2cm, 4cm, 8cm, 16cm

## Data Structures

```
// Unique identifier for each voxel
TYPE VoxelId = int  // Simple index into voxel array

// Rectangle in face-local coordinates (always in cm units)
STRUCTURE Rectangle:
    x, y: int        // Position relative to face origin (cm)
    width, height: int  // Size in cm
    // (0,0) is always at the face's minimum corner in UV space
```

## Algorithm Design

### Phase 1: Spatial Indexing

```
STRUCTURE SpatialIndex:
    cellSize = 512cm  // Must be >= largest voxel size (512cm)
    grid = HashMap<CellKey, List<VoxelId>>
    
FUNCTION insert(voxel):
    bounds = voxel.getBounds()
    for each cell overlapped by bounds:
        grid[cell].add(voxel.id)
        
FUNCTION getNeighbors(voxel):
    neighbors = Set<VoxelId>
    bounds = voxel.getBounds()
    for each cell overlapped by bounds.expanded(1cm):
        neighbors.addAll(grid[cell])
    return neighbors - voxel.id
```

### Phase 2: Vertex Management

```
STRUCTURE VertexManager:
    vertices = List<Vector3>
    vertexMap = HashMap<VertexKey, VertexIndex>
    tolerance = 0.1mm
    
FUNCTION getOrCreateVertex(position):
    // Convert to integer coordinates for exact comparison
    key = VertexKey(round(position * 10000))  // 0.1mm units
    
    if key in vertexMap:
        return vertexMap[key]
    
    index = vertices.size()
    vertices.append(position)
    vertexMap[key] = index
    return index
```

### Phase 3: Face Occlusion Tracking

```
STRUCTURE Rectangle:
    x, y, width, height  // In cm units
    
FUNCTION FaceOcclusionTracker.computeVisibleRectangles():
    // Start with full face
    visibleRects = [Rectangle(0, 0, faceSize, faceSize)]
    
    // Subtract each occlusion
    for each occlusion in occludedRegions:
        newVisible = []
        for each rect in visibleRects:
            newVisible.extend(subtractRectangle(rect, occlusion))
        visibleRects = newVisible
    
    // Merge adjacent rectangles to reduce complexity
    return mergeAdjacentRectangles(visibleRects)

FUNCTION subtractRectangle(rect, occlusion):
    if not rect.intersects(occlusion):
        return [rect]
    
    if occlusion.contains(rect):
        return []  // Fully occluded
    
    // Calculate up to 4 remaining rectangles
    result = []
    
    // Top strip
    if occlusion.y > rect.y:
        result.add(Rectangle(rect.x, rect.y, 
                           rect.width, occlusion.y - rect.y))
    
    // Bottom strip
    if occlusion.bottom < rect.bottom:
        result.add(Rectangle(rect.x, occlusion.bottom,
                           rect.width, rect.bottom - occlusion.bottom))
    
    // Left strip (only middle part)
    overlapY = max(rect.y, occlusion.y)
    overlapBottom = min(rect.bottom, occlusion.bottom)
    if occlusion.x > rect.x and overlapBottom > overlapY:
        result.add(Rectangle(rect.x, overlapY,
                           occlusion.x - rect.x, overlapBottom - overlapY))
    
    // Right strip (only middle part)
    if occlusion.right < rect.right and overlapBottom > overlapY:
        result.add(Rectangle(occlusion.right, overlapY,
                           rect.right - occlusion.right, overlapBottom - overlapY))
    
    return result
```

### Phase 4: Edge Vertex Registry

```
STRUCTURE EdgeKey:
    p1: Vector3  // First point (in 0.1mm integer units)
    p2: Vector3  // Second point (in 0.1mm integer units)
    
FUNCTION createEdgeKey(start, end):
    // Convert to integer units for exact comparison
    p1 = round(start * 10000)  // 0.1mm units
    p2 = round(end * 10000)
    
    // Normalize: smaller point first (lexicographic order)
    if p1.x < p2.x or (p1.x == p2.x and p1.y < p2.y) or 
       (p1.x == p2.x and p1.y == p2.y and p1.z < p2.z):
        return EdgeKey(p1, p2)
    else:
        return EdgeKey(p2, p1)

STRUCTURE EdgeVertexRegistry:
    edgeVertices = HashMap<EdgeKey, List<VertexIndex>>
    mutex = Mutex()  // For thread safety
    
FUNCTION getOrCreateEdgeVertices(start, end, subdivisionSize, vertexManager):
    mutex.lock()
    
    // Normalize edge
    key = createEdgeKey(start, end)
    
    if key in edgeVertices:
        mutex.unlock()
        return edgeVertices[key]
    
    // Create vertices at regular intervals
    vertices = []
    edgeLength = distance(start, end)
    numSegments = ceil(edgeLength / subdivisionSize)
    
    for i from 0 to numSegments:
        t = i / numSegments
        position = lerp(start, end, t)
        vertices.append(vertexManager.getOrCreateVertex(position))
    
    edgeVertices[key] = vertices
    mutex.unlock()
    return vertices
```

### Phase 5: Face Generation

```
FUNCTION ensureEdgeVertices(faceData, edgeRegistry, vertexManager, meshResolution):
    // Get the 4 edges of the face in world coordinates
    origin = faceData.origin
    uMax = origin + faceData.uDir * faceData.size
    vMax = origin + faceData.vDir * faceData.size
    uvMax = origin + faceData.uDir * faceData.size + faceData.vDir * faceData.size
    
    // Bottom edge (along U direction)
    faceData.bottomEdgeVertices = edgeRegistry.getOrCreateEdgeVertices(
        origin, uMax, meshResolution, vertexManager)
    
    // Top edge (along U direction)
    faceData.topEdgeVertices = edgeRegistry.getOrCreateEdgeVertices(
        vMax, uvMax, meshResolution, vertexManager)
    
    // Left edge (along V direction)
    faceData.leftEdgeVertices = edgeRegistry.getOrCreateEdgeVertices(
        origin, vMax, meshResolution, vertexManager)
    
    // Right edge (along V direction)
    faceData.rightEdgeVertices = edgeRegistry.getOrCreateEdgeVertices(
        uMax, uvMax, meshResolution, vertexManager)

FUNCTION generateFace(faceData, indices, edgeRegistry, vertexManager, meshResolution):
    // 1. Ensure edge vertices exist for all four edges
    ensureEdgeVertices(faceData, edgeRegistry, vertexManager, meshResolution)
    
    // 2. Process each visible rectangle
    for each rect in faceData.visibleRectangles:
        // Skip very small rectangles
        if rect.width < meshResolution/2 or rect.height < meshResolution/2:
            continue
            
        // Generate and triangulate this rectangle
        triangulateRectangle(rect, faceData, indices, vertexManager, meshResolution)

FUNCTION getEdgeIndex(position, edgeLength, meshResolution):
    // Given a position along an edge, find the corresponding vertex index
    // in the edge vertex list
    return round(position / meshResolution)

FUNCTION triangulateRectangle(rect, faceData, indices, vertexManager, meshResolution):
    // Calculate subdivisions
    uSubdivisions = rect.width / meshResolution
    vSubdivisions = rect.height / meshResolution
    uRemainder = rect.width % meshResolution
    vRemainder = rect.height % meshResolution
    
    // Build vertex grid for this rectangle
    vertices = Matrix<VertexIndex>
    
    // Use edge vertices where rectangle touches face edges
    for v from 0 to vSubdivisions + (vRemainder > 0 ? 1 : 0):
        for u from 0 to uSubdivisions + (uRemainder > 0 ? 1 : 0):
            vertexPos = calculateVertexPosition(rect, u, v, faceData)
            
            // Check if this vertex is on a face edge
            if rect.x == 0 and u == 0:  // On left edge
                vertices[v][u] = faceData.leftEdgeVertices[getEdgeIndex(rect.y + v*meshResolution, faceData.size)]
            else if rect.x + rect.width == faceData.size and u == uSubdivisions + (uRemainder > 0 ? 1 : 0) - 1:  // On right edge
                vertices[v][u] = faceData.rightEdgeVertices[getEdgeIndex(rect.y + v*meshResolution, faceData.size)]
            else if rect.y == 0 and v == 0:  // On bottom edge
                vertices[v][u] = faceData.bottomEdgeVertices[getEdgeIndex(rect.x + u*meshResolution, faceData.size)]
            else if rect.y + rect.height == faceData.size and v == vSubdivisions + (vRemainder > 0 ? 1 : 0) - 1:  // On top edge
                vertices[v][u] = faceData.topEdgeVertices[getEdgeIndex(rect.x + u*meshResolution, faceData.size)]
            else:  // Interior vertex
                vertices[v][u] = vertexManager.getOrCreateVertex(vertexPos)
    
    // Generate quads (2 triangles each)
    for v from 0 to vSubdivisions - 1:
        for u from 0 to uSubdivisions - 1:
            addQuad(vertices[v][u], vertices[v][u+1], 
                   vertices[v+1][u+1], vertices[v+1][u], indices)
    
    // Handle remainder quads with different sizes
    if uRemainder > 0:
        for v from 0 to vSubdivisions - 1:
            addQuad(vertices[v][uSubdivisions], vertices[v][uSubdivisions+1],
                   vertices[v+1][uSubdivisions+1], vertices[v+1][uSubdivisions], indices)
    
    if vRemainder > 0:
        for u from 0 to uSubdivisions - 1:
            addQuad(vertices[vSubdivisions][u], vertices[vSubdivisions][u+1],
                   vertices[vSubdivisions+1][u+1], vertices[vSubdivisions+1][u], indices)
    
    // Corner remainder
    if uRemainder > 0 and vRemainder > 0:
        addQuad(vertices[vSubdivisions][uSubdivisions], 
               vertices[vSubdivisions][uSubdivisions+1],
               vertices[vSubdivisions+1][uSubdivisions+1], 
               vertices[vSubdivisions+1][uSubdivisions], indices)
```

### Phase 6: Main Algorithm

```
FUNCTION generateMesh(voxelGrid, meshResolution):
    // 0. Validate inputs
    if meshResolution not in [1, 2, 4, 8, 16]:
        error("Mesh resolution must be 1, 2, 4, 8, or 16 cm")
    
    // 1. Build spatial index
    spatialIndex = SpatialIndex()
    for each voxel in voxelGrid:
        spatialIndex.insert(voxel)
    
    // 2. Initialize components
    vertexManager = VertexManager()
    edgeRegistry = EdgeVertexRegistry()
    indices = []
    
    // 3. Process each voxel
    for each voxel in voxelGrid:
        generateVoxelMesh(voxel, spatialIndex, vertexManager, 
                         edgeRegistry, indices)
    
    // 4. Build final mesh
    mesh = Mesh()
    mesh.vertices = vertexManager.vertices
    mesh.indices = indices
    return mesh

FUNCTION generateVoxelMesh(voxel, spatialIndex, vertexManager, edgeRegistry, indices):
    // Process each of 6 faces
    for each face in [NEG_X, POS_X, NEG_Y, POS_Y, NEG_Z, POS_Z]:
        // Find occlusions from neighboring voxels
        occlusionTracker = FaceOcclusionTracker(voxel.size)
        neighbors = spatialIndex.getNeighbors(voxel)
        
        for each neighbor in neighbors:
            if faceIsAdjacent(voxel, face, neighbor):
                overlap = calculateOverlap(voxel, face, neighbor)
                if overlap:
                    occlusionTracker.addOcclusion(overlap)
        
        // Get visible regions
        visibleRects = occlusionTracker.computeVisibleRectangles()
        
        if not visibleRects.empty():
            faceData = createFaceData(voxel, face, visibleRects)
            generateFace(faceData, indices)
```

## Optimizations for Degenerate Cases

### Checkerboard Pattern Optimization
When many small voxels create a complex pattern:

```
FUNCTION optimizeRectangles(rectangles):
    // Try to merge rectangles into larger strips
    sorted = sortByPosition(rectangles)
    merged = []
    
    for each rect in sorted:
        if merged.empty() or not canMerge(merged.last(), rect):
            merged.append(rect)
        else:
            merged.last() = merge(merged.last(), rect)
    
    return merged
```

### Early Rejection
```
FUNCTION shouldProcessFace(voxel, face, spatialIndex):
    // Quick check if face is completely internal
    adjacentCell = getAdjacentCellPosition(voxel, face)
    potentialBlockers = spatialIndex.getVoxelsAt(adjacentCell)
    
    for each blocker in potentialBlockers:
        if blocker.size >= voxel.size and facesAlign(voxel, face, blocker):
            return false  // Face is completely blocked
    
    return true
```

## Thread Safety Strategy

```
STRUCTURE ThreadLocalData:
    vertexManager: VertexManager  // Thread-local vertices
    indices: List<int>            // Thread-local indices
    
FUNCTION processParallel(voxels):
    numThreads = getHardwareConcurrency()
    threadData = Array<ThreadLocalData>[numThreads]
    
    // Process voxels in parallel
    parallel_for(threadIndex from 0 to numThreads):
        batch = voxels[threadIndex::numThreads]  // Strided access
        for each voxel in batch:
            generateVoxelMesh(voxel, spatialIndex, 
                            threadData[threadIndex].vertexManager,
                            sharedEdgeRegistry,  // Mutex protected
                            threadData[threadIndex].indices)
    
    // Merge results
    finalMesh = mergeThreadResults(threadData)
    return finalMesh
```

## Memory Estimates (Corrected)

### Per Voxel with Subdivision:
- Face size 32cm, mesh resolution 8cm = 5x5 vertices per face
- 6 faces × 25 vertices = 150 vertices maximum
- With sharing between faces: ~98 unique vertices per voxel
- 12 triangles × 6 faces × 16 subdivisions = 1,152 triangles per large voxel

### For 10,000 Mixed Voxels:
- Average ~50 vertices per voxel (mix of sizes) = 500,000 vertices
- Vertices: 500,000 × 12 bytes = 6MB
- Indices: ~2,000,000 × 4 bytes = 8MB
- Spatial index: ~500KB
- Working memory: ~15MB total

## Critical Algorithm Details

### Face Coordinate System and Normals
```
FUNCTION createFaceData(voxel, faceDirection, visibleRects):
    faceData = FaceData()
    
    // Define consistent coordinate system for each face
    // Origin is always at minimum corner of face
    switch faceDirection:
        case NEG_X:  // Left face
            faceData.origin = voxel.position
            faceData.uDir = (0, 0, 1)   // Z axis
            faceData.vDir = (0, 1, 0)   // Y axis
            faceData.normal = (-1, 0, 0)
        case POS_X:  // Right face
            faceData.origin = voxel.position + (voxel.size, 0, 0)
            faceData.uDir = (0, 0, -1)  // -Z axis (for correct winding)
            faceData.vDir = (0, 1, 0)   // Y axis
            faceData.normal = (1, 0, 0)
        case NEG_Y:  // Bottom face
            faceData.origin = voxel.position
            faceData.uDir = (1, 0, 0)   // X axis
            faceData.vDir = (0, 0, 1)   // Z axis
            faceData.normal = (0, -1, 0)
        case POS_Y:  // Top face
            faceData.origin = voxel.position + (0, voxel.size, 0)
            faceData.uDir = (1, 0, 0)   // X axis
            faceData.vDir = (0, 0, -1)  // -Z axis (for correct winding)
            faceData.normal = (0, 1, 0)
        case NEG_Z:  // Back face
            faceData.origin = voxel.position
            faceData.uDir = (-1, 0, 0)  // -X axis (for correct winding)
            faceData.vDir = (0, 1, 0)   // Y axis
            faceData.normal = (0, 0, -1)
        case POS_Z:  // Front face
            faceData.origin = voxel.position + (0, 0, voxel.size)
            faceData.uDir = (1, 0, 0)   // X axis
            faceData.vDir = (0, 1, 0)   // Y axis
            faceData.normal = (0, 0, 1)
    
    faceData.size = voxel.size
    faceData.visibleRectangles = visibleRects
    return faceData
```

### Triangle Winding Order
```
FUNCTION addQuad(v0, v1, v2, v3, indices):
    // Vertices ordered CCW when viewed from outside
    // v0 --- v1
    // |      |
    // v3 --- v2
    
    // First triangle: v0, v1, v2
    indices.add(v0)
    indices.add(v1)
    indices.add(v2)
    
    // Second triangle: v0, v2, v3
    indices.add(v0)
    indices.add(v2)
    indices.add(v3)
```

### Face Adjacency Definition
```
FUNCTION faceIsAdjacent(voxel, face, neighbor):
    // Two faces are adjacent if they are coplanar and overlap
    
    // Get face plane position
    facePlanePos = getFacePlanePosition(voxel, face)
    neighborFaces = getPotentiallyAdjacentFaces(neighbor, face)
    
    for each nFace in neighborFaces:
        nFacePlanePos = getFacePlanePosition(neighbor, nFace)
        
        // Check if coplanar (exact match required)
        if facePlanePos == nFacePlanePos:
            // Check if faces overlap in 2D
            if facesOverlap2D(voxel, face, neighbor, nFace):
                return true
    
    return false

FUNCTION calculateOverlap(voxel, face, neighbor):
    // Calculate the 2D rectangle where neighbor occludes voxel's face
    // Returns rectangle in face-local coordinates (0,0 is face origin)
    
    // Project both voxels onto the face plane
    voxelRect = projectToFacePlane(voxel, face)
    neighborRect = projectToFacePlane(neighbor, face)
    
    // Calculate intersection in face coordinates
    overlap = rectangleIntersection(voxelRect, neighborRect)
    
    // Convert to face-local coordinates
    overlap.x -= voxelRect.x
    overlap.y -= voxelRect.y
    
    return overlap
```

### Thread Safety Fix
```
STRUCTURE ThreadSafeComponents:
    vertexManager: VertexManager      // Thread-local
    localEdgeVertices: List<Edge>     // Thread-local edge tracking
    indices: List<int>                // Thread-local
    
FUNCTION mergeThreadResults(threadData):
    // Global vertex manager for final merge
    globalVertexManager = VertexManager()
    globalIndices = []
    
    // First pass: collect all unique edges
    allEdges = Set<Edge>
    for each thread in threadData:
        allEdges.addAll(thread.localEdgeVertices)
    
    // Second pass: create global vertices with consistent ordering
    for each edge in sorted(allEdges):
        createGlobalEdgeVertices(edge, globalVertexManager)
    
    // Third pass: remap thread-local indices to global indices
    for each thread in threadData:
        remapping = createVertexRemapping(thread.vertexManager, globalVertexManager)
        for each idx in thread.indices:
            globalIndices.add(remapping[idx])
    
    return (globalVertexManager.vertices, globalIndices)
```

## Testing Strategy

### Correctness Tests:
1. Single voxel → all faces visible
2. Two adjacent same-size voxels → shared face removed
3. Small voxel on large face → hole in large face
4. Multiple overlapping occlusions → correct visible regions
5. Edge alignment → no cracks between faces

### Performance Tests:
1. 10,000 random voxels < 1 second
2. Memory usage linear with output size
3. Parallel speedup near linear with thread count

### Edge Cases:
1. Voxel at workspace boundary
2. 1cm voxel on 512cm voxel
3. Complete face occlusion
4. Degenerate rectangles after subtraction
5. Non-aligned voxel faces (different sizes)
6. Maximum workspace extents (-1000m to +1000m)