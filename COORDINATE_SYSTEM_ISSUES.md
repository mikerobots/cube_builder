# Coordinate System Issues

## Requirements (from enhancements/requirements.md)

1. **Origin (0,0,0) is at the CENTER of the workspace**
2. **5m³ workspace has coordinates from -2.5m to +2.5m on each axis**
3. **Ground plane at Y=0**
4. **No voxels below Y=0**

## Current Implementation Issues

### 1. VoxelGrid Coordinate System (WRONG)
```cpp
// VoxelGrid.h - Current implementation assumes 0-based coordinates
bool isValidWorldPosition(const Math::Vector3f& worldPos) const {
    return worldPos.x >= 0 && worldPos.x <= m_workspaceSize.x &&
           worldPos.y >= 0 && worldPos.y <= m_workspaceSize.y &&
           worldPos.z >= 0 && worldPos.z <= m_workspaceSize.z;
}

// Should be:
bool isValidWorldPosition(const Math::Vector3f& worldPos) const {
    float halfX = m_workspaceSize.x * 0.5f;
    float halfZ = m_workspaceSize.z * 0.5f;
    return worldPos.x >= -halfX && worldPos.x <= halfX &&
           worldPos.y >= 0 && worldPos.y <= m_workspaceSize.y &&
           worldPos.z >= -halfZ && worldPos.z <= halfZ;
}
```

### 2. World to Grid Conversion (WRONG)
```cpp
// VoxelGrid.h - Current doesn't account for centered workspace
Math::Vector3i worldToGrid(const Math::Vector3f& worldPos) const {
    return Math::Vector3i(
        static_cast<int>(std::floor(worldPos.x / m_voxelSize)),
        static_cast<int>(std::floor(worldPos.y / m_voxelSize)),
        static_cast<int>(std::floor(worldPos.z / m_voxelSize))
    );
}

// Should be:
Math::Vector3i worldToGrid(const Math::Vector3f& worldPos) const {
    float halfX = m_workspaceSize.x * 0.5f;
    float halfZ = m_workspaceSize.z * 0.5f;
    return Math::Vector3i(
        static_cast<int>(std::floor((worldPos.x + halfX) / m_voxelSize)),
        static_cast<int>(std::floor(worldPos.y / m_voxelSize)),
        static_cast<int>(std::floor((worldPos.z + halfZ) / m_voxelSize))
    );
}
```

### 3. Camera Initialization (WRONG)
```cpp
// Application.cpp - Camera looking at wrong center
Math::Vector3f workspaceCenter = workspaceSize * 0.5f;  // (2.5, 2.5, 2.5)

// Should be:
Math::Vector3f workspaceCenter(0.0f, workspaceSize.y * 0.5f, 0.0f);  // (0, 2.5, 0)
```

### 4. WorkspaceManager (PARTIALLY CORRECT)
```cpp
// WorkspaceManager.h - These methods are CORRECT
Math::Vector3f getMinBounds() const {
    return Math::Vector3f(-m_size.x * 0.5f, 0.0f, -m_size.z * 0.5f);
}

Math::Vector3f getMaxBounds() const {
    return Math::Vector3f(m_size.x * 0.5f, m_size.y, m_size.z * 0.5f);
}

// This clampPosition was updated and is now CORRECT
Math::Vector3f clampPosition(const Math::Vector3f& position) const {
    float halfX = m_size.x * 0.5f;
    float halfZ = m_size.z * 0.5f;
    return Math::Vector3f(
        std::clamp(position.x, -halfX, halfX),
        std::clamp(position.y, 0.0f, m_size.y),
        std::clamp(position.z, -halfZ, halfZ)
    );
}
```

## Test Issues

The tests are inconsistent with the coordinate system:

1. **test_error_handling.sh** - Tries to place at 600cm, but for 5m workspace:
   - Valid range should be -250cm to +250cm (not 0 to 500cm)
   
2. **Ground plane tests** - Correctly test Y >= 0 constraint

3. **Camera tests** - Expect camera at (2.5, 2.5, 2.5) but should be (0, 2.5, 0)

## Impact

1. **Voxel placement** - Currently accepts any position (no bounds checking working)
2. **Grid rendering** - Grid may not be centered at origin
3. **Camera views** - Looking at wrong center point
4. **Tests failing** - Tests expect centered coordinates but implementation uses 0-based

## Required Fixes

1. Fix VoxelGrid coordinate validation and conversion
2. Fix camera initialization to look at (0, workspace.y/2, 0)
3. Update tests to use centered coordinate system
4. Ensure grid rendering is centered at origin
5. Verify all subsystems use consistent coordinate system