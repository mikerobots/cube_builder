# Coordinate System and Voxel Placement Guide

## Overview

This guide explains the coordinate system and voxel placement rules for the Voxel Editor. Understanding these concepts is essential for working with the editor's core functionality.

## Coordinate System

### Origin and Orientation
- **Origin**: The coordinate system is centered at (0, 0, 0)
- **Orientation**: Y-up coordinate system (Y axis points upward)
- **Units**: All measurements are in meters (m) for world space

### Coordinate Types

The system uses three distinct coordinate types for different purposes:

1. **WorldCoordinates**: Continuous 3D space coordinates
   - Used for: rendering, camera positions, ray casting, physics
   - Type: Vector3f (floating-point precision)
   - Units: meters

2. **IncrementCoordinates**: Universal voxel storage coordinates
   - Used for: voxel storage and addressing at 1cm granularity
   - Type: Vector3i (integer precision)
   - Units: 1cm increments
   - Example: IncrementCoordinates(100, 0, 50) = 100cm x, 0cm y, 50cm z

3. **ScreenCoordinates**: 2D viewport positions
   - Used for: mouse input, UI elements, viewport operations
   - Type: Vector2f
   - Units: screen pixels

## Workspace

### Default Workspace
- **Size**: 5m³ (5 meters cubed)
- **Bounds**: -2.5m to +2.5m on each axis (X, Y, Z)
- **Adjustable**: Can be resized between 2m³ and 8m³

### Workspace Coordinates
```
Default 5m³ workspace:
- X axis: -250cm to +250cm (-2.5m to +2.5m)
- Y axis: -250cm to +250cm (-2.5m to +2.5m)
- Z axis: -250cm to +250cm (-2.5m to +2.5m)

Origin (0,0,0) is at the exact center
```

## Ground Plane

### Ground Plane Definition
- **Position**: Y = 0 (the XZ plane at Y coordinate 0)
- **Purpose**: Represents the "floor" for voxel placement
- **Constraint**: No voxels can be placed below the ground plane (Y < 0)

### Ground Plane Grid
- **Grid Size**: 32cm x 32cm squares
- **Standard Lines**: RGB(180, 180, 180) at 35% opacity
- **Major Lines**: RGB(200, 200, 200) every 160cm (5 grid squares), thicker
- **Dynamic Opacity**: Increases to 65% within 2 grid squares of cursor during placement

## Voxel Placement Rules

### 1cm Precision System
- **All voxels** are placed at 1cm increment positions
- **No grid snapping**: Voxels don't snap to the 32cm grid
- **Universal precision**: Applies to all voxel sizes (1cm to 512cm)

### Available Voxel Sizes
The editor supports 10 voxel resolutions:
- 1cm, 2cm, 4cm, 8cm, 16cm, 32cm, 64cm, 128cm, 256cm, 512cm

### Placement Constraints

1. **Y ≥ 0 Constraint**: 
   - Voxels cannot be placed below the ground plane
   - The bottom face of a voxel must be at Y ≥ 0

2. **Workspace Bounds**:
   - The entire voxel extent must fit within workspace bounds
   - Example: A 32cm voxel at position (240, 0, 240) is valid
   - But at position (245, 0, 245) would exceed bounds (245 + 32 = 277 > 250)

3. **Collision Detection**:
   - Voxels cannot overlap with existing voxels
   - Smaller voxels can be placed adjacent to larger ones
   - Smaller voxels cannot be placed inside larger voxels

### Placement Examples

#### Ground Plane Placement
```
Valid placements on ground plane (Y = 0):
- 1cm voxel at (0, 0, 0): Bottom at Y=0, top at Y=1
- 32cm voxel at (50, 0, -100): Bottom at Y=0, top at Y=32
- 512cm voxel at (0, 0, 0): Bottom at Y=0, top at Y=512

Invalid placement:
- Any voxel at (0, -1, 0): Below ground plane
```

#### Adjacent Placement
```
Existing: 32cm voxel at (0, 0, 0)
Valid adjacent placements:
- 32cm voxel at (32, 0, 0): Perfectly aligned, touching
- 16cm voxel at (32, 0, 0): Smaller, still adjacent
- 1cm voxel at (32, 0, 0): Tiny detail next to large voxel

Invalid overlapping placement:
- 32cm voxel at (16, 0, 0): Would overlap existing voxel
```

## Face-Based Placement

When placing voxels on existing voxel faces:

### Same-Size Auto-Alignment
- When placing a voxel of the same size on a face, it auto-aligns
- Edges of the new voxel match perfectly with the clicked face
- Hold **Shift** to override auto-alignment for 1cm precision placement

### Face Selection
- Hovering over a voxel highlights the face under the cursor (yellow)
- Only one face is highlighted at a time
- Clicking places the new voxel adjacent to that face

### Placement Plane Persistence
- When placing larger voxels on smaller ones, the placement plane "sticks"
- The plane maintains its height while the preview overlaps any voxel
- Plane only changes when preview completely clears current height voxels

## Multi-Resolution Support

### Resolution Independence
- Changing resolution does NOT clear existing voxels
- Voxels of different sizes can coexist in the same workspace
- Each resolution level maintains its own voxel storage

### Resolution-Specific Operations
- `getVoxelCount()` returns count for current resolution only
- Separate method available for counting across all resolutions
- Fill command creates voxels of the current resolution size

## Visual Feedback

### Placement Preview
- **Green outline**: Valid placement position
- **Red outline**: Invalid placement (collision or out of bounds)
- Preview updates in real-time with mouse movement
- Shows exact 1cm increment position where voxel will be placed

### Interactive Feedback
- Grid opacity increases when placing nearby
- Face highlighting shows which face is selected
- Preview snaps to nearest valid 1cm position

## Common Coordinate Conversions

### World to Increment Coordinates
```cpp
// Convert world position (meters) to increment position (1cm units)
WorldCoordinates worldPos(1.5f, 0.0f, -2.3f);  // 1.5m, 0m, -2.3m
IncrementCoordinates incPos(150, 0, -230);     // 150cm, 0cm, -230cm
```

### Placement Validation Example
```cpp
// Check if a 32cm voxel can be placed at position (100, 0, 200)
IncrementCoordinates position(100, 0, 200);  // 1m, 0m, 2m
int voxelSize = 32;  // 32cm

// Validate Y ≥ 0
if (position.y() < 0) return false;

// Validate workspace bounds (assuming 5m³ workspace)
if (position.x() + voxelSize > 250) return false;  // Would exceed +2.5m
if (position.z() + voxelSize > 250) return false;  // Would exceed +2.5m

// Check for collisions with existing voxels
// ... collision detection logic ...
```

## Best Practices

1. **Always validate placement** before creating voxels
2. **Use IncrementCoordinates** for voxel storage and logic
3. **Convert to WorldCoordinates** only for rendering
4. **Respect the ground plane** - no negative Y positions
5. **Check workspace bounds** including voxel extent
6. **Handle multi-resolution** scenarios properly

## Summary

The Voxel Editor uses a centered coordinate system with 1cm precision for all voxel placements. The ground plane at Y=0 serves as the floor, and all voxels must be placed at or above this level. Understanding these coordinate systems and placement rules is crucial for implementing features and debugging issues in the editor.