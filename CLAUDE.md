At the beginning of each session, read: DESIGN.md, ARCHITECTURE.md



You will keep track of what you are working on with the TODO.md file
in the root folder.  The TODO.md files will have a list of
everything you have accomplished, and the next thing to do. Keep
the top-level TODO.md file high level. Make TODO.md files for each
subsystem as you are working on them. You will keep track of what
you are working on with the TODO.md file.  The TODO.md files will have
a list of everything you have accomplished, and the next thing to do.

## Build System

This project uses CMake with build acceleration tools for faster development:

### Using Ninja (recommended)
```bash
# Configure with Ninja
cmake -B build -G Ninja

# Build with Ninja
cmake --build build

# Or directly with ninja
ninja -C build
```

### Using ccache
ccache is automatically detected by CMake if installed. To verify it's working:
```bash
# Check ccache stats
ccache -s

# Clear ccache if needed
ccache -C
```

### Combined Usage (optimal)
```bash
# Configure once with Ninja
cmake -B build -G Ninja

# All subsequent builds will use both Ninja and ccache
cmake --build build
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

This project uses Doxygen for code documentation. The generated documentation is available in the `docs/` folder:

```bash
# Generate documentation
doxygen Doxyfile

# View HTML documentation
open docs/html/index.html

# PDF documentation is also generated
open docs/latex/refman.pdf
```

The documentation includes:
- Class hierarchies and relationships
- API references for all components
- Architecture diagrams
- Design documents

## Testing

### Automated CLI Test Scripts

Several test scripts are available to automatically test the CLI application:

1. **`test_voxel_simple.sh`** - Simple test that places voxels and quits after 5 seconds
   ```bash
   ./test_voxel_simple.sh
   ```

2. **`test_voxel_placement.sh`** - Creates a more complex voxel structure
   ```bash
   ./test_voxel_placement.sh
   ```

3. **`test_voxel_timed.sh`** - Uses expect for precise timing (falls back to FIFO if expect unavailable)
   ```bash
   ./test_voxel_timed.sh
   ```

Each test script will:
- Launch the voxel-cli application
- Set up a 5x5x5 workspace
- Set resolution to 8cm voxels
- Place voxels in a pattern
- Save the structure to a .vxl file
- Display the window for 5 seconds
- Quit automatically

### Other Test Scripts

- **`test_cli.sh`** - Basic CLI functionality test
- **`integration_test.sh`** - Full integration testing
- **`performance_test.sh`** - Performance benchmarking
- **`validate_project.sh`** - Project structure validation

## Git Repository

This project has a remote repository. After committing changes, remember to push them to the remote:
```
git push -u origin master
```

