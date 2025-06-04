# Voxel Editor Test Suite Reference

This document provides a comprehensive overview of all test files in the Voxel Editor project, organized by subsystem. Each test file is documented with its purpose and key functionality.

## Test Organization

The test suite is organized into three layers:
- **Foundation Layer**: Core utilities (math, memory, events, config, logging)
- **Core Library**: Main business logic subsystems 
- **Application Layer**: CLI and GUI applications

---

## Foundation Layer Tests

### Math Tests
- **`foundation/math/tests/test_Matrix4f.cpp`** - Tests 4x4 matrix operations including multiplication, inversion, transformation calculations, and camera matrix generation
- **`foundation/math/tests/test_Vector3f.cpp`** - Tests 3D vector operations including dot product, cross product, normalization, and arithmetic operations

### Memory Management Tests  
- **`foundation/memory/tests/test_MemoryPool.cpp`** - Tests object pooling system for performance optimization, including allocation/deallocation and pool expansion
- **`foundation/memory/tests/test_MemoryTracker.cpp`** - Tests memory usage tracking and leak detection functionality

### Event System Tests
- **`foundation/events/tests/test_EventDispatcher.cpp`** - Tests event publishing/subscribing system for decoupled component communication

### Configuration Tests
- **`foundation/config/tests/test_ConfigManager.cpp`** - Tests application settings management and configuration loading/saving
- **`foundation/config/tests/test_ConfigSection.cpp`** - Tests configuration section organization and hierarchical settings
- **`foundation/config/tests/test_ConfigValue.cpp`** - Tests individual configuration value handling and type validation

### Logging Tests
- **`foundation/logging/tests/test_Logger.cpp`** - Tests multi-level logging system and output formatting
- **`foundation/logging/tests/test_PerformanceProfiler.cpp`** - Tests performance measurement and profiling capabilities

---

## Core Library Tests

### Camera System Tests
- **`core/camera/tests/test_Camera.cpp`** - Tests abstract camera base class functionality including position, target, and view matrix calculations
- **`core/camera/tests/test_CameraController.cpp`** - Tests camera input handling and movement controls
- **`core/camera/tests/test_OrbitCamera.cpp`** - Tests orbit camera implementation with rotation around target point
- **`core/camera/tests/test_OrbitCamera_transformations.cpp`** - Tests specific coordinate transformations for orbit camera movement
- **`core/camera/tests/test_Viewport.cpp`** - Tests viewport and projection matrix calculations

### Voxel Data Management Tests
- **`core/voxel_data/tests/test_VoxelGrid.cpp`** - Tests individual resolution level voxel storage using sparse octree structure
- **`core/voxel_data/tests/test_VoxelDataManager.cpp`** - Tests multi-resolution voxel management across all 10 resolution levels
- **`core/voxel_data/tests/test_SparseOctree.cpp`** - Tests memory-efficient sparse voxel storage data structure
- **`core/voxel_data/tests/test_VoxelTypes.cpp`** - Tests voxel type definitions and resolution enumerations
- **`core/voxel_data/tests/test_WorkspaceManager.cpp`** - Tests workspace bounds and dynamic resizing functionality

### Rendering System Tests
- **`core/rendering/tests/test_RenderEngine.cpp`** - Tests main rendering engine functionality and render loop
- **`core/rendering/tests/test_RenderIncremental.cpp`** - **KEY TEST** - Tests incremental rendering pipeline validation with step-by-step rendering verification
- **`core/rendering/tests/test_OpenGLRenderer.cpp`** - Tests low-level OpenGL wrapper functionality
- **`core/rendering/tests/test_ShaderManager.cpp`** - Tests shader compilation and management
- **`core/rendering/tests/test_ShaderManagerSafe.cpp`** - Tests safe shader loading with error handling
- **`core/rendering/tests/test_ShaderManagerSimple.cpp`** - Tests simplified shader management functionality
- **`core/rendering/tests/test_ShaderManagerFixed.cpp`** - Tests fixed shader pipeline functionality
- **`core/rendering/tests/test_ShaderManagerLogging.cpp`** - Tests shader management with comprehensive logging
- **`core/rendering/tests/test_ShaderManagerDebug.cpp`** - Tests shader debugging and error reporting
- **`core/rendering/tests/test_ShaderAttributeBinding.cpp`** - Tests vertex attribute binding for shaders
- **`core/rendering/tests/test_ShaderUniforms.cpp`** - Tests uniform variable setting and management
- **`core/rendering/tests/test_RenderConfig.cpp`** - Tests rendering configuration and settings
- **`core/rendering/tests/test_RenderTypes.cpp`** - Tests rendering type definitions and enumerations
- **`core/rendering/tests/test_RenderStats.cpp`** - Tests rendering performance statistics collection

### Surface Generation Tests
- **`core/surface_gen/tests/TestSurfaceGenerator.cpp`** - Tests main surface generation pipeline using dual contouring algorithm
- **`core/surface_gen/tests/TestDualContouring.cpp`** - Tests dual contouring algorithm implementation for smooth surface extraction
- **`core/surface_gen/tests/TestMeshBuilder.cpp`** - Tests mesh construction from voxel data including vertex and triangle generation
- **`core/surface_gen/tests/TestSurfaceTypes.cpp`** - Tests surface generation type definitions and mesh structures

### Selection System Tests
- **`core/selection/tests/TestSelectionManager.cpp`** - Tests voxel selection management and multi-selection operations
- **`core/selection/tests/TestSelectionSet.cpp`** - Tests selection storage and manipulation
- **`core/selection/tests/TestBoxSelector.cpp`** - Tests box-based voxel selection tool
- **`core/selection/tests/TestSphereSelector.cpp`** - Tests sphere-based voxel selection tool
- **`core/selection/tests/TestFloodFillSelector.cpp`** - Tests flood-fill selection for connected voxels
- **`core/selection/tests/TestSelectionTypes.cpp`** - Tests selection type definitions and data structures

### Group Management Tests
- **`core/groups/tests/TestGroupManager.cpp`** - Tests voxel group creation, management, and operations
- **`core/groups/tests/TestVoxelGroup.cpp`** - Tests individual group functionality including metadata and operations
- **`core/groups/tests/TestGroupHierarchy.cpp`** - Tests nested group structure and hierarchy management
- **`core/groups/tests/TestGroupOperations.cpp`** - Tests group operations like move, hide, copy, and delete
- **`core/groups/tests/TestGroupTypes.cpp`** - Tests group type definitions and data structures

### Input System Tests
- **`core/input/tests/test_InputMapping.cpp`** - Tests input action mapping and configuration
- **`core/input/tests/test_InputTypes.cpp`** - Tests input type definitions and event structures
- **`core/input/tests/test_MouseHandler.cpp`** - Tests mouse input processing and event handling
- **`core/input/tests/test_KeyboardHandler.cpp`** - Tests keyboard input processing and key mapping
- **`core/input/tests/test_TouchHandler.cpp`** - Tests touch input processing for Qt applications
- **`core/input/tests/test_VRInputHandler.cpp`** - Tests VR hand tracking input processing

### Visual Feedback Tests
- **`core/visual_feedback/tests/TestFeedbackRenderer.cpp`** - Tests visual feedback rendering system
- **`core/visual_feedback/tests/TestHighlightRenderer.cpp`** - Tests face and voxel highlighting functionality
- **`core/visual_feedback/tests/TestOutlineRenderer.cpp`** - Tests green outline preview rendering
- **`core/visual_feedback/tests/TestOverlayRenderer.cpp`** - Tests UI overlay and indicator rendering
- **`core/visual_feedback/tests/TestFaceDetector.cpp`** - Tests face detection for mouse interaction
- **`core/visual_feedback/tests/TestFeedbackTypes.cpp`** - Tests visual feedback type definitions

### Undo/Redo System Tests
- **`core/undo_redo/tests/TestHistoryManager.cpp`** - Tests command history management and undo/redo functionality
- **`core/undo_redo/tests/TestCommand.cpp`** - Tests base command interface and execution
- **`core/undo_redo/tests/TestSimpleCommand.cpp`** - Tests simple command implementations and patterns

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

---

## Key Test Categories

### Unit Tests
- Individual component testing in isolation
- Mock objects for dependencies
- Fast execution for development workflow

### Integration Tests  
- Cross-component interaction testing
- End-to-end functionality verification
- CLI integration and command processing

### Rendering Tests
- **Critical for debugging current issues**
- `test_RenderIncremental.cpp` - Step-by-step rendering validation
- Multiple shader manager variants for different approaches
- OpenGL state and uniform testing

### Performance Tests
- Memory usage validation
- Rendering performance benchmarks
- Large dataset handling

### Visual Validation Tests
- Screenshot-based validation (in `tests/cli_validation/`)
- Color analysis and voxel visibility verification
- Camera view and rendering mode testing

---

## Test Execution

### Running Individual Tests
```bash
# Build all tests
cmake --build build --target all

# Run specific test category
ctest -R "VoxelEditor_Rendering_Tests"
ctest -R "VoxelEditor_Camera_Tests"

# Run specific test file
./build/core/rendering/tests/VoxelEditor_Rendering_Tests --gtest_filter="RenderIncrementalTest.*"
```

### Running Validation Tests
```bash
# CLI validation suite
cd tests/cli_validation
./run_all_tests.sh

# Individual validation tests
./test_basic_voxel_placement.sh
./test_camera_views.sh
```

---

## Test Development Notes

- All tests use Google Test framework
- Mock objects provided for external dependencies
- Tests organized by subsystem with clear naming
- Visual tests use PPM image analysis for validation
- Performance tests include benchmarking capabilities
- Integration tests validate cross-component functionality

This test suite provides comprehensive coverage of all major subsystems and serves as both validation and documentation of system behavior.