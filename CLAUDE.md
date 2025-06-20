# CLAUDE.md

## Project Context

This is a **multi-platform voxel editor** with a sophisticated 3-layer architecture:
- **Foundation Layer**: Math, events, memory, logging, configuration utilities
- **Core Library**: 10 major subsystems (voxel data, rendering, camera, input, selection, groups, undo/redo, surface generation, visual feedback, file I/O)  
- **Application Layer**: CLI tool (complete), Qt desktop app (future), VR app (future)

**Read at session start**: DESIGN.md, ARCHITECTURE.md, tests.md

**Track work progress**: Use TODO.md in root folder for high-level progress tracking.

## Build System

**CRITICAL**: USE NINJA FOR ALL BUILDING!

### Configure and Build

**IMPORTANT**: For development and debugging, use Debug builds to maximize compile speed and debuggability. Only use Release/RelWithDebInfo for final performance testing.

```bash
# Debug Build (RECOMMENDED FOR DEVELOPMENT)
# Fastest compilation, best debugging experience, no optimizations
cmake -B build_debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build_debug

# Release Build (ONLY for performance testing)
# Slowest compilation, no debug info, maximum optimizations
cmake -B build_ninja -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build_ninja

# RelWithDebInfo Build (For debugging performance issues)
# Slower compilation, debug symbols with optimizations
cmake -B build_relwithdebinfo -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build_relwithdebinfo
```

## Testing

### Unit Tests (Recommended)
```bash
# Run all unit tests only (excludes integration/e2e tests)
./run_all_unit.sh


### Integration Tests (C++)
```bash
# Integration test runner for C++ cross-component tests
./run_integration_tests.sh all        # All integration tests
./run_integration_tests.sh core       # Core integration tests
./run_integration_tests.sh cli-cpp    # CLI C++ integration tests
./run_integration_tests.sh quick      # Quick integration tests only
```

### End-to-End Tests (Shell)
```bash
# End-to-end test runner for CLI workflow validation
./run_e2e_tests.sh all                # All e2e tests
./run_e2e_tests.sh cli-validation     # Basic CLI validation tests
./run_e2e_tests.sh cli-comprehensive  # Complex CLI workflow tests
./run_e2e_tests.sh visual            # Visual validation tests
./run_e2e_tests.sh quick             # Quick smoke tests only
```

### Comprehensive Testing
```bash
# All unit tests with CTest
cd build_ninja && ctest --output-on-failure

# Run specific test suites directly
cd tests/e2e/cli_validation && ./run_all_tests.sh
cd tests/e2e/cli_comprehensive && ./run_all_tests.sh
```

### Visual Validation Test Suite

**Location**: `tests/e2e/cli_validation/`

Screenshot-based automated test suite using PPM color analysis:
- **`test_basic_voxel_placement.sh`** - Single voxel placement validation
- **`test_camera_views.sh`** - All camera view presets (front/back/top/bottom/left/right/iso)
- **`test_resolution_switching.sh`** - Multiple voxel resolutions (1cm-32cm)
- **`test_multiple_voxels.sh`** - Multiple voxel patterns
- **`test_render_modes.sh`** - Rendering functionality validation

**Key Features**:
- Automated visual validation without human intervention
- PPM color distribution analysis via `tools/analyze_ppm_colors.py`
- Proper failure detection when voxels not visually rendered

## Core Architecture Patterns

### Multi-Resolution Voxel System
- **10 resolution levels**: 1cm, 2cm, 4cm, 8cm, 16cm, 32cm, 64cm, 128cm, 256cm, 512cm
- **Workspace bounds**: 2m³ minimum, 8m³ maximum, 5m³ default
- **Sparse storage**: SparseOctree for memory efficiency
- **Coordinate system**: Centered at origin (0,0,0), Y-up

### Event-Driven Architecture
- Foundation EventDispatcher for decoupled communication
- Type-safe event system across all subsystems
- Event subscription/unsubscription patterns

### Memory Management
- Object pooling via MemoryPool for performance
- Memory pressure detection and cleanup
- VR-optimized (<4GB total memory for Meta Quest 3)

### Testing Architecture
- **895+ unit tests passing** across all subsystems
- **Visual validation** via screenshot analysis
- **Integration tests** for cross-component functionality
- **CLI validation** for end-to-end workflows

## CLI Application

### Commands Structure
Comprehensive CLI with auto-completion - see `apps/cli/CLI_GUIDE.md`

**Key workflow**:
```bash
# Launch CLI
./build_ninja/apps/cli/voxel-cli

# Basic voxel editing
place 0cm 0cm 0cm    # Place voxel at origin
resolution 4cm       # Switch to 4cm voxels
camera front         # Switch camera view
save project.vox     # Save project
```

## Development Workflow

### Coordinate System
**IMPORTANT**: Project uses **centered coordinate system** (origin at 0,0,0). If you see code using old coordinate system (not centered at zero), fix it immediately.

### Command Execution
**CRITICAL**: When running executables in `build_ninja/` or local scripts, ALWAYS use:
```bash
./execute_command.sh ./build_ninja/path/to/executable
./execute_command.sh ./script.sh

**CRITICAL**: When running executables in root directory or local scripts, ALWAYS cd to the root directory first and use ./execute_command.sh

```

### Testing Requirements
- **ALWAYS PUT TIMEOUTS WHEN RUNNING TESTS**
- Visual tests require OpenGL context
- Some tests require Ninja build configuration
- Headless tests can run without display

### Git Workflow
Remote repository available - after committing changes:
```bash
git push -u origin master
```

## Documentation

### Code Documentation
```bash
# Generate Doxygen documentation
doxygen Doxyfile

# View documentation
open docs/html/index.html      # HTML docs
open docs/latex/refman.pdf     # PDF docs
```

### Key Documentation Files
- **ARCHITECTURE.md**: Detailed subsystem architecture
- **DESIGN.md**: Project overview and design patterns  
- **tests.md**: Comprehensive testing framework documentation
- **apps/cli/CLI_GUIDE.md**: CLI command reference


## Development Guidelines

### Code Quality Standards
- All unit tests use Google Test framework
- Follow existing architectural patterns and naming conventions
- Use event system for cross-component communication
- Implement proper error handling and validation
- Mock external dependencies in tests

### Performance Considerations
- Memory-efficient sparse voxel storage required
- Real-time rendering at 60+ FPS target
- VR optimization for <4GB memory constraints
- Use object pooling for frequently allocated objects

### When to Run Different Test Types
- **`./run_all_unit.sh`** - Fast unit tests during development
- **`./run_integration_tests.sh core`** - C++ integration testing after changes
- **`./run_e2e_tests.sh cli-validation`** - Visual validation for rendering changes
- **`./run_e2e_tests.sh cli-comprehensive`** - Complex workflow validation
- **Full test suite** - Before major commits or releases


MOST IMPORTANT!!!!!!: If you are working on a shared todo list captured in a
.md file, please make sure you set the item to be "In progress" in the
file. And update the md file when done.

If you are working on fixing unit tests, there is a good chance that
they were written for not 0,0 being the center, and we will most
likely need to rewrite for the current coordinate system where 0,0 is
the center.

If you are updating requirements documents, don't change the IDs!!!

DON'T BE A YES MAN! CHALLENGE ME IF I AM ASKING YOU TO DO SOMETHING
THAT IS SUBPAR OR DOESN'T MAKE SENSE!
