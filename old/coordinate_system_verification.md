# Coordinate System Verification

This document summarizes the coordinate system assumptions verified in `default_camera_test.py` and their alignment with the C++ implementation.

## 1. World Coordinate System

### Definition:
- **Origin**: (0, 0, 0) at the corner of the workspace
- **X-axis**: Points right (positive X increases to the right)
- **Y-axis**: Points up (positive Y increases upward)
- **Z-axis**: Points forward/into screen (positive Z increases away from viewer)
- **Units**: Meters
- **Handedness**: Right-handed coordinate system

### Verification:
- Confirmed in `VoxelGrid.h`: `isInBounds()` checks `worldPos.x >= 0` indicating origin at corner
- Confirmed in `VoxelMeshGenerator.cpp`: Face normals match right-handed system
  - Top face normal: (0, 1, 0) - Y up
  - Right face normal: (1, 0, 0) - X right
  - Back face normal: (0, 0, 1) - Z forward

## 2. Voxel Grid Coordinates

### Definition:
- **Grid Origin**: Integer position (0, 0, 0) maps to world origin
- **Grid Coordinates**: Non-negative integers (gridX, gridY, gridZ)
- **Grid-to-World**: `worldPos = gridPos * voxelSize`
- **Voxel Center**: Add `voxelSize/2` to get center of voxel

### Verification:
```cpp
// From VoxelGrid::gridToWorld()
Math::Vector3f gridToWorld(const Math::Vector3i& gridPos) const {
    return Math::Vector3f(
        gridPos.x * m_voxelSize,
        gridPos.y * m_voxelSize,
        gridPos.z * m_voxelSize
    );
}

// From VoxelMeshGenerator::generateCubeMesh()
Math::Vector3f worldPos(
    voxelPos.gridPos.x * voxelSize + voxelSize * 0.5f,
    voxelPos.gridPos.y * voxelSize + voxelSize * 0.5f,
    voxelPos.gridPos.z * voxelSize + voxelSize * 0.5f
);
```

### Example:
- Grid position (0,0,0) with 8cm voxels → World center at (0.04, 0.04, 0.04)
- Grid position (4,4,4) with 8cm voxels → World center at (0.36, 0.36, 0.36)

## 3. Camera Coordinate System

### Definition:
- **Type**: Orbit camera with spherical coordinates
- **Yaw**: Rotation around Y-axis (0° = looking along +Z, 90° = looking along -X)
- **Pitch**: Elevation angle from XZ plane (0° = horizontal, positive = above)
- **Distance**: Radius from camera to target point

### Verification:
```cpp
// From OrbitCamera::updateCameraPosition()
Math::Vector3f offset(
    sinYaw * cosPitch,    // X component
    sinPitch,             // Y component
    cosYaw * cosPitch     // Z component
);
Math::Vector3f newPosition = m_target + offset * m_distance;
```

This matches the Python test implementation exactly.

### View Space:
- Camera looks down -Z axis in view space (OpenGL convention)
- X-axis points right
- Y-axis points up
- Z-axis points backwards (opposite of view direction)

## 4. Normalized Device Coordinates (NDC)

### Definition:
After projection and perspective divide:
- **X**: [-1, 1] where -1 = left edge, +1 = right edge
- **Y**: [-1, 1] where -1 = bottom edge, +1 = top edge
- **Z**: [-1, 1] where -1 = near plane, +1 = far plane

### Verification:
The test confirms all voxels project within NDC bounds:
- X range: [-0.252, 0.252] ✓
- Y range: [-0.513, 0.429] ✓
- Z range: [0.807, 0.875] ✓

## Key Constants Verified

| Parameter | Value | Source |
|-----------|-------|--------|
| Voxel Size (8cm) | 0.08m | `VoxelResolution::Size_8cm` |
| Workspace Size | 5.0m | Default configuration |
| Camera FOV | 45° | `Camera::m_fov` default |
| Camera Near | 0.1m | `Camera::m_nearPlane` default |
| Camera Far | 100.0m | `Camera::m_farPlane` default |
| Window Size | 1280×720 | `RenderWindow` dimensions |
| Aspect Ratio | 1.778 | Width/Height |

## Test Results

The test verifies that with the current camera configuration:
1. All 13 edge voxels in a 5×5×5 grid are visible
2. Field of view provides 2.6× coverage (excellent margin)
3. Camera angles (45° yaw, 30° pitch) provide good 3D perspective
4. Screen utilization is efficient (25% horizontal, 47% vertical)

The coordinate systems in the Python test align perfectly with the C++ implementation.