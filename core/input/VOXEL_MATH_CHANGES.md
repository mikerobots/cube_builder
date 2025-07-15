# Input Subsystem - Voxel Math Migration

## Overview
The input subsystem needs to be updated to use the new voxel math library from `foundation/voxel_math`. This document identifies the changes needed.

## Current State Analysis

### PlacementValidation.cpp/h
- Currently implements its own placement validation logic
- Has overlap checking logic that could potentially use `VoxelCollision`
- Workspace bounds validation could use `WorkspaceValidation`
- Manual bounds calculations could use `VoxelBounds`

### Potential Changes

1. **Collision Detection**
   - Replace manual overlap checking with `VoxelCollision::checkCollision()`
   - Use `VoxelCollision::checkCollisionWithGrid()` for placement validation
   - Leverage `VoxelCollision::findNearestFreePosition()` for smart snapping

2. **Bounds Validation**
   - Replace manual bounds calculations with `VoxelBounds` class
   - Use `WorkspaceValidation` for workspace boundary checks
   - Consolidate coordinate validation logic

3. **Ray Casting** (if applicable)
   - Check if any ray-voxel intersection logic could use `VoxelRaycast`

## Implementation Plan

1. Add dependency on voxel_math library in CMakeLists.txt
2. Update PlacementValidation.cpp to use voxel math functions
3. Remove redundant validation code
4. Update tests to reflect new implementation
5. Ensure all tests pass

## Benefits
- Reduced code duplication
- Consistent validation across subsystems
- Optimized algorithms from voxel math library
- Better maintainability