# Voxel Editor

A multi-platform voxel editor with smooth surface generation, supporting desktop and VR (Meta Quest 3).

## Features

- **Multi-resolution voxel system**: 10 size levels (1cm to 512cm)
- **Dynamic workspace**: Adjustable from 2m³ to 8m³
- **Smooth surface generation**: Using Dual Contouring algorithm
- **Group management**: Organize voxels into hierarchical groups
- **Visual feedback**: Green outline previews and face highlighting
- **Command-line interface**: Full-featured CLI with auto-completion
- **Mouse interaction**: Click-to-place voxels with visual feedback
- **File formats**: Custom binary format (.cvef) and STL export

## Project Structure

```
cube_edit/
├── foundation/          # Core utilities (math, events, memory, logging, config)
├── core/               # Main systems (voxels, camera, rendering, etc.)
├── apps/               # Applications
│   ├── cli/           # Command-line interface (complete)
│   ├── desktop/       # Qt desktop app (future)
│   └── vr/           # Meta Quest 3 app (future)
├── external/          # Third-party dependencies
└── build/            # Build output directory
```

## Building

### Prerequisites

- CMake 3.16+
- C++20 compatible compiler
- OpenGL 3.3+
- Git

### Quick Build

```bash
# Clone the repository
git clone <repository-url>
cd cube_edit

# Run the build script
./build.sh

# Or manually:
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

### Build Options

- `BUILD_TESTS` - Build unit tests (default: ON)
- `BUILD_CLI` - Build CLI application (default: ON)
- `BUILD_QT_DESKTOP` - Build Qt desktop app (default: OFF)
- `BUILD_VR` - Build VR application (default: OFF)

## Running the CLI Application

```bash
./build/bin/VoxelEditor_CLI
```

## CLI Commands

### File Operations
- `new` - Create a new project
- `open <filename>` - Open a project file
- `save [filename]` - Save the current project
- `saveas <filename>` - Save with a new name
- `export <filename>` - Export to STL format

### Edit Operations
- `place <x> <y> <z>` - Place a voxel at position
- `delete <x> <y> <z>` - Delete a voxel at position
- `fill <x1> <y1> <z1> <x2> <y2> <z2>` - Fill a box region
- `undo` - Undo last operation
- `redo` - Redo last undone operation
- `resolution <size>` - Set voxel size (1cm, 2cm, 4cm, etc.)
- `workspace <width> <height> <depth>` - Set workspace dimensions

### View Controls
- `camera <preset>` - Set camera view (front/back/left/right/top/bottom/iso/default)
- `zoom <factor>` - Zoom in/out (e.g., 1.5 to zoom in)
- `rotate <x> <y>` - Rotate camera by degrees
- `resetview` - Reset camera to default

### Selection
- `select <x> <y> <z>` - Select a voxel
- `selectbox <x1> <y1> <z1> <x2> <y2> <z2>` - Select box region
- `selectall` - Select all voxels
- `selectnone` - Clear selection

### Groups
- `group <name>` - Create group from selection
- `groups` - List all groups
- `hide <name>` - Hide a group
- `show <name>` - Show a group

### System
- `help [command]` - Show help
- `status` - Show editor status
- `clear` - Clear screen
- `quit` - Exit application

## Mouse Controls

- **Left Click**: Place voxel at cursor
- **Right Click**: Remove voxel at cursor
- **Middle Mouse Drag**: Orbit camera
- **Scroll Wheel**: Zoom in/out
- **Hover**: Preview placement with green outline

## Keyboard Shortcuts

- **Tab**: Auto-complete commands
- **Up/Down Arrows**: Command history
- **Ctrl+C**: Cancel current command
- **1-7**: Quick camera views (when in render window)

## Architecture

The project follows a modular architecture:

### Foundation Layer
- **Math**: Vector/matrix operations, rays, intersections
- **Events**: Type-safe event system
- **Memory**: Object pooling and allocation tracking
- **Logging**: Multi-level logging with profiling
- **Config**: Hierarchical configuration management

### Core Systems
1. **VoxelData**: Multi-resolution sparse voxel storage
2. **Camera**: Orbit camera with preset views
3. **Rendering**: OpenGL-based rendering engine
4. **Input**: Unified input handling for mouse/keyboard/touch/VR
5. **Selection**: Voxel selection with multiple algorithms
6. **Undo/Redo**: Command pattern implementation
7. **Surface Generation**: Dual Contouring for smooth surfaces
8. **Visual Feedback**: Highlighting and preview rendering
9. **Groups**: Hierarchical voxel grouping
10. **FileIO**: Project save/load and STL export

## Testing

Run all tests:
```bash
cd build
ctest --verbose
```

Run specific test suite:
```bash
./build/foundation/math/tests/VoxelEditor_Math_Tests
./build/core/voxel_data/tests/VoxelEditor_VoxelData_Tests
```

## Performance

- Supports up to millions of voxels
- Real-time rendering at 60+ FPS
- Memory-efficient sparse storage
- Optimized for Meta Quest 3 constraints (<4GB memory)

## Future Development

- **Qt Desktop Application**: Full GUI with advanced editing tools
- **VR Application**: Hand tracking interface for Meta Quest 3
- **Additional Features**: 
  - Texture painting
  - Advanced selection tools
  - Animation support
  - Collaborative editing

## License

[License information here]

## Contributing

[Contributing guidelines here]