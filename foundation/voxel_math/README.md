# Voxel Math Library

A comprehensive math library for voxel calculations that provides consistent, well-tested, and performant operations for all voxel-related mathematics in the Voxel Editor project.

## Status

### Implemented Components

1. **VoxelBounds** ✅
   - Encapsulates all bounding box calculations for voxels positioned at bottom-center
   - Provides consistent bounds calculation from both world and increment coordinates
   - Includes ray intersection, containment tests, and face calculations
   - Full unit test coverage

2. **VoxelGrid** ✅
   - Handles all grid-related calculations and snapping operations
   - Provides coordinate conversions and grid alignment checks
   - Caches voxel sizes for performance
   - Includes adjacent position calculations for all face directions
   - Full unit test coverage

3. **FaceOperations** ✅
   - Centralizes all face-related calculations and operations
   - Provides face normal vectors, offsets, and opposite face lookups
   - Includes face detection from ray hits and placement calculations
   - Conversion utilities between face directions and indices
   - Full unit test coverage

4. **WorkspaceValidation** ✅
   - Handles workspace bounds checking and validation
   - Clamping operations and ground plane constraints
   - Overhang calculations and position finding
   - Full unit test coverage

5. **VoxelCollision** ✅
   - Handles voxel overlap and collision detection
   - Spatial queries and region-based operations
   - Stability checking and connected voxel analysis
   - Full unit test coverage

6. **VoxelRaycast** ✅
   - Provides optimized ray-voxel intersection calculations
   - Grid traversal with early termination using DDA algorithm
   - Multiple hit detection and workspace intersection
   - Full unit test coverage

7. **VoxelMathSIMD** ✅
   - Provides SIMD-optimized batch operations for performance
   - Platform-specific implementations (SSE4.1, AVX2, ARM NEON)
   - Automatic fallback to scalar implementations
   - Full unit test coverage

## Usage

### VoxelBounds
```cpp
// Create bounds from world coordinates
WorldCoordinates bottomCenter(Vector3f(1.0f, 0.0f, 2.0f));
VoxelBounds bounds(bottomCenter, 0.32f);  // 32cm voxel

// Access bounds information
WorldCoordinates min = bounds.min();
WorldCoordinates max = bounds.max();
WorldCoordinates center = bounds.center();

// Check containment
bool inside = bounds.contains(somePoint);

// Ray intersection
float tMin, tMax;
bool hit = bounds.intersectsRay(ray, tMin, tMax);
```

### VoxelGrid
```cpp
// Snap to 1cm increment grid
IncrementCoordinates snapped = VoxelGridMath::snapToIncrementGrid(worldPos);

// Snap to voxel grid for specific resolution
IncrementCoordinates voxelSnapped = VoxelGridMath::snapToVoxelGrid(worldPos, resolution);

// Check grid alignment
bool aligned = VoxelGridMath::isAlignedToGrid(pos, resolution);

// Get adjacent positions
IncrementCoordinates adjacent = VoxelGridMath::getAdjacentPosition(pos, direction, resolution);
```

### FaceOperations
```cpp
// Get face normal
Vector3f normal = FaceOperations::getFaceNormal(direction);

// Calculate placement position
IncrementCoordinates newPos = FaceOperations::calculatePlacementPosition(voxelPos, face, resolution);

// Determine hit face
FaceDirection hitFace = FaceOperations::determineFaceFromHit(hitPoint, voxelBounds);

// Get face name for debugging
const char* name = FaceOperations::getFaceDirectionName(face);
```

### WorkspaceValidation
```cpp
// Create workspace bounds
Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
auto bounds = WorkspaceValidation::createBounds(workspaceSize);

// Check if position is valid
bool inBounds = WorkspaceValidation::isInBounds(pos, bounds);

// Check if voxel fits
bool fits = WorkspaceValidation::voxelFitsInBounds(pos, resolution, bounds);

// Find nearest valid position
IncrementCoordinates validPos = WorkspaceValidation::findNearestValidPosition(pos, resolution, bounds);
```

### VoxelCollision
```cpp
// Check collision between two voxels
bool collides = VoxelCollision::checkCollision(pos1, res1, pos2, res2);

// Check collision with grid
bool gridCollision = VoxelCollision::checkCollisionWithGrid(pos, resolution, grid);

// Find free position
auto freePos = VoxelCollision::findNearestFreePosition(desiredPos, resolution, grid);

// Check stability
bool stable = VoxelCollision::checkStability(pos, resolution, grid);
```

### VoxelRaycast
```cpp
// Cast ray against single voxel
Ray ray(origin, direction);
auto result = VoxelRaycast::raycastVoxel(ray, voxelPos, resolution);

// Cast ray against grid
auto gridResult = VoxelRaycast::raycastGrid(ray, grid, resolution);

// Get all hits
auto allHits = VoxelRaycast::raycastAllHits(ray, grid, resolution);

// Cast against workspace
auto workspaceHit = VoxelRaycast::raycastWorkspace(ray, workspaceSize);
```

### VoxelMathSIMD
```cpp
// Batch coordinate conversion
std::vector<WorldCoordinates> worldCoords = ...;
std::vector<IncrementCoordinates> incrementCoords(worldCoords.size());
VoxelMathSIMD::worldToIncrementBatch(worldCoords.data(), incrementCoords.data(), worldCoords.size());

// Batch distance calculation
std::vector<float> distances(positions1.size());
VoxelMathSIMD::calculateDistancesBatch(positions1.data(), positions2.data(), distances.data(), positions1.size());

// Check SIMD availability
if (VoxelMathSIMD::isSIMDAvailable()) {
    std::cout << "Using " << VoxelMathSIMD::getSIMDInstructionSet() << std::endl;
}
```

## Building

The library is integrated into the main CMake build system:

```bash
cmake -B build -G Ninja
cmake --build build
```

## Testing

Run the unit tests:

```bash
cd build
ctest -R voxel_math
```

Or run individual tests:
```bash
./test_unit_voxel_bounds
./test_unit_voxel_grid  
./test_unit_face_operations
```

## Design Principles

1. **Type Safety**: Uses coordinate types (WorldCoordinates, IncrementCoordinates) to prevent unit confusion
2. **Performance**: Caches commonly used values and provides bulk operations
3. **Consistency**: Single source of truth for all voxel mathematics
4. **Testability**: Comprehensive unit tests for all operations
5. **Clear Units**: All functions document their units clearly

## Integration

To use this library in existing code:

1. Include the appropriate header files
2. Link against `VoxelEditor_VoxelMath`
3. Replace manual calculations with library calls
4. Remove duplicate implementations

## Future Work

- Complete implementation of remaining classes
- Add SIMD optimizations for performance-critical paths
- Benchmark performance improvements
- Create migration guide for existing code