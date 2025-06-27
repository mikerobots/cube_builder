# Face Detection System Documentation

## Overview

The face detection system provides accurate ray-voxel face intersection for the voxel editor. It uses a clean geometric approach that separates the mathematical face detection from the voxel system specifics.

## Architecture

### Key Components

1. **GeometricFaceDetector** - Pure geometric face detection algorithm
2. **FaceDetector** - Integration layer that connects geometric detection with the voxel system
3. **Face** - Represents a voxel face with position, direction, and resolution

### Design Principles

- **Separation of Concerns**: Geometric algorithms are independent of voxel-specific logic
- **Coordinate System Clarity**: Explicit handling of coordinate system differences
- **Vertex Ordering Consistency**: All faces use counter-clockwise winding when viewed from outside

## Coordinate System

### Voxel Placement Convention

When a voxel is placed at increment coordinates (ix, iy, iz), the world position is:
- **X**: `world_x = ix * 0.01` (CENTER of voxel)
- **Y**: `world_y = iy * 0.01` (BOTTOM of voxel)
- **Z**: `world_z = iz * 0.01` (CENTER of voxel)

This convention means voxels are placed "on" the ground plane, with their bottom face at the Y coordinate.

### Example
```
Increment position: (64, 64, 64)
World position: (0.64, 0.64, 0.64)
Voxel size: 0.32m

Actual voxel bounds:
- X: [0.48 to 0.80]  (center ± size/2)
- Y: [0.64 to 0.96]  (bottom to bottom + size)
- Z: [0.48 to 0.80]  (center ± size/2)
```

## Face Ordering and Normals

### Face Indices
Faces are consistently ordered 0-5:
- 0: +X face (right) - normal pointing in +X direction
- 1: -X face (left) - normal pointing in -X direction
- 2: +Y face (top) - normal pointing in +Y direction
- 3: -Y face (bottom) - normal pointing in -Y direction
- 4: +Z face (front) - normal pointing in +Z direction
- 5: -Z face (back) - normal pointing in -Z direction

### Vertex Ordering
All faces use counter-clockwise vertex ordering when viewed from outside the voxel. This ensures normals point outward via the right-hand rule.

### Corner Naming Convention
To prevent confusion, corners are named using a three-letter system:
- First letter: X position (L=Low/-half, H=High/+half)
- Second letter: Y position (L=Low/-half, H=High/+half)
- Third letter: Z position (L=Low/-half, H=High/+half)

Example: `HLL` = corner at (+halfSize, -halfSize, -halfSize)

## Implementation Details

### GeometricFaceDetector

```cpp
// Create faces for a voxel placed on the ground plane
std::vector<GeometricFace> createVoxelFaces(const Math::Vector3f& worldPos, float voxelSize) {
    // worldPos.x and worldPos.z are CENTER coordinates
    // worldPos.y is BOTTOM coordinate
    Math::Vector3f center(worldPos.x, worldPos.y + voxelSize * 0.5f, worldPos.z);
    return createBoxFaces(center, voxelSize);
}
```

### Ray-Face Intersection Algorithm

1. **Ray-Plane Intersection**: Find where ray intersects the plane containing the face
2. **Point-in-Polygon Test**: Check if intersection point lies within the face boundaries
3. **Distance Sorting**: Return the closest face hit

### Integration with Voxel System

The FaceDetector class:
1. Retrieves all voxels from the grid
2. Creates geometric faces for each voxel
3. Performs ray intersection testing
4. Maps geometric results back to voxel Face objects

## Common Pitfalls and Solutions

### Problem: Wrong Face Hit
**Symptom**: Ray hits unexpected face (e.g., +X instead of -X)
**Cause**: Incorrect understanding of voxel positioning
**Solution**: Remember that world X,Z are centers, Y is bottom

### Problem: Incorrect Normals
**Symptom**: Face normals point inward instead of outward
**Cause**: Wrong vertex ordering
**Solution**: Use consistent counter-clockwise ordering from outside

### Problem: Ray Inside Voxel
**Symptom**: Ray starting "outside" hits wrong face
**Cause**: Misunderstanding voxel bounds
**Solution**: Calculate actual voxel bounds considering the coordinate convention

## Testing Strategy

1. **Unit Tests**: Test geometric algorithms with known inputs/outputs
2. **Integration Tests**: Test voxel-specific face detection scenarios
3. **Visual Validation**: Use Python prototypes to visualize and verify geometry

## Python Prototyping

The system was developed using Python prototypes to validate the logic:

```python
# Example from face_detection_clean.py
def create_voxel_faces_correct(world_pos, voxel_size):
    # Calculate actual voxel center
    center = np.array([
        world_pos[0],                    # X is already center
        world_pos[1] + voxel_size / 2,   # Y needs adjustment
        world_pos[2]                     # Z is already center
    ])
    # ... create faces with proper vertex ordering
```

This approach allows rapid iteration and visual debugging before C++ implementation.

## Performance Considerations

- Faces are created on-demand during ray testing
- Future optimization: Cache faces for static voxels
- Use spatial partitioning for large voxel counts

## Future Enhancements

1. **Face Caching**: Store pre-computed faces for better performance
2. **Octree Integration**: Use spatial hierarchy for faster ray testing
3. **Multi-Resolution Support**: Handle different voxel sizes efficiently
4. **Face Culling**: Skip testing faces that can't be hit by ray direction