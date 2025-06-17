# TODO - Fix Integration Test Failures

## 🎯 CURRENT WORK: Fixing Broken Integration Tests

**Context**: After fixing FaceDetector coordinate system and completing core subsystem cleanup, several integration tests are still failing.

### Test Status Summary

**✅ Passing Tests:**
- `test_ground_plane_voxel_placement` - All 6 tests passed
- `test_mouse_ray_movement` - All 8 tests passed
- `test_mouse_boundary_clicking` - All 5 tests passed
- `test_voxel_face_clicking_simple` - All 3 tests passed (fixed with FaceDetector coordinate system)
- `VoxelEditor_VoxelData_Tests` - Tests are passing

**❌ Failing Tests:**
1. `test_mouse_ground_plane_clicking` - 1/6 tests failed
2. `CoreFunctionalityTests` - 2/6 tests failed (4 skipped in headless mode)
3. `test_click_voxel_placement` - All 3 tests failed
4. `test_voxel_face_clicking` - Hangs (possible infinite loop or deadlock)

### 🔥 HIGH PRIORITY FAILURES

#### 1. Mouse Ground Plane Clicking Test - ClickGroundPlaneMultiplePositions
**Status**: ⏳ PENDING  
**Failure**: Expected 7 voxels placed, but only 5 were placed
**Theory**: Likely related to coordinate transformation or workspace boundary issues
- Missing voxels at grid positions (90, 0, 50) and (50, 0, 90)
- These positions might be outside the valid workspace bounds
- Or the coordinate transformation is placing them incorrectly

#### 2. Core Functionality Tests
**Status**: ⏳ PENDING

**a) OneCmIncrementPlacement Test**
- **Failure**: `isValidIncrementPosition(invalidPos)` returns false when expected true
- **Theory**: The test's understanding of valid increment positions doesn't match the new coordinate system
- Likely testing with old coordinate assumptions that are no longer valid

**b) UndoRedoOperational Test**  
- **Failure**: `historyManager->undo()` returns false
- **Theory**: Commands might not be properly registered in history manager
- Or the command execution is failing silently, so there's nothing to undo

#### 3. Click Voxel Placement Tests
**Status**: ⏳ PENDING
**Failure**: All tests fail at voxel placement with "Failed to place voxel"
**Theory**: 
- The click-to-placement pipeline is broken somewhere
- Likely related to face detection → placement position calculation
- May be using old coordinate system assumptions
- The Face's calculatePlacementPosition might return invalid positions

#### 4. Voxel Face Clicking Test (Hanging)
**Status**: ⏳ PENDING  
**Issue**: Test hangs indefinitely
**Theory**:
- Possible infinite loop in ray traversal algorithm
- Or deadlock in voxel placement logic
- Could be related to the coordinate system changes causing unexpected behavior

### 🛠️ DEBUGGING APPROACH

#### Priority Order:
1. **Click Voxel Placement Tests** - Core functionality that other tests depend on
2. **Voxel Face Clicking Test** - Hanging issue could reveal systemic problems
3. **Mouse Ground Plane Clicking** - Boundary case handling
4. **Core Functionality Tests** - May resolve themselves once placement works

### 📋 ROOT CAUSE ANALYSIS

**Common Thread**: All failures seem related to the coordinate system changes
- FaceDetector was fixed to handle centered workspace
- But other components (PlacementUtils, CalculatePlacementPosition, Commands) may still use old assumptions
- The placement pipeline: Face Detection → Calculate Position → Validate → Place needs review

### 🔄 NEXT ACTIONS

**1. Fix Click Voxel Placement (HIGHEST PRIORITY)**
   - Check if calculatePlacementPosition returns valid increment coordinates
   - Verify PlacementUtils validates positions correctly with new coordinate system
   - Ensure commands properly convert between coordinate systems

**2. Debug Hanging Test**
   - Add timeout to identify where it hangs
   - Check for infinite loops in ray traversal
   - Look for coordinate values that might cause infinite iteration

**3. Fix Boundary Issues**
   - Review workspace bounds validation
   - Check if positions (90,0,50) and (50,0,90) are within valid bounds
   - Verify coordinate snapping at boundaries

**4. Update Test Assumptions**
   - Review what positions tests consider "valid"
   - Update tests to match new coordinate system
   - Ensure undo/redo tracks commands correctly

**Strategy**: Start with click placement as it's fundamental to all voxel operations. The coordinate system fix for FaceDetector needs to be propagated through the entire placement pipeline.