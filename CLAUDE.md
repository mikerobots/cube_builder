At the beginning of each session, read: DESIGN.md, ARCHITECTURE.md

You will keep track of what you are working on with the TODO.md file
in the root folder.  The TODO.md files will have a list of
everything you have accomplished, and the next thing to do. Keep
the top-level TODO.md file high level.

## Build System

This project uses CMake with Ninja for all builds. **Always use Ninja for building this project.**

### Configure and Build
```bash
# Configure with Ninja (ALWAYS use this)
cmake -B build -G Ninja

# Build with Ninja
cmake --build build

# Or directly with ninja
ninja -C build
```

### Optimized Build Commands
```bash
# Full rebuild with maximum parallelization
cmake --build build --clean-first -j$(sysctl -n hw.ncpu)

# Incremental build (fastest for small changes)
cmake --build build

# Build specific target only
cmake --build build --target VoxelEditor_CLI

# Debug build
cmake -B build_debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build_debug

# Release build with optimizations
cmake -B build_release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build_release
```

## Documentation

This project uses Doxygen for code documentation in `docs/` folder.

```bash
# Generate documentation
doxygen Doxyfile

# View HTML documentation
open docs/html/index.html

# PDF documentation is also generated
open docs/latex/refman.pdf
```

## CLI Commands
Commands can be found apps/cli/CLI_GUIDE.md

## Testing

### CLI Validation Test Suite

**Location**: `tests/cli_validation/`

A comprehensive automated test suite that validates CLI functionality using screenshot analysis:

- **`test_basic_voxel_placement.sh`** - Single voxel placement and visibility validation
- **`test_camera_views.sh`** - All camera view presets (front, back, top, bottom, left, right, iso)
- **`test_resolution_switching.sh`** - Multiple voxel resolutions (1cm, 4cm, 8cm, 16cm, 32cm)
- **`test_multiple_voxels.sh`** - Multiple voxel placement in patterns
- **`test_render_modes.sh`** - Basic rendering functionality validation
- **`run_all_tests.sh`** - Execute all validation tests with summary report

**Usage:**
```bash
cd tests/cli_validation
./run_all_tests.sh  # Run all tests
./test_basic_voxel_placement.sh  # Run individual test
```

**Key Features:**
- Screenshot-based validation using PPM color analysis
- Proper failure detection when voxels are not visually rendered
- Detailed color distribution analysis with `tools/analyze_ppm_colors.py`
- Comprehensive documentation in `tests/cli_validation/README.md`

### Other Test Scripts
- **`integration_test.sh`** - Full integration testing
- **`performance_test.sh`** - Performance benchmarking
- **`validate_project.sh`** - Project structure validation

## Git Repository

This project has a remote repository. After committing changes, remember to push them to the remote:
```
git push -u origin master
```

## Command Execution

When debugging or testing, run commands directly without asking for permission. This includes:
- Building the project
- Running test scripts
- Opening files to view results
- Running debugging commands
- Any other commands needed to solve the problem at hand

