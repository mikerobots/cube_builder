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

### Rebuild CLI

```bash
# Rebuild CLI only
cd build_ninja && cmake --build . --target VoxelEditor_CLI

# Run CLI
./execute_command.sh ./build_ninja/bin/VoxelEditor_CLI
```

## Testing

### Unified Test Runner (Recommended)

**Test Naming Convention**: All tests follow the pattern `test_<type>_<subsystem>_<component>_<description>`:
- **Types**: unit, integration, e2e, performance, stress, uperf (unit performance tests)
- **Subsystems**: foundation, core, cli, interaction, rendering, shader, feedback, verification
- **Examples**: `test_unit_foundation_memory_pool.cpp`, `test_integration_core_camera_visibility.cpp`, `test_integration_interaction_mouse_ray_movement.cpp`
- **Performance Tests**: Use `test_uperf_` prefix for unit-level performance tests that may take longer to run (e.g., `test_uperf_core_voxel_data_collision.cpp`)

Test needs OpenGL, read ./guides/opengl_integration_test.md

If a test is failing, double check requirements.md to see if it needs to change.

### Test Runners
```bash
# Unit tests only
./run_all_unit.sh

# Integration test runner with auto-discovery
./run_integration_tests.sh <various options>
```

### End-to-End Tests (Shell)
```bash
# End-to-end test runner for CLI workflow validation
./run_e2e_tests.sh <various options>
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
- **Individual test execution** with partial name matching

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
save project.vxl     # Save project
```

## Development Workflow

### Coordinate System
**IMPORTANT**: Project uses **centered coordinate system** (origin at 0,0,0). If you see code using old coordinate system (not centered at zero), fix it immediately.

### Command Execution
**SUPER CRITICAL - ALWAYS USE execute_command.sh**: 
When running ANY executables, scripts, or programs, you MUST ALWAYS use execute_command.sh wrapper:

```bash
# CORRECT - ALWAYS DO THIS:
./execute_command.sh ./build_ninja/path/to/executable
./execute_command.sh ./script.sh
./execute_command.sh ./run_all_unit.sh
./execute_command.sh ./fix_math_types.sh
./execute_command.sh ./cmd.sh
./execute_command.sh ./build_ninja/bin/test_unit_something

# WRONG - NEVER DO THIS:
./script.sh                    # NO! Use execute_command.sh
./run_all_unit.sh             # NO! Use execute_command.sh
chmod +x foo.sh && ./foo.sh   # NO! Use execute_command.sh

# Exception: Basic commands like cd, ls, grep, sed, cmake don't need execute_command.sh
# But ANY script or executable file MUST use execute_command.sh
```

**CRITICAL**: When running executables in root directory or local scripts, ALWAYS cd to the root directory first and use ./execute_command.sh

**When you run bash command, wrap with ./execute_command.sh and run from
the root directory. This doesn't apply to bash commands in scripts you
have written.**

### Testing Requirements
- **ALWAYS PUT TIMEOUTS WHEN RUNNING TESTS**
- Visual tests require OpenGL context
- Some tests require Ninja build configuration
- Headless tests can run without display

### Test Naming Convention
- Integration tests follow the pattern: `test_integration_<category>_<description>`
- Categories include: core, cli, interaction, visual, rendering, shader, feedback, verification
- The `run_integration_tests.sh` script auto-discovers tests by this naming pattern

### Git Workflow
Remote repository available - after committing changes:
```bash
git push -u origin master
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

If you are updating requirements documents, don't change the IDs!

If you are in charge of making a test work, that most likely means the
underlying code is broken, not the test. Sometimes it can be the
test. Challenge all assumptions.

DON'T BE A YES MAN! CHALLENGE ME IF I AM ASKING YOU TO DO SOMETHING
THAT IS SUBPAR OR DOESN'T MAKE SENSE!

**CRITICAL**: Completed subsystem directories are locked. Don't unlock.

DON'T ADD SKIPS TO FIX TESTS!!!
We are in the process of fixing integration tests. DON'T RUN THE CLI!

If I have told you to work on the TODO.md list. Don't request feedback until all items on the list are done! Keep working on the list!