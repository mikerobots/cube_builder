# Voxel Editor Design Document

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

### Build System
- CMake for cross-platform builds
- Dependencies:
  - OpenGL (graphics)
  - Qt6 (desktop GUI)
  - OpenXR SDK (VR interface)
  - Meta Hand Tracking SDK
  - LZ4 (compression)
  - Google Test (unit testing)
- Modular library structure for shared core

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

## Core Components

### 1. Voxel Data Management
- Multi-resolution voxel system: 10 size levels (1cm, 2cm, 4cm, 8cm, 16cm, 32cm, 64cm, 128cm, 256cm, 512cm)
- Workspace size limits: 2m³ minimum, 8m³ maximum, 5m³ default
- Dynamic workspace resizing during editing (within bounds)
- Hierarchical sparse octree data structure (memory efficient)
- Level-of-detail (LOD) management with aggressive culling
- Memory pooling and object reuse
- Streaming for large models with background loading
- Limited undo/redo system (5-10 operations for VR)
- Dynamic resolution switching during editing
- Automatic memory pressure detection and cleanup

### 2. Vertex Selection System
- 3D selection tools
- Multi-vertex operations
- Selection persistence
- Visual feedback

### 3. Voxel Grouping System
- Create groups from selected voxels
- Group operations:
  - Move entire group as unit
  - Hide/show groups
  - Lock groups (prevent editing)
  - Copy/duplicate groups
- Group hierarchy support (nested groups)
- Visual group indicators (color coding, outlines)
- Group management UI (list, rename, delete)
- Group metadata storage in file format

### 4. Surface Generation
- Dual Contouring algorithm (better feature preservation than Marching Cubes)
- Adaptive mesh generation based on voxel resolution
- Multi-resolution surface generation (LOD)
- Real-time preview with simplified mesh
- High-quality export mesh generation
- Sharp edge preservation for architectural details

### 5. Rendering Engine
- OpenGL abstraction layer
- Multiple render targets (window/buffer)
- Solid surface rendering with basic lighting
- Multiple rendering modes:
  - Solid view (default)
  - Wireframe view (toggle)
  - Combined view (solid + transparent wireframe overlay)
- Basic materials and shading
- Depth testing and culling
- Performance profiling hooks

### 6. Camera System
- 3D camera with orbit controls (rotate around center)
- Zoom in/out functionality
- Predefined view shortcuts:
  - Front view
  - Back view
  - Top view
  - Bottom view
  - Left view
  - Right view
  - Isometric view (default)
- Smooth camera transitions between views
- Camera state persistence in project files

### 7. Visual Feedback System
- Face highlighting on hover/point
- Green outline preview of voxel placement location
- Real-time visual cues for:
  - Selected voxels (highlight color)
  - Group boundaries (color-coded outlines)
  - Active resolution level indicator
  - Face normals for placement guidance
- Interactive feedback for all input methods
- Performance-optimized overlay rendering

### 8. File I/O System
- Custom binary format for native storage includes:
  - File header with version and metadata
  - Workspace dimensions and settings
  - Multi-resolution voxel data (all 10 levels)
  - Current active resolution level
  - Camera position and view settings
  - Limited undo history (last 10-20 operations)
  - Vertex selection state
  - Group definitions and metadata
  - Group visibility states
  - Creation/modification timestamps
- STL export functionality for 3D printing/sharing
- Format versioning for backward compatibility
- LZ4 compression for efficient storage

### 9. Input Abstraction
- Mouse/keyboard (desktop): 
  - Click on face to add voxel with green outline preview
  - Mouse wheel for zoom, middle-drag for orbit
  - Number keys 1-7 for view shortcuts (front, back, top, bottom, left, right, iso)
  - Scroll for resolution switching
- Touch/gesture (Qt): 
  - Tap on face to add voxel with green outline preview
  - Pinch for zoom, two-finger drag for orbit
  - Gesture shortcuts for view changes
- Hand tracking (VR): 
  - Point and pinch to place/remove voxels with green outline preview
  - Hand raycast for face highlighting
  - Grab gesture to select and move vertex groups
  - Double grab to create/select groups
  - Two-handed resize for workspace scaling
  - Palm facing down for resolution switching
  - Fist gesture for undo
  - Thumbs up/down for group visibility toggle
  - Hand gestures for view shortcuts
- Unified input event system across platforms
- Real-time face detection and highlighting for all input methods

## Command Line Tool Interface

### Interactive Commands
- `help` - Show all available commands
- `help <command>` - Show specific command help
- `zoom <factor>` - Zoom in/out (e.g., zoom 1.5, zoom 0.8)
- `view <name>` - Switch to predefined view (front, back, top, bottom, left, right, iso)
- `rotate <x> <y> <z>` - Rotate camera by degrees
- `reset` - Reset camera to default position
- `resolution <size>` - Set voxel resolution (1cm, 2cm, 4cm, 8cm, 16cm, 32cm, 64cm, 128cm, 256cm, 512cm)
- `workspace <x> <y> <z>` - Set workspace dimensions
- `render <mode>` - Set render mode (solid, wireframe, combined)
- `save <filename>` - Save project to file
- `load <filename>` - Load project from file
- `export <filename>` - Export to STL
- `group create <name>` - Create group from selection
- `group hide <name>` - Hide group
- `group show <name>` - Show group
- `group list` - List all groups
- `undo` - Undo last operation
- `redo` - Redo last undone operation
- `quit` - Exit application

### Auto-completion
- Tab completion for command names
- Tab completion for filenames
- Tab completion for group names
- Context-sensitive parameter suggestions

### Mouse Interaction
- Left click on face: Add voxel with green outline preview
- Right click on voxel: Remove voxel
- Mouse hover: Show face highlighting
- All other interactions via commands

## Design Complete

All specifications have been defined. Ready for implementation starting with the command line application.
