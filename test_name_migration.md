# Test Name Migration Plan

This document outlines the migration from current test names to the new naming convention: `test_<type>_<category>_<description>`

## Naming Convention Pattern

```
test_<type>_<category>_<description>

Where:
- type: integration, unit, e2e, perf, stress
- category: core, cli, interaction, visual, rendering, shader, feedback, verification
- description: specific test purpose (snake_case)
```

## Integration Test Renames Checklist

### Core Integration Tests
- [ ] `test_camera_cube_visibility` → `test_integration_core_camera_cube_visibility`
- [ ] `test_camera_cube_visibility_simple` → `test_integration_core_camera_cube_visibility_simple`
- [ ] `test_ground_plane_voxel_placement` → `test_integration_core_ground_plane_voxel_placement`
- [ ] `test_workspace_boundary_placement` → `test_integration_core_workspace_boundary_placement`
- [ ] `test_mouse_boundary_clicking` → `test_integration_interaction_mouse_boundary_clicking`
- [ ] `test_mouse_ground_plane_clicking` → `test_integration_interaction_mouse_ground_plane_clicking`

### CLI Integration Tests
- [ ] `VoxelEditor_CLI_Tests` → `test_integration_cli_all_commands` (special case - multi-test suite)
- [ ] `test_click_voxel_placement` → `test_integration_interaction_click_voxel_placement`
- [ ] `test_face_clicking` → `test_integration_interaction_face_clicking`
- [ ] `test_mouse_ray_movement` → `test_integration_interaction_mouse_ray_movement`
- [ ] `test_voxel_face_clicking` → `test_integration_interaction_voxel_face_clicking`
- [ ] `test_voxel_face_clicking_simple` → `test_integration_interaction_voxel_face_clicking_simple`
- [ ] `test_zoom_behavior` → `test_integration_interaction_zoom_behavior`
- [ ] `VoxelEditor_VoxelMeshGenerator_Tests` → `test_integration_cli_voxel_mesh_generator`

### Shader Integration Tests
- [ ] `ShaderTest` → `test_integration_shader_comprehensive_validation`
- [ ] `SimpleShaderTest` → `test_integration_shader_basic_functionality`
- [ ] `RenderIntegrationTest` → `test_integration_rendering_full_pipeline`
- [ ] `GeometricShaderValidation` → `test_integration_shader_geometric_validation`

### Rendering Pipeline Tests
- [ ] `test_enhanced_shader_validation_integration` → `test_integration_rendering_enhanced_shader_validation`
- [ ] `test_shader_visual_validation` → `test_integration_visual_shader_output_validation`

### Visual Feedback Integration Tests
- [ ] `test_feedback_renderer_integration` → `test_integration_feedback_renderer_components`
- [ ] `test_overlay_renderer_integration` → `test_integration_feedback_overlay_renderer`
- [ ] `test_visual_feedback_requirements_integration` → `test_integration_feedback_requirements_validation`

### Verification Tests
- [ ] `CoreFunctionalityTests` → `test_integration_verification_core_functionality`

## CMake File Updates

### 1. Update `tests/integration/CMakeLists.txt`
- [ ] Rename all `add_executable()` calls with new names
- [ ] Update `add_test()` calls with new names
- [ ] Add labels to all tests:
  ```cmake
  set_tests_properties(test_integration_core_camera_cube_visibility PROPERTIES
      LABELS "integration;core;camera"
  )
  ```

### 2. Update `apps/cli/tests/CMakeLists.txt`
- [ ] Rename test executables
- [ ] Update target names
- [ ] Add appropriate labels

### 3. Update `apps/shader_test/CMakeLists.txt`
- [ ] Rename shader test executables
- [ ] Update post-build commands that reference old names

### 4. Update `tests/verification/CMakeLists.txt`
- [ ] Rename CoreFunctionalityTests

## Source File Renames

### Core Integration Tests
- [ ] `test_camera_cube_visibility.cpp` → `test_integration_core_camera_cube_visibility.cpp`
- [ ] `test_camera_cube_visibility_simple.cpp` → `test_integration_core_camera_cube_visibility_simple.cpp`
- [ ] `test_ground_plane_voxel_placement.cpp` → `test_integration_core_ground_plane_voxel_placement.cpp`
- [ ] `test_workspace_boundary_placement.cpp` → `test_integration_core_workspace_boundary_placement.cpp`
- [ ] `test_mouse_boundary_clicking.cpp` → `test_integration_interaction_mouse_boundary_clicking.cpp`
- [ ] `test_mouse_ground_plane_clicking.cpp` → `test_integration_interaction_mouse_ground_plane_clicking.cpp`

### CLI Test Files
- [ ] Keep `integration_test.cpp` as is (part of multi-file test)
- [ ] `test_click_voxel_placement.cpp` → `test_integration_interaction_click_voxel_placement.cpp`
- [ ] `test_face_clicking.cpp` → `test_integration_interaction_face_clicking.cpp`
- [ ] `test_mouse_ray_movement.cpp` → `test_integration_interaction_mouse_ray_movement.cpp`
- [ ] `test_voxel_face_clicking.cpp` → `test_integration_interaction_voxel_face_clicking.cpp`
- [ ] `test_voxel_face_clicking_simple.cpp` → `test_integration_interaction_voxel_face_clicking_simple.cpp`

### Shader Test Files
- [ ] Consider renaming main.cpp files to match executable names

### Visual Feedback Test Files
- [ ] `test_feedback_renderer_integration.cpp` → `test_integration_feedback_renderer_components.cpp`
- [ ] `test_overlay_renderer_integration.cpp` → `test_integration_feedback_overlay_renderer.cpp`
- [ ] `test_visual_feedback_requirements_integration.cpp` → `test_integration_feedback_requirements_validation.cpp`

### Rendering Test Files
- [ ] `test_enhanced_shader_validation_integration.cpp` → `test_integration_rendering_enhanced_shader_validation.cpp`
- [ ] `test_shader_visual_validation.cpp` → `test_integration_visual_shader_output_validation.cpp`

### Verification Test Files
- [ ] `test_core_functionality.cpp` → `test_integration_verification_core_functionality.cpp`

## Script Updates

### 1. Update `run_integration_tests.sh`
- [ ] Implement auto-discovery based on `test_integration_*` pattern
- [ ] Update or remove hard-coded test lists
- [ ] Add dynamic category extraction from test names
- [ ] Update special case handling for multi-test suites
- [ ] Add support for uncategorized tests
- [ ] Update help/list functions to show discovered categories

### 2. Create `run_all_tests.sh` (new)
- [ ] Create unified test runner that can run by type:
  ```bash
  ./run_all_tests.sh integration
  ./run_all_tests.sh unit
  ./run_all_tests.sh all
  ```

### 3. Update CI/CD Scripts
- [ ] Update any CI scripts that reference old test names
- [ ] Update GitHub Actions workflows
- [ ] Update any build scripts

## Documentation Updates

### 1. Update `TODO.md`
- [ ] Update all test names in the integration test tracking section
- [ ] Update test counts and status

### 2. Update `CLAUDE.md`
- [ ] Update any references to test names
- [ ] Add section about test naming convention

### 3. Update `tests.md`
- [ ] Document the new naming convention
- [ ] Update example test names
- [ ] Add migration notes

### 4. Update `README.md` files
- [ ] Update any README files that reference test names
- [ ] Add naming convention guidelines

## Build System Updates

### 1. Update build output directories
- [ ] Consider organizing build output by test type:
  ```
  build_ninja/bin/
  ├── integration/
  ├── unit/
  └── e2e/
  ```

### 2. Update `.gitignore`
- [ ] Add patterns for new test names if needed
- [ ] Ensure test output directories are ignored

## Test Output Updates

### 1. Update test output directories
- [ ] Ensure PPM output files follow new naming
- [ ] Update any hardcoded output paths in tests

### 2. Update log file names
- [ ] Ensure log files match new test names

## Additional Tasks

### 1. Create Migration Script
- [ ] Create `scripts/migrate_test_names.sh` to automate:
  - File renames
  - CMake updates
  - Basic content updates

### 2. Backward Compatibility
- [ ] Consider creating symlinks for old names temporarily
- [ ] Add deprecation warnings in old scripts

### 3. Validation
- [ ] Create validation script to ensure all tests follow convention
- [ ] Add pre-commit hook to validate new test names

### 4. Test Discovery Improvements
- [ ] Implement CTest label-based discovery
- [ ] Add test metadata extraction
- [ ] Create test dependency graphs

### 5. Error Handling
- [ ] Update error messages to use new names
- [ ] Update test failure reports

### 6. Performance
- [ ] Group tests by expected runtime
- [ ] Add timeout configurations based on test type

## Migration Order

1. **Phase 1: Preparation**
   - Create migration scripts
   - Document new convention
   - Create backward compatibility layer

2. **Phase 2: Source Files**
   - Rename .cpp files
   - Update includes
   - Fix any hardcoded paths

3. **Phase 3: Build System**
   - Update CMakeLists.txt files
   - Update build scripts
   - Verify builds work

4. **Phase 4: Scripts**
   - Update run_integration_tests.sh
   - Update CI/CD scripts
   - Create new unified test runner

5. **Phase 5: Documentation**
   - Update all documentation
   - Update TODO tracking
   - Add migration notes

6. **Phase 6: Cleanup**
   - Remove backward compatibility
   - Remove old references
   - Final validation

## Notes

- The `VoxelEditor_CLI_Tests` executable contains multiple test files and should be treated as a special case
- Consider keeping some flexibility for tests that don't fit the pattern perfectly
- The disabled shader tests should remain commented out but could be renamed in comments for consistency
- Some tests appear in multiple categories in the current system (e.g., `test_face_clicking`) - the new naming makes category explicit

## Success Criteria

- [ ] All integration tests follow new naming convention
- [ ] run_integration_tests.sh uses auto-discovery
- [ ] No hardcoded test lists in scripts
- [ ] All tests pass with new names
- [ ] Documentation is updated
- [ ] CI/CD continues to work