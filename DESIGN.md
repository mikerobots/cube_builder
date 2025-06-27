# Voxel Editor Design Document
*Updated: June 18, 2025*

## Project Overview
A multi-platform voxel editor for building 3D objects with smooth surface generation capabilities. Supports desktop development/testing and VR deployment on Meta Quest 3.

## Core Architecture

### Shared Library (Core Engine)
**Purpose**: Contains all voxel manipulation, rendering, and surface generation logic
**Responsibilities**:
- Voxel data structure management
- Vertex selection and manipulation
- Smooth surface generation algorithms
- OpenGL rendering pipeline
- Image buffer rendering (headless mode)
- Unit testable business logic

### Application Layer
Three distinct applications sharing the core library:

1. **Command Line Tool**
   - Simple OpenGL rendering window
   - Rapid testing and validation
   - Headless rendering capabilities
   - Script-friendly interface

2. **Desktop Qt Application** 
   - Full GUI with menus, toolbars, file management
   - Advanced editing features
   - User-friendly voxel manipulation
   - Real-time preview

3. **VR Application (Meta Quest 3)**
   - Hand tracking integration
   - Immersive 3D editing
   - Spatial interaction model
   - Performance optimized for mobile hardware

## Technical Constraints

### Performance Requirements
- Real-time voxel editing across all 10 resolution levels
- Smooth VR framerates (90+ FPS)
- Memory constraints for Meta Quest 3:
  - Total app memory: <4GB (system reserves ~4GB)
  - Voxel data: <2GB maximum
  - Rendering buffers: <512MB
  - Application overhead: <1GB
- Fast surface generation algorithms with LOD support
- Sub-100ms resolution switching
- Aggressive memory management with sparse data structures

### Platform Constraints
- Cross-platform compatibility (desktop)
- Meta Quest 3 VR compatibility
- OpenGL rendering backend
- Hand tracking library integration

### Technology Stack
- **Build System**: CMake for cross-platform compatibility
- **Graphics**: OpenGL 3.3 Core Profile (consolidated standard)
  - All shaders use GLSL version 330 core
  - No legacy OpenGL 2.1 support
  - Forward-compatible context on macOS
- **Desktop GUI**: Qt6 for modern, native interfaces
- **VR Platform**: OpenXR for future-proof VR support
- **Testing**: Google Test for comprehensive unit testing

*For detailed dependencies and build instructions, see ARCHITECTURE.md*

### Development Constraints
- Single-user editing focus (no collaboration features)
- Testable architecture with Google Test unit tests
- Comprehensive test coverage for core library
- Shared codebase across applications
- Command line validation workflow
- CI/CD integration for automated testing

### Development Priority
1. **Command Line Application** (primary validation platform)
   - Core library development and testing
   - Basic OpenGL rendering window with mouse interaction
   - Interactive command prompt with auto-completion
   - Built-in help system and command documentation
   - Mouse click-to-add voxels in render window
   - All camera/view controls via commands
   - Voxel editing and surface generation validation
   - Performance benchmarking
2. **Desktop Qt Application** (full-featured editor)
3. **VR Application** (final deployment target)

## Core Features and Design Decisions

### Multi-Resolution Voxel System
- **10 resolution levels**: 1cm to 512cm in powers of 2
- **Centered coordinate system**: Origin at (0,0,0), Y-up orientation
- **1cm precision**: All voxels placed at 1cm increment positions
- **Ground plane constraint**: No voxels below Y=0
- **Sparse storage**: Memory-efficient hierarchical data structure
- **Design rationale**: Enables both fine detail work and large-scale construction

### Interaction Design
- **Visual feedback first**: Real-time previews, highlighting, and validation
- **Platform-adaptive input**: Mouse/keyboard, touch, and VR hand tracking
- **Consistent mental model**: Same core operations across all platforms
- **Performance target**: <16ms response time for all interactions

### Surface Generation Strategy
- **Primary Goal**: Transform blocky voxel creations into smooth, organic, printable 3D objects
- **Dual Contouring algorithm**: Provides base mesh generation with topology preservation
- **Progressive smoothing**: User-controllable smoothing levels (0-10+) to eliminate voxel appearance
- **Topology preservation**: Maintains loops, holes, and complex geometry during smoothing
- **Multi-resolution LOD**: Adaptive quality based on view distance
- **Real-time preview**: Interactive smoothing preview during editing
- **Printable output**: Watertight, manifold meshes suitable for 3D printing
- **Design rationale**: Enable children to build with familiar voxel blocks, then transform into smooth toys/objects

### File Format Design
- **Custom binary format**: Optimized for voxel data storage
- **LZ4 compression**: Fast compression with good ratios
- **Version support**: Forward compatibility for format evolution
- **Comprehensive state**: Preserves entire editing session

*For detailed technical implementation of these features, see ARCHITECTURE.md*

## User Experience Design

### Command Line Application (Phase 1)
- **Interactive command prompt**: Primary interface for all operations
- **Auto-completion**: Tab completion for commands, files, and parameters
- **Integrated help system**: Context-sensitive documentation
- **Hybrid interaction**: Commands + mouse for efficient workflow
- **Visual window**: Real-time 3D view with mouse interaction

### Desktop Application (Phase 2)
- **Professional GUI**: Menus, toolbars, and panels
- **Advanced workflows**: Multi-document support
- **Productivity features**: Shortcuts, macros, templates

### VR Application (Phase 3)
- **Natural interactions**: Hand tracking and gestures
- **Spatial editing**: True 3D manipulation
- **Immersive experience**: Designed for Meta Quest 3

*For complete command reference, see apps/cli/CLI_GUIDE.md*

## Design Complete

All specifications have been defined. Ready for implementation starting with the command line application.
