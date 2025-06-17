# Coordinate System Fix Summary

## The Issue

There's a fundamental coordinate system mismatch in the voxel placement system:

1. **WorkspaceManager** assumes a centered coordinate system:
   - X axis: -workspaceSize.x/2 to +workspaceSize.x/2
   - Y axis: 0 to workspaceSize.y
   - Z axis: -workspaceSize.z/2 to +workspaceSize.z/2
   - This is evident in `getMinBounds()` and `getMaxBounds()` methods

2. **VoxelGrid** assumes a non-centered coordinate system:
   - X axis: 0 to workspaceSize.x
   - Y axis: 0 to workspaceSize.y
   - Z axis: 0 to workspaceSize.z
   - This is evident in `isValidWorldPosition()` method

3. **VoxelPosition::fromWorldSpace** doesn't handle the coordinate transformation

## Why Tests Hang

When `test_workspace_boundary_placement.cpp` tries to place voxels at negative coordinates:
1. The test assumes centered coordinates (correctly matching WorkspaceManager)
2. `setVoxelAtWorldPos` is called with negative world coordinates
3. `VoxelGrid::worldToGrid` converts these to negative grid indices
4. Various checks may fail or cause unexpected behavior with negative indices

## The Fix

We need to choose one coordinate system and make it consistent throughout:

### Option 1: Keep Centered Coordinates (Recommended)
This is more intuitive for users and matches the camera/rendering system.

Changes needed:
1. Update `VoxelGrid::isValidWorldPosition()` to use centered bounds
2. Update `VoxelGrid::worldToGrid()` to handle the coordinate offset
3. Update `VoxelPosition::fromWorldSpace()` to handle the offset

### Option 2: Switch to Non-Centered Coordinates
This would require more changes throughout the codebase.

## Immediate Fix for VoxelGrid

```cpp
// In VoxelGrid.h

bool isValidWorldPosition(const Math::Vector3f& worldPos) const {
    // Use centered coordinate system
    float halfX = m_workspaceSize.x * 0.5f;
    float halfZ = m_workspaceSize.z * 0.5f;
    return worldPos.x >= -halfX && worldPos.x <= halfX &&
           worldPos.y >= 0 && worldPos.y <= m_workspaceSize.y &&
           worldPos.z >= -halfZ && worldPos.z <= halfZ;
}

Math::Vector3i worldToGrid(const Math::Vector3f& worldPos) const {
    // Convert from centered world coordinates to grid indices
    float halfX = m_workspaceSize.x * 0.5f;
    float halfZ = m_workspaceSize.z * 0.5f;
    
    // Shift from centered to non-centered coordinates
    float shiftedX = worldPos.x + halfX;
    float shiftedZ = worldPos.z + halfZ;
    
    return Math::Vector3i(
        static_cast<int>(std::floor(shiftedX / m_voxelSize)),
        static_cast<int>(std::floor(worldPos.y / m_voxelSize)),
        static_cast<int>(std::floor(shiftedZ / m_voxelSize))
    );
}

Math::Vector3f gridToWorld(const Math::Vector3i& gridPos) const {
    // Convert from grid indices to centered world coordinates
    float halfX = m_workspaceSize.x * 0.5f;
    float halfZ = m_workspaceSize.z * 0.5f;
    
    return Math::Vector3f(
        gridPos.x * m_voxelSize - halfX,
        gridPos.y * m_voxelSize,
        gridPos.z * m_voxelSize - halfZ
    );
}
```

## Testing the Fix

After implementing the fix:
1. Run `test_workspace_boundary_placement` to verify it no longer hangs
2. Check that voxels can be placed at negative coordinates
3. Verify that the visual output shows voxels in the correct positions
4. Run all other tests to ensure nothing else breaks