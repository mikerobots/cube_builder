# Test Naming Convention Guide

## Naming Pattern

```
test_<type>_<category>_<description>
```

**Types**: `unit`, `uperf`, `integration`, `e2e`, `performance`, `stress`
**Categories**: `foundation`, `core`, `cli`, `interaction`, `visual`, `rendering`, `shader`, `feedback`, `verification`

uperf = unit performance

## Examples

```
test_unit_core_camera_orbit_camera.cpp
test_unit_foundation_math_vector3f.cpp
test_integration_core_camera_cube_visibility.cpp
test_integration_interaction_mouse_ray_movement.cpp
test_uperf_core_rendering_engine.cpp
test_stress_core_rendering_engine.cpp
test_e2e_cli_basic_voxel_placement.sh
```

## Categories

### Foundation Layer
- **`foundation`** - Math, events, memory, logging, configuration subsystems

### Core Layer  
- **`core`** - Camera, VoxelData, Input, Selection, Groups, File I/O, Undo/Redo, Surface Generation, Visual Feedback, Rendering subsystems

### Application Layer
- **`cli`** - Command parsing, CLI workflows, application integration
- **`interaction`** - Mouse clicking, keyboard input, zoom behavior, face detection (formerly misclassified as CLI tests)
- **`visual`** - Visual correctness, screenshot analysis, pixel validation
- **`rendering`** - Rendering pipeline integration tests
- **`shader`** - Shader compilation, validation, and visual output tests
- **`feedback`** - Visual feedback system integration tests
- **`verification`** - Requirements traceability, system-wide validation

## Test Discovery

Tests are automatically discovered by pattern matching:

```bash
# Find all integration tests
find build_ninja/bin -name "test_integration_*" -executable

# Extract category
category=$(echo "test_integration_core_camera" | cut -d'_' -f3)  # "core"

# Run by category
./run_integration_tests.sh core
ctest -L integration
```

## CMake Integration

```cmake
add_executable(test_integration_core_camera_cube_visibility 
    test_integration_core_camera_cube_visibility.cpp)

add_test(NAME test_integration_core_camera_cube_visibility
    COMMAND test_integration_core_camera_cube_visibility)

set_tests_properties(test_integration_core_camera_cube_visibility 
    PROPERTIES LABELS "integration;core;camera")
```

## Migration Examples

| Legacy Name | New Name |
|-------------|----------|
| `test_Camera_requirements.cpp` | `test_unit_core_camera_requirements.cpp` |
| `test_OrbitCamera_transformations.cpp` | `test_unit_core_camera_orbit_transformations.cpp` |
| `test_VoxelGrid.cpp` | `test_unit_core_voxel_data_grid.cpp` |
| `TestDualContouring.cpp` | `test_unit_core_surface_gen_dual_contouring.cpp` |
| `test_integration_cli_face_clicking.cpp` | `test_integration_interaction_face_clicking.cpp` |
| `test_RenderPerformance.cpp` | `test_performance_core_rendering_engine.cpp` |
| `test_RenderStress.cpp` | `test_stress_core_rendering_engine.cpp` |

## Benefits

- **Zero-maintenance discovery** - New tests automatically found
- **Category-based execution** - Run only relevant tests
- **Clear purpose** - Test name indicates what's being tested
- **Flexible CI/CD** - Distribute tests by category across workers

## Migration Status (as of 2025-06-21)

### Completed Subsystems
- ✅ **Camera** - 8 test files migrated
- ✅ **Input** - 10 test files migrated  
- ✅ **Voxel Data** - 2 test files migrated
- ✅ **Surface Generation** - 5 test files migrated
- ✅ **Visual Feedback** - 6 test files migrated
- ✅ **Rendering** - 29 test files migrated
- ✅ **CLI/Interaction** - 6 test files recategorized

### Test Runners Updated
- ✅ `run_integration_tests.sh` - Auto-discovers by pattern
- ✅ `run_all_tests.sh` - Unified runner with full support
- ✅ `run_e2e_tests.sh` - Shell script tests (unaffected)
- ✅ `run_all_unit.sh` - Updated to recognize test_unit_* pattern

### Summary
- **61 test files** successfully migrated
- **7 CMakeLists.txt files** updated
- **4 test runner scripts** verified/updated
- **8 test categories** properly organized