# Camera Subsystem - Requirements Validation

## 🚨 CRITICAL: COORDINATE SYSTEM MIGRATION REQUIRED

**IMPORTANT**: The foundation coordinate system has been simplified, but this subsystem still uses the old GridCoordinates system and needs immediate updating.

### 📖 REQUIRED READING
**BEFORE STARTING**: Read `/coordinate.md` in the root directory to understand the new simplified coordinate system.

### 🎯 Migration Overview
Update the Camera subsystem from the old GridCoordinates system to the new simplified coordinate system:
- **OLD**: GridCoordinates with complex grid-to-world conversions
- **NEW**: IncrementCoordinates (1cm granularity) for all voxel operations, centered at origin (0,0,0)

### 📋 Migration Tasks (MEDIUM PRIORITY)

#### Phase 1: Remove GridCoordinates Dependencies
- [ ] **Update Camera.h** - Remove GridCoordinates if used in camera calculations
- [ ] **Update CameraController.h** - Use IncrementCoordinates for camera positioning
- [ ] **Update OrbitCamera.h** - Ensure orbit center works with centered coordinate system
- [ ] **Update Viewport.h** - Remove GridCoordinates from viewport calculations

#### Phase 2: Update Implementation Files
- [ ] **Update camera implementations** - Use IncrementCoordinates for any voxel-related positioning
- [ ] **Update ray generation** - Ensure screen-to-world rays work with centered coordinate system
- [ ] **Update view presets** - Validate view positioning with centered coordinates

#### Phase 3: Update Tests
- [ ] **test_Camera.cpp** - Update camera tests for centered coordinate system
- [ ] **test_CameraController.cpp** - Update controller tests for IncrementCoordinates
- [ ] **test_OrbitCamera.cpp** - Update orbit tests for centered coordinates
- [ ] **test_Viewport.cpp** - Update viewport tests for new coordinate system
- [ ] **Ray generation tests** - Update screen-to-world ray tests

#### Phase 4: Validation
- [ ] **Compile Check** - Ensure all files compile without GridCoordinates errors
- [ ] **Unit Tests** - Run `cd build_ninja && ctest -R "VoxelEditor_Camera_Tests"`
- [ ] **Fix Issues** - Address any failing tests or compilation errors

### 🔧 Key Code Changes Required

```cpp
// OLD - Remove all instances of:
GridCoordinates gridPos;
convertWorldToGrid();
convertGridToWorld();
#include "GridCoordinates.h"

// NEW - Replace with:
IncrementCoordinates voxelPos;
CoordinateConverter::worldToIncrement();
CoordinateConverter::incrementToWorld();
#include "foundation/math/CoordinateConverter.h"
```

### 🎯 Camera-Specific Changes

#### View Matrix Updates
- Ensure camera view matrices work with centered coordinate system
- Validate that Y=0 ground plane is correctly positioned at origin
- Update camera positioning for centered workspace bounds

#### Ray Generation Updates
- Update `screenToWorldRay()` to work with centered coordinate system
- Ensure ray intersection calculations work with IncrementCoordinates
- Validate ray generation with negative coordinate values

#### View Preset Updates
- Update all view presets to work with centered coordinate system
- Ensure orbit center is properly positioned at (0,0,0)
- Validate camera positioning relative to centered workspace

### 🎯 Success Criteria
- ✅ All GridCoordinates references removed (if any)
- ✅ Camera works correctly with centered coordinate system
- ✅ Ray generation works with centered coordinates
- ✅ View presets work with centered workspace
- ✅ All files compile without coordinate system errors
- ✅ All Camera unit tests pass

**PRIORITY**: MEDIUM - Camera system works with world coordinates but needs validation with centered system

---

## Overview
This subsystem manages 3D camera movement, view presets, and ray generation for input.
**Total Requirements**: 3

## Requirements to Validate

### View Matrices
- [ ] REQ-1.1.2: View matrices for grid at Y=0
  - The grid shall be positioned at Y=0 (ground level)
  - Camera view matrices must properly transform grid at Y=0

### View Independence
- [ ] REQ-4.2.3: View independence for highlighting
  - Highlighting shall be visible from all camera angles
  - Camera transforms must preserve highlight visibility

### Ray Transformation
- [ ] REQ-5.1.4: Ray transformation for mouse
  - Ray-casting shall determine face/position under cursor
  - Camera must provide screen-to-world ray transformation

## API Review Checklist

### Camera
- [ ] Check getViewMatrix() properly positions Y=0 grid
- [ ] Check getProjectionMatrix() maintains proper perspective
- [ ] Check screenToWorldRay() generates accurate rays

### OrbitCamera
- [ ] Check orbit controls maintain Y=0 as ground
- [ ] Check zoom functionality preserves ray accuracy
- [ ] Check rotation doesn't affect highlight visibility

### CameraController
- [ ] Check view presets properly show Y=0 grid
- [ ] Check smooth transitions maintain ray accuracy
- [ ] Check reset functionality returns to valid state

### Viewport
- [ ] Check viewport transforms for ray generation
- [ ] Check aspect ratio handling
- [ ] Check screen coordinate to NDC conversion

## Implementation Tasks
1. Verify ray generation accuracy:
   - Screen coordinates → NDC → World ray
   - Must work at all zoom levels
   - Must work with all view presets
2. Test Y=0 grid visibility:
   - Grid must be visible in all standard views
   - Grid must be at correct position
3. Ensure highlight visibility:
   - Depth testing configuration
   - No view angle should hide active highlights

## Test Coverage Needed
1. Ray generation accuracy tests
2. View matrix tests (Y=0 positioning)
3. Screen-to-world coordinate tests
4. Zoom level ray accuracy tests
5. View preset tests
6. Highlight visibility from multiple angles

## Issues Found During Review

### Code Quality Issues
1. **Viewport Ray Generation**: The `screenToWorldRay` method has a potential issue with perspective divide - it's creating rays without proper perspective divide for the clip space points.
2. **Hard-coded Values**: Several magic numbers like drag threshold (3.0f), smoothing reference FPS (60.0f), and zoom normalization (800.0f).
3. **Missing Error Handling**: No validation of matrix inversions beyond determinant check.
4. **Inconsistent Coordinate Systems**: Comments mention screen space as top-left origin, but NDC conversion assumes bottom-left.

### Design Issues
1. **Limited View Presets**: Only 7 hardcoded presets, no way to add custom presets as originally designed.
2. **No Animation System**: The original design called for a dedicated CameraAnimator, but only basic smoothing is implemented.
3. **Missing Features**: No keyboard shortcuts implementation, no touch/VR support.
4. **Event System**: Only basic event dispatch, no event data beyond change type.

### Performance Concerns
1. **Matrix Recalculation**: While caching is implemented, the dirty flag system could lead to redundant updates when multiple properties change.
2. **Smoothing System**: Uses deltaTime * 60 which assumes 60 FPS baseline - may behave differently at other framerates.

### Testing Gaps
1. No integration tests with rendering system
2. No performance benchmarks
3. No multi-threaded access testing
4. No error recovery tests