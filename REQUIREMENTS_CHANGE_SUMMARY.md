# Requirements Change Summary: Voxel Placement Behavior

**Date**: December 2024  
**Purpose**: This document summarizes the recent clarification of voxel placement requirements. Multiple agents will use this to update code and tests.

## Overview of Change

The voxel placement behavior has been clarified to **remove resolution-based grid snapping**. Voxels should be placeable at any 1cm increment position, regardless of their size.

## What Changed

### Previous Understanding (Incorrect)
- Voxels would snap to resolution-based grids (e.g., 4cm voxels snap to 0, 4, 8, 12... positions)
- Different resolution voxels had different valid placement positions
- Position (1,1,1) would snap to (0,0,0) for a 4cm voxel

### New Requirements (Correct)
- **NO resolution-based snapping**
- All voxels can be placed at ANY 1cm increment position
- A 4cm voxel placed at (1,1,1) stays at (1,1,1) - no snapping
- A 32cm voxel placed at (7,13,21) stays at (7,13,21) - no snapping

## Updated Requirements

The following requirements in `requirements.md` were clarified:

1. **REQ-2.1.1**: "Voxels shall be placed at any 1cm increment position without resolution-based snapping"
   - Changed from: "Voxels can be placed at 1cm increments on the ground plane"
   - **Impact**: Remove any grid snapping logic

2. **REQ-2.1.2**: "Voxels of all sizes shall maintain their exact placement position in 1cm increments"
   - Changed from: "Within each 32cm grid cell, there shall be exactly 32 valid positions per axis"
   - **Impact**: All voxel sizes use the same 1cm grid

3. **REQ-2.2.2**: "The preview shall show the exact 1cm increment position where the voxel will be placed"
   - Changed from: "The preview shall snap to the nearest valid 1cm increment position..."
   - **Impact**: Preview shows actual placement position, no snapping

4. **REQ-2.2.4**: "Voxels of any size (1cm to 512cm) shall be placeable at any 1cm increment position"
   - Changed from: "All voxel sizes shall be placeable at any valid 1cm increment position..."
   - **Impact**: Emphasizes all sizes use same placement rules

## Code Areas Requiring Updates

### 1. VoxelGrid Implementation
**File**: `core/voxel_data/VoxelGrid.h`
- **Current**: Uses `snapToVoxelResolution()` in `incrementToGrid()` method
- **Required Change**: Remove snapping, use positions directly

### 2. CoordinateConverter
**File**: `foundation/math/CoordinateConverter.h`
- **Current**: `snapToVoxelResolution()` snaps to multiples of voxel size
- **Required Change**: This function should likely be removed or deprecated

### 3. VoxelDataManager
**File**: `core/voxel_data/VoxelDataManager.h`
- **Current**: Has commented-out alignment validation (lines 61-68)
- **Required Change**: Ensure no resolution-based validation remains

### 4. Tests Requiring Updates
Many tests assume resolution-based snapping behavior:

- **`test_integration_overlap_detection.cpp`**
  - Currently expects positions like (1,1,1) to snap to (0,0,0) for 4cm voxels
  - Needs rewrite to test actual overlap at exact positions

- **Unit tests for VoxelGrid**
  - May test snapping behavior that should be removed

- **Integration tests for placement**
  - May expect snapped positions instead of exact positions

## Important Notes

### REQ-3.1.1 Clarification
This requirement about "face-to-face alignment" is **NOT about grid snapping**. It means:
- When placing a same-size voxel on another voxel's face
- The new voxel is positioned so its edges align with the clicked face
- This is about adjacent placement, not grid snapping
- Example: Clicking the top face of a 4cm voxel at (10,10,10) places the new 4cm voxel at (10,14,10)

### Face-to-Face Alignment Logic
The face alignment calculation should work like this:
1. User clicks on a face of an existing voxel
2. Calculate the adjacent position based on face normal
3. For same-size voxels, ensure edges align perfectly
4. No grid snapping involved - just proper adjacent positioning

## Migration Strategy

1. **Remove Snapping Functions**
   - Delete or deprecate `snapToVoxelResolution()`
   - Update all callers to use positions directly

2. **Update VoxelGrid Storage**
   - Ensure it can store voxels at any 1cm position
   - Remove any resolution-based position validation

3. **Fix Tests**
   - Update test expectations to match new behavior
   - Add new tests for 1cm increment placement

4. **Verify Collision Detection**
   - Ensure overlap detection works with exact positions
   - No false positives from snapping

## Example Scenarios

### Before (Incorrect)
```cpp
// Place 4cm voxel at (1,1,1)
// System snaps to (0,0,0)
// Voxel stored at (0,0,0)
```

### After (Correct)
```cpp
// Place 4cm voxel at (1,1,1)
// No snapping occurs
// Voxel stored at (1,1,1) with size 4cm
```

### Overlap Detection Example
```cpp
// 4cm voxel at (0,0,0) occupies space from (0,0,0) to (3,3,3)
// Attempting to place any voxel at (1,1,1) should fail - it's inside the first voxel
// Attempting to place a voxel at (4,0,0) should succeed - it's outside
```

## Success Criteria

The implementation is correct when:
1. Voxels can be placed at any 1cm position (0, 1, 2, 3, ... 31, 32, 33...)
2. No resolution-based grid snapping occurs
3. Overlap detection works based on actual voxel bounds
4. Face-to-face alignment still works for same-size voxels (REQ-3.1.1)
5. All tests pass with the new behavior

## Questions to Resolve

If any agent encounters these situations, please document the decision:
1. Should `snapToVoxelResolution()` be removed entirely or kept but unused?
2. How should the VoxelGrid internally store positions? Keep current structure or redesign?
3. Are there any performance implications of allowing arbitrary 1cm positions?