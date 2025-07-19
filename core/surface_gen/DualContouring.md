# Dual Contouring Implementation

## Overview
Dual Contouring is the primary algorithm for generating triangular meshes from voxel data. It serves as the foundation for all smoothing levels (0-10) in the voxel editor.

This is an optimized implementation that only processes cells near actual voxels, providing significant performance improvements (5x+ speedup) for sparse voxel grids.

## How Dual Contouring Fits in the Smoothing Pipeline

**Critical Context**: Dual Contouring is NOT the smoothing algorithm itself. The smoothing pipeline works as follows:

1. **Dual Contouring generates the BASE MESH** from voxel data
2. **Smoothing algorithms (Laplacian, Taubin, BiLaplacian) are applied to this base mesh**
3. **Smoothing levels**:
   - Level 0 = No smoothing applied (raw dual contouring output - blocky appearance)
   - Levels 1-3 = Light Laplacian smoothing
   - Levels 4-7 = Medium Taubin smoothing  
   - Levels 8-10+ = Heavy BiLaplacian smoothing

**Key Requirements**:
- **REQ-10.1.8**: Convert blocky voxel models into smooth, non-voxel-appearing surfaces
- **REQ-10.1.10**: User-controllable smoothing levels (0=no smoothing to 10+=maximum)
- **REQ-10.1.12**: Real-time preview of smoothing effects

**Important Implications**:
- For smoothing level 0, we need a blocky/Minecraft-like appearance
- The dual contouring output is just an intermediate step - it will be smoothed
- Perfect dual contouring quality is less critical since smoothing algorithms will modify it
- A simpler mesh generation approach that guarantees watertight meshes might be preferable

## Architecture

### Current State
- **DualContouring** class uses sparse cell traversal (refactored from DualContouringSparse)
- Supports all 10 voxel resolutions (1cm to 512cm)
- Only generates cells that potentially contain surface (near voxels)
- Avoids duplicate cell generation when voxels overlap cell ranges

### Key Concepts

#### 1. Cell Grid
- **Cells** are cubes overlaid on voxel space
- Cell size MUST match local voxel size (e.g., 32cm cells for 32cm voxels)
- Cells positioned to detect voxel boundaries (edges cross voxel faces)
- Cell positions are at multiples of voxel size from origin

#### 2. Edge Intersections
- Each cell has 12 edges (cube wireframe)
- Edge crosses surface when endpoints have different inside/outside states
- Intersection point and normal stored for QEF solving

#### 3. Vertex Generation
- QEF (Quadratic Error Function) solver finds optimal vertex position
- One vertex per cell that has edge intersections
- Vertex represents surface within that cell

#### 4. Face Generation
- Faces shared between 4 neighboring cells
- Generate quad (2 triangles) when all 4 cells have vertices
- Must handle partial faces for boundary conditions

## Key Requirements

### 1. Variable Voxel Size Support
- **REQ-DC-1.1**: Must support all 10 voxel resolutions (1cm to 512cm)
- **REQ-DC-1.2**: Cell size must match the voxel size being processed
- **REQ-DC-1.3**: Must handle mixed resolution voxels in the same grid
- **REQ-DC-1.4**: Cell boundaries must align with voxel boundaries at each resolution

### 2. Sparse Cell Generation
- **REQ-DC-2.1**: Only generate cells that potentially contain surface (near voxels)
- **REQ-DC-2.2**: For each voxel, generate cells in a 3x3x3 grid pattern around it
- **REQ-DC-2.3**: Cell increment must match voxel size (e.g., 32cm cells for 32cm voxels)
- **REQ-DC-2.4**: Avoid duplicate cell generation when voxels overlap cell ranges

### 3. Coordinate System
- **REQ-DC-3.1**: Work in IncrementCoordinates (1cm unit system)
- **REQ-DC-3.2**: Cells positioned at multiples of voxel size from grid origin
- **REQ-DC-3.3**: Support centered coordinate system (origin at 0,0,0)
- **REQ-DC-3.4**: Handle negative coordinates correctly
- **REQ-DC-3.5**: Support voxels placed at ANY 1cm increment position (not just aligned to voxel size)

### 4. Edge Processing
- **REQ-DC-4.1**: Check all 12 edges per cell for voxel intersections
- **REQ-DC-4.2**: Only store cells that have at least one edge intersection
- **REQ-DC-4.3**: Edge vertices must be offset by voxel size, not 1cm
- **REQ-DC-4.4**: Support multi-threaded edge processing for performance

### 5. Mesh Generation
- **REQ-DC-5.1**: Generate watertight meshes (no holes or gaps)
- **REQ-DC-5.2**: No duplicate faces between adjacent cells
- **REQ-DC-5.3**: Correct triangle winding order for proper normals
- **REQ-DC-5.4**: Handle shared faces between different resolution voxels

### 6. Performance
- **REQ-DC-6.1**: Achieve 5x+ speedup over dense implementation for sparse grids
- **REQ-DC-6.2**: Process cells in parallel when beneficial
- **REQ-DC-6.3**: Early exit when no voxels present
- **REQ-DC-6.4**: Efficient memory usage - only store cells with intersections

### 7. Correctness
- **REQ-DC-7.1**: Single 32cm voxel should generate exactly 12 triangles (6 faces)
- **REQ-DC-7.2**: Adjacent voxels should share vertices at boundaries
- **REQ-DC-7.3**: No degenerate triangles (area > epsilon)
- **REQ-DC-7.4**: Vertex positions within or on voxel boundaries

## Multi-Resolution Support

### Requirements
1. **Adaptive Cell Sizing**: Cells must match local voxel resolution
2. **Resolution Boundaries**: Handle transitions between different voxel sizes
3. **Watertight Meshes**: No gaps at resolution boundaries

### Current Issues
- Cell generation uses incorrect step size (16cm instead of voxel size)
- No proper handling of mixed resolution boundaries
- Single isolated voxels don't generate complete meshes

### Solution Approach
1. **Dynamic Cell Sizing**: Determine cell size from local voxels
2. **Resolution Detection**: Identify voxel size in each region
3. **Transition Handling**: Create proper faces at resolution boundaries

## Design: How Dual Contouring Should Work

### Boundary Detection Fundamentals
For dual contouring to detect a surface, cell edges must cross voxel boundaries:
- A voxel at position (x,y,z) with size S occupies space from (x,y,z) to (x+S,y+S,z+S)
- The voxel has 6 faces at: x=x, x=x+S, y=y, y=y+S, z=z, z=z+S
- To detect a boundary, we need cells whose edges cross that boundary

### The Alignment Challenge
**Critical Issue**: Voxels can be placed at ANY 1cm increment, but cells of size S must be aligned to the grid at multiples of S from the origin. This creates two options:

#### Option 1: Fixed Cell Grid (Current Approach)
- Cells are always at multiples of voxel size from origin
- For 32cm voxels: cells at (..., -64, -32, 0, 32, 64, ...)
- Problem: A voxel at (1,1,1) won't align with cell boundaries!

#### Option 2: Adaptive Cell Positioning (Correct Approach)
- Generate cells that are aligned to properly detect each voxel's boundaries
- For a voxel at arbitrary position, we need to determine which cells can detect it

### Cell Positioning for Boundary Detection

For a voxel at (vx,vy,vz) with size S, we need to find cells that will detect its boundaries.

**Key Insight**: A cell at position (cx,cy,cz) has edges that extend S units in each direction. For example:
- Edge 0: from (cx,cy,cz) to (cx+S,cy,cz)
- Edge 3: from (cx,cy,cz) to (cx,cy+S,cz)
- Edge 8: from (cx,cy,cz) to (cx,cy,cz+S)

For an edge to detect a voxel boundary, one endpoint must be inside the voxel and the other outside.

1. **Find the aligned cell grid positions that can detect the voxel**:
   ```
   // For each axis, find the cell positions whose edges can cross voxel boundaries
   cellX_min = floor(vx / S) * S      // Cells at this X can have edges crossing x=vx+S
   cellX_max = floor((vx + S) / S) * S // May equal cellX_min if voxel fits within one cell
   
   // Example: voxel at x=1 with size=32
   // Voxel occupies x=[1,33]
   // cellX_min = floor(1/32)*32 = 0  (cell at x=0 has edge to x=32, crosses x=1)
   // cellX_max = floor(33/32)*32 = 32 (cell at x=32 has edge to x=64, crosses x=33)
   ```

2. **Generate cells that can detect the voxel boundaries**:
   - Need cells from (cellX_min-S) to cellX_max in X direction
   - Similar for Y and Z directions
   - This typically creates a 3x3x3 or 4x4x4 grid depending on voxel alignment

The exact set of cells depends on how the voxel aligns with the cell grid.

### Algorithm Pseudocode

```
function generateCellsForVoxel(voxelPos, voxelSize):
    cells = Set()
    
    // Find the aligned cell positions that can detect this voxel
    // Cells must be at multiples of voxelSize from origin
    cellMinX = floor(voxelPos.x / voxelSize) * voxelSize
    cellMaxX = floor((voxelPos.x + voxelSize) / voxelSize) * voxelSize
    cellMinY = floor(voxelPos.y / voxelSize) * voxelSize
    cellMaxY = floor((voxelPos.y + voxelSize) / voxelSize) * voxelSize
    cellMinZ = floor(voxelPos.z / voxelSize) * voxelSize
    cellMaxZ = floor((voxelPos.z + voxelSize) / voxelSize) * voxelSize
    
    // Generate all cells that can detect this voxel
    // We need cells whose edges can cross the voxel boundaries
    for z from (cellMinZ - voxelSize) to cellMaxZ step voxelSize:
        for y from (cellMinY - voxelSize) to cellMaxY step voxelSize:
            for x from (cellMinX - voxelSize) to cellMaxX step voxelSize:
                // All cells at this point are aligned by construction
                if isInWorkspaceBounds(x, y, z):
                    cells.add(IncrementCoordinates(x, y, z))
    
    return cells

function buildActiveCellSet(voxelGrid):
    activeCells = Set()
    
    for each voxel in voxelGrid:
        voxelSize = getVoxelSizeInIncrements(voxel.resolution)
        cellsForVoxel = generateCellsForVoxel(voxel.position, voxelSize)
        activeCells.addAll(cellsForVoxel)
    
    return activeCells

function processCell(cellPos, voxelGrid):
    voxelSize = determineLocalVoxelSize(cellPos, voxelGrid)
    hasIntersection = false
    
    // Check all 12 edges of the cell
    for each edge in CELL_EDGES:
        // Scale edge endpoints by voxel size
        v0 = cellPos + EDGE_VERTEX_0[edge] * voxelSize
        v1 = cellPos + EDGE_VERTEX_1[edge] * voxelSize
        
        // Check if edge crosses voxel boundary
        inside0 = voxelGrid.hasVoxelAt(v0)
        inside1 = voxelGrid.hasVoxelAt(v1)
        
        if inside0 != inside1:
            hasIntersection = true
            storeEdgeIntersection(cellPos, edge, v0, v1)
    
    // Only keep cells that have surface intersections
    if hasIntersection:
        generateVertexForCell(cellPos)
```

### Boundary Conditions

1. **Single Isolated Voxel**:
   - Requires all 27 cells (3x3x3 grid) to detect all 6 faces
   - Without neighbors, all boundary edges will be detected
   - Should generate exactly 12 triangles (2 per face)

2. **Adjacent Voxels (Same Size)**:
   - Shared cells between voxels detect internal boundaries
   - Internal faces are not generated (no edge crossings)
   - Only external faces create triangles

3. **Workspace Boundaries**:
   - Cells outside workspace are not generated
   - May result in missing faces at workspace edges
   - Consider virtual "empty" space outside workspace

4. **Mixed Resolution Voxels**:
   - Smaller voxels may require additional cells
   - Larger voxel cells may encompass smaller voxel regions
   - Special handling needed at resolution boundaries

### Concrete Example: 32cm Voxel at (1,1,1)

Let's trace through the algorithm for a 32cm voxel placed at position (1,1,1):

1. **Voxel bounds**: Occupies space from (1,1,1) to (33,33,33)

2. **Find aligned cell positions**:
   ```
   cellMinX = floor(1/32) * 32 = 0
   cellMaxX = floor(33/32) * 32 = 32
   cellMinY = floor(1/32) * 32 = 0
   cellMaxY = floor(33/32) * 32 = 32
   cellMinZ = floor(1/32) * 32 = 0
   cellMaxZ = floor(33/32) * 32 = 32
   ```

3. **Generate cells**: The algorithm generates cells from:
   - X: from (0-32) to 32 = cells at x = -32, 0, 32
   - Y: from (0-32) to 32 = cells at y = -32, 0, 32
   - Z: from (0-32) to 32 = cells at z = -32, 0, 32
   
   This creates a 3x3x3 grid = 27 cells total.

4. **Which cells detect which boundaries**:
   - **X=1 boundary** (left face): Cells with x=-32 have edges from x=-32 to x=0, won't cross x=1
   - **X=33 boundary** (right face): Cells with x=0 have edges from x=32 to x=64, WILL cross x=33 ✓
   - **Y=1 boundary** (bottom face): Cells with y=-32 have edges that reach y=0, won't cross y=1
   - **Y=33 boundary** (top face): Cells with y=0 have edges from y=32 to y=64, WILL cross y=33 ✓
   - Similar analysis for Z boundaries

5. **Problem identified**: The voxel's left/bottom/back faces at x=1, y=1, z=1 won't be detected because no cell edges cross these boundaries! This is why we're getting incomplete meshes.

### Solution: Ensure Complete Boundary Detection

The issue is that cells at x=0 have edges from (0,y,z) to (32,y,z), which crosses the voxel space [1,33] but only detects where the edge ENTERS the voxel (at x=1), not where it EXITS (at x=33). We need BOTH boundaries.

**Corrected approach**: For each voxel face, ensure we have cells positioned to detect it:
- For face at x=vx: need cell at x = floor(vx/S)*S - S (so its edge crosses x=vx)
- For face at x=vx+S: need cell at x = floor(vx/S)*S (so its edge crosses x=vx+S)

This means we actually need cells at positions that ensure edges cross ALL six faces of the voxel.

### Critical Implementation Notes

1. **Cell Alignment**: Cells MUST be positioned at multiples of voxel size from origin
2. **Edge Scaling**: Edge offsets MUST be scaled by voxel size, not fixed at 1cm
3. **Voxel Detection**: The `hasVoxelAt()` check must correctly handle voxel boundaries
4. **Coordinate System**: All calculations in IncrementCoordinates (1cm units)
5. **Voxel Placement**: The algorithm must work for voxels placed at ANY 1cm increment

## Implementation Details

### Cell Generation Algorithm
```
For each voxel at position (x,y,z) with size S:
  - Generate cells from (x-S, y-S, z-S) to (x+S, y+S, z+S)
  - Cell increment = S (not 1cm)
  - Clip to workspace bounds
  - Store in hash set to avoid duplicates
```

### Edge Direction Scaling
The base DualContouring class defines EDGE_DIRECTIONS with 1cm offsets. For variable voxel sizes, these must be scaled:
```cpp
// Base class has: EDGE_DIRECTIONS[0] = IncrementCoordinates(1, 0, 0)
// For 32cm voxel: offset should be (32, 0, 0)
```

### Mixed Resolution Handling
When voxels of different sizes are adjacent:
- Larger voxel's cells encompass smaller voxel's region
- Smaller voxel may create additional detail cells
- Shared faces must align at common boundaries

## Known Limitations

### Single Voxel Meshes
Dual contouring works best with adjacent voxels. Single isolated voxels create incomplete meshes because:
- Only partial neighbor information available
- Boundary faces need special handling
- Algorithm designed for continuous surfaces

### Resolution Transitions
Current implementation doesn't properly handle:
- Small voxels on large voxel faces
- T-junctions at resolution boundaries
- Adaptive cell refinement

## Test Cases

### Single Voxel Tests
1. 32cm voxel at (0,0,0) → 12 triangles, 8 vertices
2. 1cm voxel at (0,0,0) → 12 triangles, 8 vertices
3. 512cm voxel at (0,0,0) → 12 triangles, 8 vertices

### Adjacent Voxel Tests
1. Two 32cm voxels side-by-side → 20 triangles (shared face removed)
2. 32cm next to 16cm voxel → proper face sharing
3. Grid of 8 voxels (2x2x2) → fully connected mesh

### Performance Tests
1. 1000 scattered voxels → <100ms generation time
2. Dense 10x10x10 grid → comparable to base implementation
3. Single voxel in large workspace → <10ms

## Common Issues and Solutions

### Issue: Too Many Cells Generated
- **Symptom**: 9000+ cells for single voxel
- **Cause**: Using 1cm increment instead of voxel size
- **Solution**: Increment by voxelSizeIncrements in loops

### Issue: Missing Faces
- **Symptom**: 0 triangles generated
- **Cause**: Edge vertices not scaled to voxel size
- **Solution**: Scale EDGE_DIRECTIONS by voxel size

### Issue: Duplicate Faces
- **Symptom**: 28+ triangles for single voxel
- **Cause**: Multiple cells generating same face
- **Solution**: Proper cell boundary calculation

## Future Improvements

1. **Adaptive Octree Cells**: Use octree for better multi-resolution support
2. **Boundary Handling**: Special case for isolated voxels
3. **LOD Support**: Level-of-detail for large scenes
4. **GPU Acceleration**: Parallel processing on GPU