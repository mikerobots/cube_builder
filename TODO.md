# TODO.md - Voxel Placement Requirements Update

This TODO.md list is shared by multiple agents. If you pick up a task,
mark it as "In Progress, [Your Name]". Update it as you are
working. Don't pick up any work that is "In progress" by other
agents. If you have not picked a random name, please pick one. Once
you finish a task, pick another one. Do not disable tests unless
specifically approved.

---

## Requirements Change Reference
**IMPORTANT**: Read `/Users/michaelhalloran/cube_edit/REQUIREMENTS_CHANGE_SUMMARY.md` before starting any work.
This document explains the voxel placement behavior changes in detail.

## Phase 1: Core Code Updates

### Math/Coordinate System Updates
- [ ] **MATH-001**: Remove `snapToVoxelResolution()` function from `foundation/math/CoordinateConverter.h`
- [ ] **MATH-002**: Remove all calls to `snapToVoxelResolution()` throughout the codebase
- [ ] **MATH-003**: Verify coordinate conversion functions work with arbitrary 1cm positions

### Voxel Data Storage Updates
- [ ] **VOXEL-001**: Update `VoxelGrid::incrementToGrid()` to remove snapping behavior
- [ ] **VOXEL-002**: Verify `VoxelGrid` can store voxels at any 1cm position
- [ ] **VOXEL-003**: Update `VoxelDataManager::wouldOverlapInternal()` for exact position overlap detection
- [ ] **VOXEL-004**: Remove commented alignment validation in `VoxelDataManager.h` (lines 61-68)
- [ ] **VOXEL-005**: Update `VoxelDataManager::setVoxel()` to handle exact positions
- [ ] **VOXEL-006**: Verify sparse octree storage works with non-aligned positions

### Input System Updates
- [ ] **INPUT-001**: Update mouse position to voxel position calculations
- [ ] **INPUT-002**: Ensure placement preview shows exact 1cm positions
- [ ] **INPUT-003**: Update face-to-face alignment logic (REQ-3.1.1) to work without snapping

### Visual Feedback Updates
- [ ] **VIS-001**: Update preview positioning to show exact placement location
- [ ] **VIS-002**: Ensure grid highlighting works with arbitrary positions
- [ ] **VIS-003**: Update face detection for non-aligned voxels

## Phase 2: Test Updates

### Unit Test Updates
- [ ] **UNIT-001**: Remove snapping tests from `test_unit_foundation_coordinate_converter.cpp`
- [ ] **UNIT-002**: Update `test_unit_core_voxel_grid.cpp` - test arbitrary position storage
- [ ] **UNIT-003**: Update `test_unit_core_voxel_data_manager.cpp` - test exact position placement
- [ ] **UNIT-004**: Update `test_unit_core_voxel_data_collision.cpp` - test overlap at exact positions
- [ ] **UNIT-005**: Add new tests for 1cm increment placement validation

### Integration Test Updates
- [ ] **INT-001**: Fix `test_integration_overlap_detection.cpp` - remove snapping assumptions
- [ ] **INT-002**: Update `test_integration_voxel_placement.cpp` - test exact positions
- [ ] **INT-003**: Update `test_integration_face_detection.cpp` - handle non-aligned voxels
- [ ] **INT-004**: Update `test_integration_preview_positioning.cpp` - test exact preview
- [ ] **INT-005**: Add integration tests for face-to-face alignment without snapping

### CLI Test Updates
- [ ] **CLI-001**: Update CLI integration tests for new placement behavior
- [ ] **CLI-002**: Update E2E tests to expect exact positions
- [ ] **CLI-003**: Verify CLI commands work with arbitrary positions

## Phase 3: Code Review

### Review Core Changes
- [ ] **REVIEW-001**: Review all math/coordinate system changes
- [ ] **REVIEW-002**: Review voxel data storage changes
- [ ] **REVIEW-003**: Review input system changes
- [ ] **REVIEW-004**: Review visual feedback changes
- [ ] **REVIEW-005**: Ensure no snapping logic remains anywhere

### Review Test Changes
- [ ] **REVIEW-006**: Review unit test updates
- [ ] **REVIEW-007**: Review integration test updates
- [ ] **REVIEW-008**: Review CLI test updates
- [ ] **REVIEW-009**: Verify test coverage for new behavior

## Phase 4: Build and Fix

### Foundation Layer
- [ ] **BUILD-001**: Build and run foundation unit tests
- [ ] **FIX-001**: Fix any foundation test failures

### Core Voxel Data
- [ ] **BUILD-002**: Build and run voxel data unit tests
- [ ] **FIX-002**: Fix any voxel data test failures

### Core Systems
- [ ] **BUILD-003**: Build and run all core unit tests
- [ ] **FIX-003**: Fix any core system test failures

### Integration Tests
- [ ] **BUILD-004**: Build and run integration tests
- [ ] **FIX-004**: Fix any integration test failures

### CLI Tests
- [ ] **BUILD-005**: Build and run CLI tests
- [ ] **FIX-005**: Fix any CLI test failures

## Phase 5: Validation

- [ ] **VAL-001**: Verify voxels can be placed at any 1cm position
- [ ] **VAL-002**: Verify no resolution-based snapping occurs
- [ ] **VAL-003**: Verify overlap detection works correctly
- [ ] **VAL-004**: Verify face-to-face alignment still works (REQ-3.1.1)
- [ ] **VAL-005**: Run full test suite and ensure all tests pass
- [ ] **VAL-006**: Manual testing of CLI to verify behavior

## Notes

1. **Do NOT** start Phase 4 (Build and Fix) until ALL code changes in Phases 1-3 are complete
2. Each task should be marked as complete only after code changes AND review
3. When fixing tests, verify the test logic matches new requirements, don't just make tests pass
4. Document any design decisions or edge cases discovered during implementation
5. If you find additional code that needs updating, add it to the appropriate phase

## Completed Tasks

### Research and Planning (Completed by previous agent)
- [x] Analyzed current implementation behavior
- [x] Identified ambiguity in requirements
- [x] Updated requirements.md with clarifications
- [x] Created REQUIREMENTS_CHANGE_SUMMARY.md