# TODO.md - Unit Test Fixes

This TODO.md list is shared by multiple agents. If you pick up a task,
mark it as "In Progress, [Your Name]". Update it as you are
working. Don't pick up any work that is "In progress" by other
agents. If you have not picked a random name, please pick one. Once
you finish a task, pick another one. Do not disable tests unless
specifically approved.

IMPORTANT: To prevent collisions between agents:
- ALWAYS use sed to update this TODO.md file (never use Write or Edit tools)
- ALWAYS mark a task as "In Progress, [Your Name]" BEFORE starting work on it
- Use exact match patterns that will FAIL if someone else already updated the line
- Examples (note the exact match patterns that include the full line):
  - Mark task in progress: sed -i '' 's/^- FaceDetectorTest.MinimalRaycast_VoxelAtOrigin$/- FaceDetectorTest.MinimalRaycast_VoxelAtOrigin/' TODO.md
  - Mark checklist item done: sed -i '' '/FaceDetectorTest.MinimalRaycast_VoxelAtOrigin.*In Progress, Alice/,/Update TODO.md/ s/  - \[ \] Check to see if it still valid/  - \[x\] Check to see if it still valid/' TODO.md
  - Mark test complete: sed -i '' 's/^- FaceDetectorTest.MinimalRaycast_VoxelAtOrigin \*\*In Progress, Alice\*\*$/- FaceDetectorTest.MinimalRaycast_VoxelAtOrigin **COMPLETE**/' TODO.md
- If sed fails, it means someone else is working on that item - pick a different task!

## Current Test Status (2025-06-27)

**Unit Test Summary:**
- **Total Tests**: 123
- **Passed**: 112 ✓
- **Failed**: 11 ✗
- **Success Rate**: 91%

## Currently Failing Unit Tests (11 total - new failures after command refactoring)

### 1. test_unit_cli_arbitrary_positions
- **Issue**: Test failures after command refactoring
- **Details**: Likely related to command API changes or registration issues
- **Severity**: Medium - affects arbitrary position placement functionality
- **Status**: Available for work
- [ ] Review test failures and command registration
- [ ] Fix command handling or update test expectations
- [ ] Verify fix and update TODO.md

### 2. test_unit_cli_coordinate_system_constraints
- **Issue**: Test failures after command refactoring
- **Details**: Likely related to coordinate validation in new command system
- **Severity**: Medium - affects coordinate system constraints
- **Status**: Available for work
- [ ] Review test failures and command validation
- [ ] Fix coordinate handling or update test expectations
- [ ] Verify fix and update TODO.md

### 3. test_unit_cli_select_commands
- **Issue**: 6 out of 10 tests failing after command refactoring
- **Details**: Commands are registered but selection operations failing
- **Severity**: High - affects selection functionality
- **Status**: Available for work
- [ ] Debug selection command execution
- [ ] Fix selection logic or command parameters
- [ ] Verify fix and update TODO.md

### 4. test_unit_cli_smoothing_commands
- **Issue**: Test failures after command refactoring
- **Details**: Smoothing command functionality affected
- **Severity**: Medium - affects mesh smoothing features
- **Status**: Available for work
- [ ] Review smoothing command implementation
- [ ] Fix command handling or update tests
- [ ] Verify fix and update TODO.md

### 5. test_unit_core_rendering_ground_plane_grid
- **Issue**: Ground plane grid rendering tests failing
- **Details**: Not related to command refactoring
- **Severity**: Medium - affects visual feedback
- **Status**: Available for work
- [ ] Debug ground plane grid rendering
- [ ] Fix rendering logic
- [ ] Verify fix and update TODO.md

### 6. test_unit_core_rendering_ground_plane_grid_dynamics
- **Issue**: Dynamic grid opacity/behavior tests failing
- **Details**: Not related to command refactoring
- **Severity**: Medium - affects visual feedback dynamics
- **Status**: Available for work
- [ ] Debug dynamic grid behavior
- [ ] Fix grid dynamics logic
- [ ] Verify fix and update TODO.md

### 7. test_unit_core_rendering_shader_manager
- **Issue**: Shader manager tests failing
- **Details**: Not related to command refactoring
- **Severity**: High - affects rendering pipeline
- **Status**: Available for work
- [ ] Debug shader manager issues
- [ ] Fix shader compilation/linking
- [ ] Verify fix and update TODO.md

### 8. test_unit_core_undo_redo_voxel_commands
- **Issue**: Undo/redo command tests failing
- **Details**: May be related to command refactoring changes
- **Severity**: High - affects undo/redo functionality
- **Status**: Available for work
- [ ] Review undo/redo command integration
- [ ] Fix command execution or state management
- [ ] Verify fix and update TODO.md

### 9. test_unit_core_voxel_data_collision_simple
- **Issue**: Voxel collision detection tests failing
- **Details**: Not related to command refactoring
- **Severity**: High - affects voxel placement validation
- **Status**: Available for work
- [ ] Debug collision detection logic
- [ ] Fix collision algorithms
- [ ] Verify fix and update TODO.md

### 10. test_unit_core_voxel_data_grid
- **Issue**: Voxel grid tests failing
- **Details**: Not related to command refactoring
- **Severity**: High - core voxel storage functionality
- **Status**: Available for work
- [ ] Debug voxel grid operations
- [ ] Fix grid logic
- [ ] Verify fix and update TODO.md

### 11. test_unit_core_voxel_data_manager
- **Issue**: Voxel data manager tests failing
- **Details**: Not related to command refactoring
- **Severity**: High - core voxel management functionality
- **Status**: Available for work
- [ ] Debug voxel data manager operations
- [ ] Fix manager logic
- [ ] Verify fix and update TODO.md

## Previously Completed Tests (moved below new failures)

### test_unit_cli_smoothing_commands **COMPLETE**
- **Test**: SmoothCommandAlgorithmAutoSelection
- **Fix**: Removed condition that prevented algorithm auto-selection when level changes
- **Issue**: Test assertion failure (not a crash/timeout)
- **Severity**: Low - feature-specific test
- [x] Review test logic and expectations
- [x] Fix algorithm auto-selection logic or update test
- [x] Verify fix and update TODO.md

### 2. test_unit_core_surface_gen_mesh_smoother **COMPLETE**
- **Test**: PreviewQualityMode
- **Issue**: Preview mode takes 70ms vs normal mode 65ms (expected preview to be faster)
- **Fix**: Updated test to use larger mesh (100 vertices) and multiple runs for stable timing comparison
- **Severity**: Low - performance expectation mismatch
- [x] Review preview mode optimization settings
- [x] Either optimize preview mode or adjust test expectations
- [x] Verify fix and update TODO.md

### 3. test_unit_core_visual_feedback_face_detector **COMPLETE**
- **Test**: NonAlignedVoxelPositions_DifferentResolutions
- **Issue**: Not detecting 16cm and 256cm voxels at non-aligned positions
- **Fix**: Updated FaceDetector to use adaptive search range based on voxel size and fixed test ray positioning- **Severity**: Medium - may affect multi-resolution support
- [x] Debug why larger resolution voxels aren't detected at non-aligned positions
- [x] Fix detection algorithm for multi-resolution support
- [x] Verify fix and update TODO.md

## Recently Completed (for reference)

- ✓ **test_unit_core_face_detector_traversal** - All 12 tests now passing
- ✓ **test_unit_core_input_placement_validation** - All tests passing
- ✓ **test_unit_core_surface_gen_dual_contouring** - Fixed timeout issues
- ✓ **test_unit_core_surface_gen_generator** - Fixed timeout issues
- ✓ **test_unit_core_surface_gen_requirements** - Fixed validation issues
- ✓ **test_unit_core_undo_redo_voxel_commands** - All tests passing
- ✓ **test_unit_core_visual_feedback_requirements** - All tests passing
- ✓ **test_unit_face_direction_accuracy** - All tests passing

## Build Status

- **CLI Compilation**: ✓ Fixed (API mismatches resolved)
- **Core Libraries**: ✓ Building successfully
- **Foundation**: ✓ Building successfully

## Important Testing Note

**OpenGL Error Handling**: All OpenGL errors now fail with asserts to help find issues faster. This means:
- Any GL error will cause immediate test failure with clear error location
- Tests that previously passed with silent GL errors may now fail
- This is intentional to catch rendering issues early in development
- When fixing tests, check for GL errors in the assertion output

## Next Steps

## Integration Test Status (2025-06-26)

**Integration Test Summary:**
- **Total Tests**: 51
- **Passed**: 44 ✓
- **Failed**: 4 ✗
- **Skipped**: 3
- **Success Rate**: 86%

## Currently Failing Integration Tests (4 total)

### 1. test_integration_cli_ray_visualization **COMPLETE**
- **Tests**: 6 ray visualization tests (RayVisualizationOffByDefault, CanToggleRayVisualization, RayAppearsWhenEnabled, RayFollowsMouseMovement, RayPassesThroughScreenPoint, DebugCommandTogglesRayVisualization)
- **Issue**: Ray visualization test failures with abort trap errors
- **Severity**: Medium - affects visual debugging features
- **Fix**: Fixed OutlineRenderer OpenGL Core Profile compliance issues - added defensive checks for empty batches, fixed VAO/buffer binding issues
- **Status**: ALL 6 TESTS PASSING

### 2. test_integration_feedback_overlay_renderer **COMPLETE**
- **Tests**: 8 tests - TextRendering, TextStyles, PerformanceMetrics, MemoryUsage, GroundPlaneGridBasic, GroundPlaneGridDynamicOpacity, FrameManagement, DifferentScreenSizes 
- **Issue**: GL_INVALID_VALUE (1281) error in flushTextBatch due to buffer overflow
- **Solution**: Added dynamic buffer resizing to handle vertex/index data exceeding initial buffer capacity
- **Severity**: High - core visual feedback feature (RESOLVED)
- **Status**: ALL TESTS PASSING - Fixed GL_INVALID_VALUE errors from buffer size mismatches

### 3. test_integration_feedback_requirements_validation **COMPLETE**
- **Tests**: 17 visual feedback requirements tests (GridSize, GridColor, DynamicOpacity, etc.)
- **Issue**: OverlayRenderer line shader causing GL_INVALID_ENUM errors leading to assertion crashes
- **Severity**: Medium - requirements compliance validation
- **Fix**: Added comprehensive error checking to shader compilation/linking and graceful handling of invalid shaders
- **Status**: ALL 17 TESTS PASSING

### 4. test_integration_grid_overlay **In Progress, Nova**
- **Tests**: Grid overlay functionality tests
- **Issue**: OpenGL Error in drawArrays: GL_INVALID_OPERATION
- **Details**: Debug grid overlay enabled but rendering fails with GL errors
- **Severity**: Medium - visual feedback for placement accuracy
- **Status**: Available for work

### 5. test_integration_placement_validation **COMPLETE**
- **Tests**: PlacementValidationTest.BoundaryValidationConsistency
- **Issue**: VoxelDataManager boundary validation did not account for voxel size - only checked position, not position + voxel height
- **Severity**: High - core placement logic
- **Fix**: Updated VoxelGrid.isValidIncrementPosition() to check if voxel position + voxel size fits within workspace bounds
- **Status**: ALL TESTS PASSING

### 6. test_integration_rendering_opengl_wrapper_validation **COMPLETE**
- **Tests**: OpenGLWrapperValidationTest.TextureManagement
- **Issue**: OpenGL Error in createTexture2D: GL_INVALID_ENUM
- **Details**: Texture creation failing with invalid enum error
- **Severity**: Medium - rendering subsystem validation
- **Fix**: Fixed texture format translation bug in createTextureCube and updateTexture - properly using translateTextureFormat return value for external format
- **Status**: ALL 9 TESTS PASSING

### 7. test_integration_visual_shader_output_validation
- **Tests**: ShaderVisualValidationTest.ShaderErrorVisualization
- **Issue**: Shader compilation assertion failure for intentionally bad shader
- **Details**: Test expects error handling but gets assertion failure instead
- **Severity**: Low - error handling validation
- **Status**: Available for work

## Next Steps

1. ✓ All unit tests are now passing!
2. Fix remaining 7 failing integration tests (down from 13)
3. Run E2E tests to ensure overall system functionality
4. Consider adding more integration tests for multi-resolution voxel support

## Recently Completed Integration Tests (moved to completed status)

- ✅ **test_integration_interaction_click_voxel_placement** - Fixed adjacent voxel placement calculations
- ✅ **test_integration_interaction_face_clicking** - Fixed mixed-size voxel collision issues
- ✅ **test_integration_interaction_voxel_face_clicking** - Fixed placement expectations and overlap issues
- ✅ **test_integration_overlap_detection** - Fixed redundant operation handling
- ✅ **test_integration_preview_accuracy** - Fixed placement logic consistency
- ✅ **test_integration_coordinate_consistency** - Fixed workspace boundary validation
- ✅ **test_integration_face_click_pipeline** - Fixed face clicking placement logic
- ✅ **test_integration_face_detection_boundary** - Fixed ray intersection issues
- ✅ **test_integration_grid_overlay** - Fixed debug grid command implementation
- ✅ **test_integration_cli_smoothing_export** - Fixed STL export validation
- ✅ **test_integration_cli_command_sequences** - Fixed grid alignment validation
- ✅ **test_integration_cli_headless** - Verified test was actually passing

## Recent Progress Update

**Claude's Session Summary (2025-06-27):**
- ✅ Refactored monolithic 1924-line Commands.cpp into modular command system
- ✅ Created 6 command modules: EditCommands, FileCommands, ViewCommands, SelectCommands, SystemCommands, MeshCommands
- ✅ Implemented CommandRegistry with builder pattern for easy command creation
- ✅ Fixed all build errors related to command refactoring
- ⚠️ Introduced 11 unit test failures (4 CLI-related, 7 core functionality)
- 📉 Unit test success rate decreased from 100% to 91% due to refactoring
- 🎯 Successfully modularized command system for better maintainability

**Axel's Session Summary:**
- ✅ Fixed test_integration_face_detection_boundary by extending voxel wall Z range
- ✅ Fixed test_integration_grid_overlay by correcting debug grid command implementation
- ✅ Fixed test_integration_interaction_voxel_face_clicking by resolving overlap issues and updating test expectations for hit-point-based placement
- 📈 Integration test success rate improved from 70.8% to 75%
- 🎯 4 major integration tests completed, reducing failing tests from 13 to 10

**Zara's Session Summary:**
- ✅ Fixed test_integration_interaction_click_voxel_placement by implementing proper voxel size-based adjacent positioning (32-increment offsets for 32cm voxels instead of 1-increment)
- ✅ Fixed test_integration_interaction_face_clicking by resolving voxel collision (moved 32cm voxel from overlapping position to non-overlapping position)  
- ✅ Ran comprehensive integration test suite (51 tests total)
- 📈 Integration test success rate improved from 75% to 80%
- 🎯 Successfully completed 2 high-priority integration tests, reducing failing tests from 9 to 7
- 📋 Updated TODO.md with current integration test status and new failing test details

**Nova's Session Summary:**
- ✅ Discovered smooth command was already fully implemented in Commands.cpp
- ✅ Verified all 19 smoothing command unit tests are passing
- ✅ Updated surface_gen TODO.md to mark smooth command implementation complete
- ✅ Added CLI testing restriction note until integration tests are stable
- 🎯 Phase 5.1 CLI smooth command implementation is 100% complete with comprehensive test coverage- ✅ Fixed test_integration_preview_accuracy by updating placement logic to match MouseInteraction and correcting test expectations for hit-point-based placement
- 📈 Integration test success rate improved from 75% to ~79%
- 🎯 1 integration test completed
