# CLI Application

## Overview

The CLI (Command Line Interface) application is the primary user interface for the Voxel Editor. It provides an interactive terminal with integrated 3D visualization, combining text commands with mouse interaction.

## Current Status

### ✅ What Works
- **Core Editing**: Place, delete, and fill voxels with precise coordinate control
- **Visual Interface**: Real-time 3D rendering with mouse interaction
- **File Operations**: Save/load projects (.cvef) and export to STL
- **Camera Control**: Multiple view presets and free orbit navigation
- **Groups & Selection**: Organize voxels into named groups
- **Undo/Redo**: Full action history support
- **Auto-completion**: Tab completion for commands and files

### ⚠️ What's Documented But Not Working
- **Mesh Smoothing**: `smooth` commands are accepted but have no effect
- **Mesh Validation**: `mesh` commands are not implemented
- **3D Printing Features**: Exports are blocky, not smooth

## Quick Start

```bash
# Build the application
cd /path/to/cube_edit
cmake -B build_ninja -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build_ninja

# Run the CLI
./build_ninja/apps/cli/voxel-cli

# Basic commands
> place 0cm 0cm 0cm      # Place a voxel at origin
> camera iso             # Switch to isometric view
> export model.stl       # Export to STL (will be blocky)
> help                   # List all commands
```

## Architecture

See [DESIGN.md](DESIGN.md) for detailed architecture documentation.

Key components:
- `Application` - Main controller and lifecycle
- `CommandProcessor` - Parses and executes commands
- `MouseInteraction` - Handles 3D mouse input
- `VoxelMeshGenerator` - Converts voxels to renderable geometry

## Important Notes for Developers

### Surface Generation Status
The surface generation subsystem is in transition:
- **Core mesh generation**: ✅ Working (dual contouring)
- **Mesh smoothing**: ❌ Not implemented (MeshSmoother is TODO)
- **Result**: All exports are blocky voxel geometry

### Why Smoothing Commands Don't Work
1. Commands are parsed and accepted by the CLI
2. But `MeshSmoother::smooth()` doesn't exist yet
3. `SurfaceGenerator` returns raw voxel meshes only
4. Settings like `smoothingLevel` are ignored

### File Locations
- Commands: `src/Commands.cpp` (all command handlers)
- Command definitions: `include/cli/CommandTypes.h`
- Export logic: Search for `exportModel` in Commands.cpp
- Mouse logic: `src/MouseInteraction.cpp`

## Testing

```bash
# Run unit tests
./build_ninja/bin/test_unit_cli_*

# Run integration tests  
./run_integration_tests.sh cli

# Run end-to-end tests
cd tests/e2e/cli_validation
./run_all_tests.sh
```

## Known Issues

1. **Smoothing Not Functional**: Despite being documented, smooth commands do nothing
2. **Performance**: Large scenes (>100k voxels) may reduce FPS
3. **Memory**: Mesh generation duplicates voxel data
4. **Platform**: Some mouse interactions vary by OS

## Future Work

See [TODO.md](TODO.md) for the implementation plan, particularly for smoothing features.

Priority tasks:
1. Wait for surface_gen smoothing implementation
2. Hook up smooth commands to new APIs
3. Add mesh validation functionality
4. Implement real-time preview

## Contributing

When adding new commands:
1. Add to `CommandType` enum in CommandTypes.h
2. Implement handler in Commands.cpp
3. Update auto-completion in AutoComplete.cpp
4. Add tests in tests/
5. Update CLI_GUIDE.md

## Support

For issues or questions:
- Check [CLI_GUIDE.md](CLI_GUIDE.md) for usage
- Review [requirements.md](requirements.md) for specifications
- See core library docs for subsystem details