# Placement System Update Plan

## Overview
Update the voxel placement system to implement new grid-aligned placement behavior where voxels snap to their own grid size by default, with 1cm increments available via Shift key.

## Key Changes Required

### 1. Core Algorithm Updates

#### 1.1 VoxelPlacementMath (`foundation/voxel_math/`)
- **Update `snapToSurfaceFaceGrid()`**:
  - Add logic to determine face "bottom-left" corner for grid origin
  - Implement grid snapping based on placement voxel size (not 1cm)
  - Add bounds checking when shift is not pressed (no overhangs)
  - Keep 1cm snapping when shift is pressed
  
- **Update `snapToGridAligned()`**:
  - Currently snaps to voxel grid - this is correct for ground plane
  - Ensure it works properly for all voxel sizes

- **Add new function `calculateFaceGridOrigin()`**:
  - Calculate the bottom-left corner of each face type
  - Account for bottom-center coordinate system

#### 1.2 PlacementValidation (`core/input/`)
- Update to use new VoxelPlacementMath behavior
- Ensure shift key state is properly passed through

### 2. Test Updates Required

#### 2.1 Unit Tests - High Priority
1. **test_unit_voxel_placement_math.cpp**
   - Update `SnapToSurfaceFaceGrid_*` tests for new grid behavior
   - Add tests for face corner grid alignment
   - Add tests for no-overhang constraint without shift
   - Update expected values for grid snapping

2. **test_unit_core_input_placement_validation.cpp**
   - Update tests that expect 1cm snapping to use voxel-size snapping
   - Add tests for shift key behavior

3. **test_unit_core_input_surface_face_grid_snapping.cpp**
   - Critical: This tests the exact behavior we're changing
   - Update all expected positions for grid-aligned snapping
   - Add tests for face corner alignment

#### 2.2 Integration Tests - Medium Priority
1. **test_integration_face_to_face_alignment.cpp**
   - Should still pass (same-size alignment is maintained)
   - May need updates if it tests shift behavior

2. **test_integration_core_ground_plane_voxel_placement.cpp**
   - Update to expect voxel-size grid snapping on ground

3. **test_unit_cli_same_size_voxel_alignment.cpp**
   - Verify still works with new grid alignment

#### 2.3 CLI Tests - Lower Priority
1. **test_unit_cli_arbitrary_positions.cpp**
   - Update to test shift key allows arbitrary positions

2. **test_unit_cli_place_command.cpp**
   - May need updates for default grid behavior

### 3. Implementation Strategy

#### Phase 1: Core Math Updates
1. Update `VoxelPlacementMath::snapToSurfaceFaceGrid()` to:
   ```cpp
   // Pseudo-code
   if (!shiftPressed) {
       // Calculate face bottom-left corner
       Vector3f gridOrigin = calculateFaceGridOrigin(surfacePos, surfaceRes, faceDir);
       
       // Snap to placement voxel grid from origin
       position = snapToGridFromOrigin(hitPoint, gridOrigin, placementRes);
       
       // Check bounds - no overhangs
       if (!isWithinFaceBounds(position, surfacePos, surfaceRes)) {
           // Clamp to bounds
       }
   } else {
       // Current 1cm behavior with overhangs allowed
   }
   ```

2. Add helper function for face grid origin calculation

#### Phase 2: Test Updates
1. Update unit tests first (they'll fail initially)
2. Fix implementation until unit tests pass
3. Update integration tests
4. Verify E2E tests still work

#### Phase 3: Special Cases
1. Handle larger voxel on smaller face (always 1cm)
2. Ensure ground plane uses voxel-size grid
3. Verify same-size voxel perfect alignment

### 4. Test Execution Order
1. Run unit tests first: `./run_all_unit.sh`
2. Run placement-specific tests:
   ```bash
   ./execute_command.sh ./build_ninja/bin/test_unit_voxel_placement_math
   ./execute_command.sh ./build_ninja/bin/test_unit_core_input_placement_validation
   ./execute_command.sh ./build_ninja/bin/test_unit_core_input_surface_face_grid_snapping
   ```
3. Run integration tests
4. Run E2E visual validation

### 5. Documentation Updates
- Update CLI_GUIDE.md to explain new placement behavior
- Update inline comments in code
- Consider adding examples in requirements.md

### 6. Validation Checklist
- [ ] Ground plane snaps to voxel grid (no shift)
- [ ] Ground plane allows 1cm with shift
- [ ] Same-size voxels align perfectly (grid = voxel size)
- [ ] Smaller voxels snap to their grid on larger faces
- [ ] Grid starts from face bottom-left corner
- [ ] No overhangs without shift
- [ ] Overhangs allowed with shift
- [ ] Larger voxels on smaller faces use 1cm
- [ ] All unit tests pass
- [ ] All integration tests pass
- [ ] Visual placement looks correct in CLI