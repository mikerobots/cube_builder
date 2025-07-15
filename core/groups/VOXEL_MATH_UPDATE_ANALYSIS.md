# Groups Subsystem - Voxel Math Update Analysis

## Overview
The groups subsystem needs to be updated to properly use the voxel math coordinate system types and converters. The current implementation already uses some of the coordinate types but there are areas that need improvement.

## Current State Analysis

### Already Using Voxel Math Correctly:
1. **GroupTypes.h** - VoxelId structure already uses:
   - `Math::IncrementCoordinates` for voxel positions
   - `Math::CoordinateConverter::incrementToWorld()` in `getWorldPosition()`
   - Proper coordinate type wrappers

2. **VoxelGroup.cpp** - Translation method uses:
   - `Math::WorldCoordinates` and `Math::IncrementCoordinates`
   - `Math::CoordinateConverter::worldToIncrement()` for conversion

### Areas Needing Updates:

#### 1. Direct Vector3f Usage
Several places still use raw `Math::Vector3f` that should use coordinate types:
- `GroupMetadata::pivot` - should consider using `WorldCoordinates`
- `GroupTransform::translation` - should use `WorldCoordinates`
- `GroupTransform::rotation` - OK as is (Euler angles)
- `GroupTransform::scale` - OK as is (scale factors)

#### 2. Incomplete Transform Implementation
- `VoxelGroup::rotate()` - TODO implementation needs voxel math
- `VoxelGroup::scale()` - TODO implementation needs voxel math

#### 3. Bounding Box Calculations
- `VoxelGroup::updateBounds()` - Currently uses raw Vector3f min/max
- Should ensure proper coordinate type usage throughout

#### 4. GroupOperations.cpp
Need to review this file for proper coordinate type usage in:
- Move operations
- Copy operations
- Merge operations

#### 5. GroupManager.cpp
Need to review for:
- Any direct Vector3f usage that should be coordinate types
- Proper conversion between coordinate systems

## Recommended Changes

### 1. Update GroupMetadata Structure
```cpp
struct GroupMetadata {
    // Change from Vector3f to WorldCoordinates
    Math::WorldCoordinates pivot = Math::WorldCoordinates::zero();
    // ... rest of the fields
};
```

### 2. Update GroupTransform Structure
```cpp
struct GroupTransform {
    Math::WorldCoordinates translation = Math::WorldCoordinates::zero();
    Math::Vector3f rotation = Math::Vector3f(0, 0, 0); // Keep as is - Euler angles
    Math::Vector3f scale = Math::Vector3f(1, 1, 1);    // Keep as is - scale factors
};
```

### 3. Implement Rotation and Scale Methods
These methods need proper implementation using:
- Convert voxel positions to world coordinates
- Apply transformations
- Convert back to increment coordinates
- Handle precision and rounding properly

### 4. Update Method Signatures
Methods that take Vector3f for positions should use appropriate coordinate types:
- `setPivot(const Math::WorldCoordinates& pivot)`
- `translate(const Math::WorldCoordinates& offset)` - Already done!
- `rotate(const Math::Vector3f& eulerAngles, const Math::WorldCoordinates& pivot)`
- `scale(float factor, const Math::WorldCoordinates& pivot)`

### 5. Update GroupOperations Classes
- `RotateGroupOperation` - Change pivot from Vector3f to WorldCoordinates
- `ScaleGroupOperation` - Change pivot from Vector3f to WorldCoordinates
- Already correct: `MoveGroupOperation`, `CopyGroupOperation` use WorldCoordinates

### 6. Update GroupOperationUtils
- `calculateOptimalPivot` - Should return WorldCoordinates instead of Vector3f
- `transformVoxel` - Already uses proper coordinate conversion
- Remove hardcoded workspace sizes - get from VoxelDataManager

### 7. Ensure Consistent Coordinate Usage
- All world-space positions should use `WorldCoordinates`
- All voxel storage positions should use `IncrementCoordinates`
- Use `CoordinateConverter` for all conversions
- Never mix coordinate systems without explicit conversion

## Testing Considerations
After making these changes, we need to:
1. Run all unit tests in the groups subsystem
2. Verify transform operations work correctly
3. Test edge cases with coordinate conversions
4. Ensure no regression in existing functionality

## Priority
1. High: Fix direct Vector3f usage for positions
2. High: Implement rotate/scale methods properly
3. Medium: Update method signatures for type safety
4. Low: Code cleanup and optimization