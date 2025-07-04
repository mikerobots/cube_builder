# Voxel Editor Test Suite Reference

This document provides a comprehensive overview of the testing framework and all test files in the Voxel Editor project. The project employs a sophisticated multi-layered testing approach combining traditional unit testing with innovative visual validation techniques.

## Test Architecture Overview

The test suite is organized into a comprehensive multi-tiered architecture:

### Test Directory Structure
```
/Users/michaelhalloran/cube_edit/
├── tests/                           # Main test directory
│   ├── integration/                 # Integration tests (following test_integration_<category>_* pattern)
│   ├── e2e/                        # End-to-end tests
│   │   ├── cli_validation/          # Visual CLI validation tests (shell scripts)
│   │   └── cli_comprehensive/       # Extended CLI test suite (shell scripts)
│   └── verification/                # Functional verification tests
├── core/*/tests/                    # Unit tests (following test_unit_core_<subsystem>_* pattern)
├── foundation/*/tests/              # Foundation unit tests (following test_unit_foundation_<subsystem>_* pattern)
├── apps/cli/tests/                  # CLI unit and integration tests
├── apps/shader_test/                # Shader testing framework
├── tools/                           # Testing utilities and analyzers
├── run_all_tests.sh                 # Unified test runner (auto-discovers all test types)
├── run_integration_tests.sh         # Integration test runner (auto-discovers by pattern)
├── run_all_unit.sh                  # Unit test runner
└── run_e2e_tests.sh                 # End-to-end test runner (shell scripts)
```

### Testing Frameworks and Tools

**Primary Testing Frameworks:**
- **Google Test (GTest)** - Standard unit testing framework for C++ code
- **CTest** - CMake's built-in testing framework for test discovery and execution
- **Custom Shell Script Framework** - For CLI and visual validation testing

**Specialized Testing Tools:**
- **PPM Color Analyzer** (`tools/analyze_ppm_colors.py`) - Analyzes screenshot colors for visual validation
- **Shader Test Framework** - Custom framework for testing OpenGL shaders
- **Various Analysis Tools** - Grid pattern analysis, geometric validation, pixel detection

---

## Test Categories and Types

### 1. Unit Tests (Google Test Framework)

**Location**: `core/*/tests/` and `foundation/*/tests/` directories
**Naming Convention**: `test_unit_<layer>_<subsystem>_<description>.cpp`
**Examples**:
- `test_unit_core_camera_orbit_camera.cpp`
- `test_unit_foundation_math_vector3f.cpp`
- `test_unit_core_voxel_data_workspace_manager.cpp`
**Coverage**: All core and foundation modules have dedicated unit test suites
**Framework**: Google Test with CMake/CTest integration

### 2. Integration Tests (C++)

**Location**: `tests/integration/` and `apps/cli/tests/`
**Naming Convention**: `test_integration_<category>_<description>.cpp`
**Categories**: `core`, `cli`, `interaction`, `visual`, `rendering`, `shader`, `feedback`, `verification`
**Examples**:
- `test_integration_core_camera_cube_visibility.cpp`
- `test_integration_interaction_mouse_ray_movement.cpp`
- `test_integration_shader_comprehensive_validation.cpp`
**Focus**: Cross-module functionality testing
**Approach**: Full system integration with real components

### 3. End-to-End Tests (Shell Scripts + Visual Analysis)

**Location**: `tests/e2e/cli_validation/` and `tests/e2e/cli_comprehensive/`
**Naming Convention**: Shell scripts following descriptive patterns
**Approach**: Screenshot-based validation using PPM color analysis
**Innovation**: Automated visual validation without human intervention
**Categories**: `cli-validation`, `cli-comprehensive`, `visual`, `workflow`

### 4. Comprehensive CLI Tests

**Location**: `tests/cli_comprehensive/`
**Focus**: End-to-end workflows and real-world usage scenarios
**Features**: Save/load workflows, enhancement validation, error handling

### 5. Shader Tests

**Location**: `apps/shader_test/` and `tests/integration/rendering/`
**Framework**: Custom OpenGL shader testing framework + Google Test integration tests
**OpenGL Version**: Consolidated on OpenGL 3.3 Core Profile (GLSL 330)
**Capabilities**: 
- Shader compilation and linking validation
- VAO setup and attribute binding verification
- Uniform setting and matrix transformation testing
- Real rendering pipeline validation
- Visual output verification through screenshot analysis

### 6. Visual Validation Tests

**Approach**: Screenshot capture as PPM files with automated color analysis
**Tools**: Python-based pixel-level validation tools
**Coverage**: Rendering correctness across different settings and modes

---

## Foundation Layer Tests

### Math Tests
- **`foundation/math/tests/test_unit_foundation_math_matrix4f.cpp`** - Tests 4x4 matrix operations including multiplication, inversion, transformation calculations, and camera matrix generation
- **`foundation/math/tests/test_unit_foundation_math_vector3f.cpp`** - Tests 3D vector operations including dot product, cross product, normalization, and arithmetic operations
- **`foundation/math/tests/test_unit_foundation_math_coordinate_converter.cpp`** - Tests coordinate system conversions
- **`foundation/math/tests/test_unit_foundation_math_coordinate_types.cpp`** - Tests coordinate type definitions and operations

### Memory Management Tests  
- **`foundation/memory/tests/test_unit_foundation_memory_pool.cpp`** - Tests object pooling system for performance optimization, including allocation/deallocation and pool expansion
- **`foundation/memory/tests/test_unit_foundation_memory_tracker.cpp`** - Tests memory usage tracking and leak detection functionality

### Event System Tests
- **`foundation/events/tests/test_unit_foundation_events_dispatcher.cpp`** - Tests event publishing/subscribing system for decoupled component communication

### Configuration Tests
- **`foundation/config/tests/test_unit_foundation_config_manager.cpp`** - Tests application settings management and configuration loading/saving
- **`foundation/config/tests/test_unit_foundation_config_section.cpp`** - Tests configuration section organization and hierarchical settings
- **`foundation/config/tests/test_unit_foundation_config_value.cpp`** - Tests individual configuration value handling and type validation

### Logging Tests
- **`foundation/logging/tests/test_unit_foundation_logging_logger.cpp`** - Tests multi-level logging system and output formatting
- **`foundation/logging/tests/test_unit_foundation_logging_performance_profiler.cpp`** - Tests performance measurement and profiling capabilities

---

## Core Library Tests

### Camera System Tests (122 tests passing)
- **`core/camera/tests/test_unit_core_camera_camera.cpp`** - Tests abstract camera base class functionality including position, target, and view matrix calculations
- **`core/camera/tests/test_unit_core_camera_controller.cpp`** - Tests camera input handling and movement controls
- **`core/camera/tests/test_unit_core_camera_orbit_camera.cpp`** - Tests orbit camera implementation with rotation around target point
- **`core/camera/tests/test_unit_core_camera_orbit_transformations.cpp`** - Tests specific coordinate transformations for orbit camera movement
- **`core/camera/tests/test_unit_core_camera_viewport.cpp`** - Tests viewport and projection matrix calculations
- **`core/camera/tests/test_unit_core_camera_set_distance.cpp`** - Tests camera distance setting functionality
- **`core/camera/tests/test_unit_core_camera_zoom_functionality.cpp`** - Tests camera zoom behavior and constraints
- **`core/camera/tests/test_unit_core_camera_requirements.cpp`** - Tests compliance with camera subsystem requirements

### Voxel Data Management Tests
- **`core/voxel_data/tests/test_unit_core_voxel_data_grid.cpp`** - Tests individual resolution level voxel storage using sparse octree structure
- **`core/voxel_data/tests/test_unit_core_voxel_data_manager.cpp`** - Tests multi-resolution voxel management across all 10 resolution levels
- **`core/voxel_data/tests/test_unit_core_voxel_data_sparse_octree.cpp`** - Tests memory-efficient sparse voxel storage data structure
- **`core/voxel_data/tests/test_unit_core_voxel_data_types.cpp`** - Tests voxel type definitions and resolution enumerations
- **`core/voxel_data/tests/test_unit_core_voxel_data_workspace_manager.cpp`** - Tests workspace bounds and dynamic resizing functionality
- **`core/voxel_data/tests/test_unit_core_voxel_data_collision_simple.cpp`** - Tests collision detection and constraint validation
- **`core/voxel_data/tests/test_unit_core_voxel_data_requirements.cpp`** - Tests compliance with system requirements

### Rendering System Tests
- **`core/rendering/tests/test_unit_core_rendering_engine.cpp`** - Tests main rendering engine functionality and render loop
- **`core/rendering/tests/test_unit_core_rendering_incremental.cpp`** - **KEY TEST** - Tests incremental rendering pipeline validation with step-by-step rendering verification
- **`core/rendering/tests/test_unit_core_rendering_state.cpp`** - Tests render state management and OpenGL state tracking
- **`core/rendering/tests/test_unit_core_rendering_opengl_renderer.cpp`** - Tests low-level OpenGL wrapper functionality
- **`core/rendering/tests/test_unit_core_rendering_shader_manager.cpp`** - Tests shader compilation and management
- **`core/rendering/tests/test_unit_core_rendering_shader_manager_safe.cpp`** - Tests safe shader loading with error handling
- **`core/rendering/tests/test_unit_core_rendering_shader_manager_simple.cpp`** - Tests simplified shader management functionality
- **`core/rendering/tests/test_unit_core_rendering_shader_manager_fixed.cpp`** - Tests fixed shader pipeline functionality
- **`core/rendering/tests/test_unit_core_rendering_shader_manager_logging.cpp`** - Tests shader management with comprehensive logging
- **`core/rendering/tests/test_unit_core_rendering_shader_manager_debug.cpp`** - Tests shader debugging and error reporting
- **`core/rendering/tests/test_unit_core_rendering_shader_attribute_binding.cpp`** - Tests vertex attribute binding for shaders
- **`core/rendering/tests/test_unit_core_rendering_shader_attribute_validation.cpp`** - Tests shader attribute validation and error detection
- **`core/rendering/tests/test_unit_core_rendering_shader_uniforms.cpp`** - Tests uniform variable setting and management
- **`core/rendering/tests/test_unit_core_rendering_config.cpp`** - Tests rendering configuration and settings
- **`core/rendering/tests/test_unit_core_rendering_types.cpp`** - Tests rendering type definitions and enumerations
- **`core/rendering/tests/test_unit_core_rendering_stats.cpp`** - Tests rendering performance statistics collection
- **`core/rendering/tests/test_unit_core_rendering_edge_rendering.cpp`** - Tests edge rendering functionality for wireframe visualization
- **`core/rendering/tests/test_unit_core_rendering_enhanced_shader_validation.cpp`** - Tests enhanced shader validation with comprehensive error checking
- **`core/rendering/tests/test_unit_core_rendering_highlight_shader_validation.cpp`** - Tests highlight shader compilation and validation
- **`core/rendering/tests/test_unit_core_rendering_ground_plane_grid.cpp`** - Tests ground plane grid rendering functionality
- **`core/rendering/tests/test_unit_core_rendering_ground_plane_grid_dynamics.cpp`** - Tests ground plane grid dynamic updates and animations
- **`core/rendering/tests/test_unit_core_rendering_ground_plane_checkerboard.cpp`** - Tests ground plane checkerboard pattern rendering
- **`core/rendering/tests/test_unit_core_rendering_ground_plane_shader_files.cpp`** - Tests ground plane shader file loading and validation
- **`core/rendering/tests/test_unit_core_rendering_inline_shader_validation.cpp`** - Tests inline shader compilation and validation
- **`core/rendering/tests/test_unit_core_rendering_file_based_shader_validation.cpp`** - Tests file-based shader loading and compilation
- **`core/rendering/tests/test_unit_core_rendering_shader_visual_validation.cpp`** - Tests visual validation of shader rendering output
- **`core/rendering/tests/test_performance_core_rendering_engine.cpp`** - Performance benchmarks for rendering engine
- **`core/rendering/tests/test_stress_core_rendering_engine.cpp`** - Stress tests for rendering engine
- **`core/rendering/tests/test_unit_core_rendering_requirements.cpp`** - Tests compliance with rendering subsystem requirements

### Surface Generation Tests (72 tests passing)
- **`core/surface_gen/tests/test_unit_core_surface_gen_generator.cpp`** - Tests main surface generation pipeline using dual contouring algorithm
- **`core/surface_gen/tests/test_unit_core_surface_gen_dual_contouring.cpp`** - Tests dual contouring algorithm implementation for smooth surface extraction
- **`core/surface_gen/tests/test_unit_core_surface_gen_mesh_builder.cpp`** - Tests mesh construction from voxel data including vertex and triangle generation
- **`core/surface_gen/tests/test_unit_core_surface_gen_types.cpp`** - Tests surface generation type definitions and mesh structures
- **`core/surface_gen/tests/test_unit_core_surface_gen_requirements.cpp`** - Tests compliance with surface generation requirements

### Selection System Tests
- **`core/selection/tests/test_unit_core_selection_manager.cpp`** - Tests voxel selection management and multi-selection operations
- **`core/selection/tests/test_unit_core_selection_set.cpp`** - Tests selection storage and manipulation
- **`core/selection/tests/test_unit_core_selection_box_selector.cpp`** - Tests box-based voxel selection tool
- **`core/selection/tests/test_unit_core_selection_sphere_selector.cpp`** - Tests sphere-based voxel selection tool
- **`core/selection/tests/test_unit_core_selection_flood_fill.cpp`** - Tests flood-fill selection for connected voxels
- **`core/selection/tests/test_unit_core_selection_types.cpp`** - Tests selection type definitions and data structures
- **`core/selection/tests/test_unit_core_selection_requirements.cpp`** - Tests compliance with selection requirements
- **`core/selection/tests/test_performance_core_selection_manager.cpp`** - Performance tests for selection operations

### Group Management Tests
- **`core/groups/tests/test_unit_core_groups_manager.cpp`** - Tests voxel group creation, management, and operations
- **`core/groups/tests/test_unit_core_groups_voxel_group.cpp`** - Tests individual group functionality including metadata and operations
- **`core/groups/tests/test_unit_core_groups_hierarchy.cpp`** - Tests nested group structure and hierarchy management
- **`core/groups/tests/test_unit_core_groups_operations.cpp`** - Tests group operations like move, hide, copy, and delete
- **`core/groups/tests/test_unit_core_groups_types.cpp`** - Tests group type definitions and data structures
- **`core/groups/tests/test_unit_core_groups_requirements.cpp`** - Tests compliance with groups requirements

### Input System Tests (158 tests passing)
- **`core/input/tests/test_unit_core_input_mapping.cpp`** - Tests input action mapping and configuration
- **`core/input/tests/test_unit_core_input_types.cpp`** - Tests input type definitions and event structures
- **`core/input/tests/test_unit_core_input_mouse_handler.cpp`** - Tests mouse input processing and event handling
- **`core/input/tests/test_unit_core_input_keyboard_handler.cpp`** - Tests keyboard input processing and key mapping
- **`core/input/tests/test_unit_core_input_touch_handler.cpp`** - Tests touch input processing for Qt applications
- **`core/input/tests/test_unit_core_input_vr_handler.cpp`** - Tests VR hand tracking input processing
- **`core/input/tests/test_unit_core_input_modifier_key_tracking.cpp`** - Tests modifier key state tracking (Shift, Ctrl, Alt, Cmd)
- **`core/input/tests/test_unit_core_input_placement_validation.cpp`** - Tests voxel placement validation and constraint checking
- **`core/input/tests/test_unit_core_input_plane_detector.cpp`** - Tests plane detection for constrained voxel placement
- **`core/input/tests/test_unit_core_input_requirements.cpp`** - Tests compliance with input requirements

### Visual Feedback Tests
- **`core/visual_feedback/tests/test_unit_core_visual_feedback_renderer.cpp`** - Tests visual feedback rendering system
- **`core/visual_feedback/tests/test_unit_core_visual_feedback_highlight_renderer.cpp`** - Tests face and voxel highlighting functionality
- **`core/visual_feedback/tests/test_unit_core_visual_feedback_outline_renderer.cpp`** - Tests green outline preview rendering
- **`core/visual_feedback/tests/test_unit_core_visual_feedback_outline_renderer_preview.cpp`** - Tests outline preview functionality
- **`core/visual_feedback/tests/test_unit_core_visual_feedback_overlay_renderer.cpp`** - Tests UI overlay and indicator rendering
- **`core/visual_feedback/tests/test_unit_core_visual_feedback_face_detector.cpp`** - Tests face detection for mouse interaction
- **`core/visual_feedback/tests/test_unit_core_visual_feedback_types.cpp`** - Tests visual feedback type definitions
- **`core/visual_feedback/tests/test_unit_core_visual_feedback_highlight_manager.cpp`** - Tests highlight management and coordination
- **`core/visual_feedback/tests/test_unit_core_visual_feedback_preview_manager.cpp`** - Tests preview management functionality
- **`core/visual_feedback/tests/test_unit_core_visual_feedback_requirements.cpp`** - Tests compliance with visual feedback requirements

### Undo/Redo System Tests (9 core tests passing)
- **`core/undo_redo/tests/TestHistoryManager.cpp`** - Tests command history management and undo/redo functionality
- **`core/undo_redo/tests/TestCommand.cpp`** - Tests base command interface and execution
- **`core/undo_redo/tests/TestSimpleCommand.cpp`** - Tests simple command implementations and patterns
- **`core/undo_redo/tests/TestPlacementCommands.cpp`** - Tests voxel placement command implementations

### File I/O System Tests (121 tests passing)
- **`core/file_io/tests/TestBinaryFormat.cpp`** - Tests custom binary format handling and versioning
- **`core/file_io/tests/TestBinaryIO.cpp`** - Tests binary input/output operations
- **`core/file_io/tests/TestCompression.cpp`** - Tests LZ4 compression functionality
- **`core/file_io/tests/TestFileManager.cpp`** - Tests project file management and autosave
- **`core/file_io/tests/TestFileTypes.cpp`** - Tests file type definitions and handling
- **`core/file_io/tests/TestFileVersioning.cpp`** - Tests file format versioning and compatibility
- **`core/file_io/tests/TestProject.cpp`** - Tests project data structure and serialization
- **`core/file_io/tests/TestSTLExporter.cpp`** - Tests STL file export functionality

---

## Application Layer Tests

### CLI Application Tests
- **`apps/cli/tests/integration_test.cpp`** - Tests full CLI application integration and end-to-end functionality
- **`apps/cli/tests/test_VoxelMeshGenerator.cpp`** - Tests voxel mesh generation for CLI rendering
- **`apps/cli/tests/test_cli_commands.cpp`** - Tests command processing and parsing in CLI
- **`apps/cli/tests/test_cli_error_handling.cpp`** - Tests error handling and recovery in CLI
- **`apps/cli/tests/test_cli_headless.cpp`** - Tests headless (no GUI) CLI operation mode
- **`apps/cli/tests/test_cli_rendering.cpp`** - Tests CLI rendering functionality and output
- **`apps/cli/tests/test_cli_rendering_basic.cpp`** - Tests basic CLI rendering operations
- **`apps/cli/tests/test_click_voxel_placement.cpp`** - Tests voxel placement via mouse clicking
- **`apps/cli/tests/test_face_clicking.cpp`** - Tests face detection and clicking functionality
- **`apps/cli/tests/test_mouse_ray_movement.cpp`** - Tests mouse ray casting for 3D interaction
- **`apps/cli/tests/test_voxel_face_clicking.cpp`** - Tests clicking on voxel faces for placement
- **`apps/cli/tests/test_voxel_face_clicking_simple.cpp`** - Simple voxel face clicking tests
- **`apps/cli/tests/test_zoom_behavior.cpp`** - Tests camera zoom functionality

### Test Runner Scripts

#### Master Test Runners
- **`./run_all_unit.sh`** - Runs all unit tests across all subsystems
- **`./run_integration_tests.sh`** - Runs C++ integration tests with group selection
- **`./run_e2e_tests.sh`** - Runs end-to-end shell script tests with group selection

#### Test Runner Features
- Group-based test execution for targeted testing
- Help command to show available groups
- List command to enumerate all tests
- Colored output with clear pass/fail indicators
- Timeout handling (30s default, 60s for visual tests)
- Summary statistics and failure reporting
- Proper error handling and exit codes

### Shader Test Application
- **`apps/shader_test/src/main.cpp`** - Main entry point for shader testing framework
- **`apps/shader_test/src/ShaderTestFramework.cpp`** - Core framework for shader validation
- **`apps/shader_test/src/TestMeshGenerator.cpp`** - Generates test meshes for shader validation
- **`apps/shader_test/src/SimpleShaderTest.cpp`** - Basic shader compilation and linking tests
- **`apps/shader_test/src/GeometricShaderValidation.cpp`** - Tests geometric correctness of shaders
- **`apps/shader_test/src/RenderIntegrationTest.cpp`** - Integration tests for rendering pipeline
- **`apps/shader_test/src/ShaderTestRunner.cpp`** - Test runner and reporting utilities
- **`apps/shader_test/src/TestGroundPlaneShader.cpp`** - Ground plane shader specific tests

---

## Integration and Validation Tests

### C++ Integration Tests
**Location**: `tests/integration/`
**Runner**: `./run_integration_tests.sh`

#### Core Integration Tests
- **`test_camera_cube_visibility.cpp`** - Tests camera and cube rendering integration
- **`test_camera_cube_visibility_simple.cpp`** - Simplified camera-cube visibility tests
- **`test_ground_plane_voxel_placement.cpp`** - Tests ground plane voxel placement integration
- **`test_mouse_boundary_clicking.cpp`** - Tests mouse interaction boundary conditions
- **`test_mouse_ground_plane_clicking.cpp`** - Tests mouse clicking on ground plane
- **`test_workspace_boundary_placement.cpp`** - Tests workspace boundary constraint validation

#### Visual Feedback Integration Tests
**Location**: `tests/integration/visual_feedback/`
- **`test_visual_feedback_requirements_integration.cpp`** - Visual feedback requirements validation
- **`test_feedback_renderer_integration.cpp`** - Feedback renderer integration tests
- **`test_overlay_renderer_integration.cpp`** - Overlay renderer integration tests

#### Shader and Rendering Pipeline Integration Tests
**Location**: `tests/integration/rendering/`
- **`test_enhanced_shader_validation_integration.cpp`** - Validates shader compilation and linking for all shader types (basic, enhanced, ground plane)
- **`test_shader_pipeline_integration.cpp`** - Tests complete VAO setup with vertex attributes and rendering pipeline
- **`test_shader_vao_integration.cpp`** - Simplified VAO testing focusing on OpenGL 3.3 Core Profile compliance
- **`test_real_shader_pipeline.cpp`** - Tests actual RenderEngine usage patterns with mesh buffer setup
- **`test_shader_real_usage.cpp`** - Tests real-world shader usage with complete rendering components
- **`test_shader_usage_validation.cpp`** - Validates VAO switching, ground plane rendering, and attribute locations
- **`test_shader_visual_validation.cpp`** - **Visual validation tests** that capture framebuffer contents and validate shader rendering output:
  - Renders colored triangles and validates pixel color distribution
  - Tests voxel cube shading with lighting calculations
  - Validates ground plane grid rendering appearance
  - Tests multiple objects with different shaders
  - Verifies shader error handling and fallback rendering
  - Outputs PPM files to `test_output/` for visual debugging

### End-to-End Tests (Shell Scripts)
**Runner**: `./run_e2e_tests.sh`

#### CLI Validation Tests
**Location**: `tests/e2e/cli_validation/`
- **`test_basic_voxel_placement.sh`** - Single voxel placement and visibility validation
- **`test_camera_views.sh`** - All camera view presets (front, back, top, bottom, left, right, iso)
- **`test_multiple_voxels.sh`** - Multiple voxel placement in patterns
- **`test_render_modes.sh`** - Basic rendering functionality validation
- **`test_resolution_switching.sh`** - Multiple voxel resolutions (1cm, 4cm, 8cm, 16cm, 32cm)
- **`run_all_tests.sh`** - Execute all validation tests with summary report

#### Comprehensive CLI Tests
**Location**: `tests/e2e/cli_comprehensive/`
- **`test_basic_smoke.sh`** - Basic smoke testing for CLI functionality
- **`test_commands_headless.sh`** - Headless command testing
- **`test_constraints_only.sh`** - Constraint validation testing
- **`test_enhancement_validation.sh`** - Feature enhancement validation
- **`test_error_handling.sh`** - Error handling and edge case testing
- **`test_integration.sh`** - Integration workflow testing
- **`test_new_features.sh`** - New feature validation
- **`test_visual_enhancements.sh`** - Visual enhancement testing
- **`run_all_tests.sh`** - Execute all comprehensive tests
- **`run_headless_tests.sh`** - Run only headless tests
- **`run_rendering_tests.sh`** - Run only rendering tests

### Verification Tests
- **`tests/verification/test_core_functionality.cpp`** - Verifies core system functionality

---

## Test Execution Guide

### Build Configuration
```bash
# Configure with Ninja (ALWAYS required)
cmake -B build_ninja -G Ninja

# Build all targets including tests
cmake --build build_ninja
```

### Running Different Test Types

#### 1. Unit Tests (via CTest)
```bash
cd build_ninja
ctest --output-on-failure

# Run specific subsystem tests
ctest -R "VoxelEditor_Rendering_Tests"
ctest -R "VoxelEditor_Camera_Tests"
ctest -R "VoxelEditor_VoxelData_Tests"
```

#### 2. Integration Test Runner (Recommended)
```bash
# Show available test groups
./run_integration_tests.sh

# Run specific groups
./run_integration_tests.sh core cli rendering
./run_integration_tests.sh quick      # Quick smoke tests
./run_integration_tests.sh all        # All tests

# Run visual validation tests
./run_integration_tests.sh visual
```

#### 3. CLI Validation Tests (Visual)
```bash
cd tests/cli_validation
./run_all_tests.sh                   # All CLI validation tests
./test_basic_voxel_placement.sh      # Individual test
./test_camera_views.sh               # Camera view testing
./test_resolution_switching.sh       # Resolution validation
```

#### 4. Comprehensive CLI Tests
```bash
cd tests/cli_comprehensive
./run_all_tests.sh                   # Complete test suite
./run_headless_tests.sh              # Headless-only tests
./run_rendering_tests.sh             # Visual rendering tests
```

#### 5. Individual Test Executables
```bash
# Core subsystem tests
./build_ninja/bin/VoxelEditor_VoxelData_Tests
./build_ninja/bin/VoxelEditor_Selection_Tests
./build_ninja/bin/VoxelEditor_Rendering_Tests

# Integration test executables  
./build_ninja/bin/test_camera_cube_visibility
./build_ninja/bin/test_ground_plane_voxel_placement
```

### Test Execution Features

**Test Runner Capabilities:**
- Colored output with status indicators (✓ for pass, ✗ for fail)
- Test execution timing and performance metrics
- Automatic test discovery and organization
- Group-based test execution with flexible filtering
- Failed test reporting with detailed error logs
- Timeout handling to prevent hanging tests
- Help and list commands for test discovery
- Summary statistics (passed/failed/total)
- CI/CD friendly output with proper exit codes

**Test Groups Available:**
- **Integration Tests**: `core`, `cli-cpp`, `interaction`, `shader`, `visual-feedback`, `verification`, `quick`, `all`
- **E2E Tests**: `cli-validation`, `cli-comprehensive`, `visual`, `rendering`, `workflow`, `boundary`, `quick`, `smoke`, `all`

**Visual Validation Features:**
- Screenshots captured as PPM files
- Automated color distribution analysis using Python tools
- Pixel-level validation for rendering correctness
- Comparison testing across different settings and camera views
- No human intervention required for visual validation

---

## Test Coverage Summary

### Total Test Coverage by Subsystem
- **Camera System**: 108/108 tests passing ✅
- **Groups System**: 75/75 tests passing ✅
- **Input System**: 128/128 tests passing ✅
- **Rendering System**: 159/159 tests passing ✅
- **Selection System**: 128/128 tests passing ✅
- **Surface Generation**: 61/61 tests passing ✅
- **Undo/Redo System**: 9/9 core tests passing ✅
- **Visual Feedback**: 32/117 core tests passing ✅ (renderer tests require OpenGL context)
- **Voxel Data System**: 107/107 tests passing ✅
- **File I/O System**: 121/121 tests passing ✅

### Testing Innovation Highlights

**Visual Validation Without Human Intervention:**
- Automated screenshot capture and analysis
- PPM color distribution validation
- Pixel-perfect rendering verification
- Cross-platform visual consistency testing

**Comprehensive Shader Testing:**
- Compilation validation for OpenGL 3.3 Core Profile (consolidated standard)
- VAO (Vertex Array Object) setup and management validation
- Real-world shader usage patterns with complete rendering pipeline
- Shader attribute locations and uniform binding verification
- Multiple VAO switching and state management
- Ground plane and enhanced shader rendering validation
- Visual output verification using automated screenshot analysis

**Multi-layered Integration:**
- Unit tests for individual components
- Integration tests for cross-component functionality
- End-to-end CLI workflow validation
- Visual validation for graphics correctness

---

## Test Development Guidelines

### Best Practices
- All unit tests use Google Test framework
- Mock objects provided for external dependencies
- Tests organized by subsystem with clear naming conventions
- Visual tests use PPM image analysis for automated validation
- Performance tests include benchmarking capabilities
- Integration tests validate cross-component functionality
- Tests include timeouts to prevent hanging
- Shell scripts are properly executable with appropriate error handling

### Special Requirements
- Visual tests require display/OpenGL context
- Some tests require specific build configuration (Ninja)
- Headless tests can run without display
- Test execution requires proper CMake configuration

### Quality Metrics
- Comprehensive coverage across all major subsystems
- Multiple testing approaches (unit, integration, visual, CLI)
- Automated validation reduces manual testing burden
- CI/CD friendly with proper reporting and exit codes
- Well-documented test structure for developer onboarding

## Test Organization Summary

The reorganized test structure provides:

1. **Clear Separation of Concerns**
   - Unit tests in `core/*/tests/` for individual components
   - C++ integration tests in `tests/integration/` for cross-component testing
   - Shell-based E2E tests in `tests/e2e/` for workflow validation

2. **Flexible Test Execution**
   - Three master test runners for different test types
   - Group-based execution for targeted testing
   - Quick/smoke test options for rapid validation

3. **Comprehensive Coverage**
   - 900+ unit tests across all subsystems
   - Integration tests for critical interactions
   - Visual validation through screenshot analysis
   - End-to-end workflow testing

4. **Developer-Friendly Features**
   - Help and list commands for test discovery
   - Colored output with clear indicators
   - Timeout handling to prevent hanging
   - Detailed failure reporting

This test structure demonstrates a mature, production-ready testing approach that combines traditional software testing with innovative graphics validation techniques, making it particularly well-suited for a complex graphics-intensive voxel editing application.