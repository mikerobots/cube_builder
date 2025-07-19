# CLI Application Requirements

## Overview

The CLI (Command Line Interface) application provides an interactive terminal-based interface for the Voxel Editor, supporting both text commands and mouse interaction in a rendered window. This document defines the functional and non-functional requirements for the CLI subsystem.

## Functional Requirements

### 1. Core Voxel Operations

#### CLI-REQ-001: Voxel Placement
- **Description**: Users shall be able to place voxels at specific positions using coordinates
- **Status**: ✅ Implemented
- **Commands**:
  - `place <x> <y> <z>` - Place voxel at specified coordinates (units required: cm/m)
  - Mouse left-click - Place voxel at mouse cursor position
- **Validation**: Coordinates must be within workspace bounds and valid for current resolution

#### CLI-REQ-002: Voxel Removal
- **Description**: Users shall be able to remove voxels at specific positions
- **Status**: ✅ Implemented
- **Commands**:
  - `delete <x> <y> <z>` - Remove voxel at specified coordinates
  - `remove <x> <y> <z>` - Alias for delete
  - Mouse right-click - Remove voxel at mouse cursor position

#### CLI-REQ-003: Bulk Operations
- **Description**: Users shall be able to fill/clear regions of voxels
- **Status**: ✅ Implemented
- **Commands**:
  - `fill <x1> <y1> <z1> <x2> <y2> <z2>` - Fill box region with voxels
  - `clear` - Remove all voxels
  - `new` - Clear and reset to defaults

### 2. Workspace Management

#### CLI-REQ-004: Workspace Configuration
- **Description**: Users shall be able to configure workspace dimensions
- **Status**: ✅ Implemented
- **Commands**:
  - `workspace <width> <height> <depth>` - Set workspace size in meters
  - `workspace` - Display current workspace dimensions
- **Constraints**: Min 2m³, Max 8m³, Default 5m³

#### CLI-REQ-005: Resolution Control
- **Description**: Users shall be able to change voxel resolution
- **Status**: ✅ Implemented
- **Commands**:
  - `resolution <size>` - Set voxel size (1cm, 2cm, 4cm, 8cm, 16cm, 32cm, 64cm, 128cm, 256cm, 512cm)
  - `resolution` - Display current resolution
- **Note**: Changing resolution clears all voxels

### 3. Camera and Visualization

#### CLI-REQ-006: Camera Control
- **Description**: Users shall be able to control camera view
- **Status**: ✅ Implemented
- **Commands**:
  - `camera <preset>` - Set camera to preset view (front, back, left, right, top, bottom, iso, default)
  - `rotate <horizontal> <vertical>` - Rotate camera by degrees
  - `zoom <factor>` - Set zoom level (0.1 to 10.0)
  - `resetview` - Reset to default view

#### CLI-REQ-007: Rendering Options
- **Description**: Users shall be able to control rendering features
- **Status**: ✅ Implemented
- **Commands**:
  - `grid on/off/toggle` - Control ground plane grid visibility
  - `edges on/off/toggle` - Control voxel edge/wireframe visibility
  - `screenshot <filename>` - Capture current view to file

#### CLI-REQ-008: Debug Visualization
- **Description**: Users shall be able to enable debug visualizations
- **Status**: ✅ Implemented
- **Commands**:
  - `debug ray on/off` - Toggle ray visualization for mouse interaction
  - `debug grid on/off` - Toggle 1cm increment grid overlay

### 4. Selection and Groups

#### CLI-REQ-009: Selection Operations
- **Description**: Users shall be able to select voxels for group operations
- **Status**: ✅ Implemented
- **Commands**:
  - `select <x> <y> <z>` - Select single voxel
  - `selectbox <x1> <y1> <z1> <x2> <y2> <z2>` - Select box region
  - `selectall` - Select all voxels
  - `selectnone` - Clear selection

#### CLI-REQ-010: Group Management
- **Description**: Users shall be able to create and manage voxel groups
- **Status**: ✅ Implemented
- **Commands**:
  - `group <name>` - Create group from selection
  - `groups` - List all groups
  - `hide <name>` - Hide group
  - `show <name>` - Show group
  - `ungroup <name>` - Dissolve group

### 5. File Operations

#### CLI-REQ-011: Project Save/Load
- **Description**: Users shall be able to save and load projects
- **Status**: ✅ Implemented
- **Commands**:
  - `save <filename>` - Save project to .cvef file
  - `open <filename>` - Load project from file
  - `load <filename>` - Alias for open
- **Format**: Custom Voxel Editor Format (.cvef) with LZ4 compression

#### CLI-REQ-012: Export Operations
- **Description**: Users shall be able to export voxel data for external use
- **Status**: ✅ Implemented (Basic STL export only)
- **Commands**:
  - `export <filename.stl>` - Export surface mesh to STL format
- **Note**: Exports raw voxel geometry without smoothing

### 6. Surface Generation (Planned Features)

#### CLI-REQ-013: Mesh Smoothing
- **Description**: Users shall be able to apply smoothing to exported meshes
- **Status**: ❌ Not Implemented (Documented but not functional)
- **Planned Commands**:
  - `smooth` - Show current smoothing level
  - `smooth <0-10>` - Set smoothing level
  - `smooth preview on/off` - Toggle real-time preview
  - `smooth algorithm <type>` - Choose algorithm (laplacian, taubin, bilaplacian)

#### CLI-REQ-014: Mesh Validation
- **Description**: Users shall be able to validate mesh for 3D printing
- **Status**: ❌ Not Implemented (Documented but not functional)
- **Planned Commands**:
  - `mesh validate` - Check if mesh is watertight/manifold
  - `mesh info` - Show mesh statistics
  - `mesh repair` - Fix mesh issues

### 7. User Interface

#### CLI-REQ-015: Command Auto-completion
- **Description**: Users shall have tab-completion for commands and filenames
- **Status**: ✅ Implemented
- **Features**:
  - Tab completes partial commands
  - Tab completes filenames for save/load/export
  - Shows available options when ambiguous

#### CLI-REQ-016: Command History
- **Description**: Users shall be able to navigate command history
- **Status**: ✅ Implemented
- **Features**:
  - Up/Down arrows navigate history
  - History persists during session

#### CLI-REQ-017: Help System
- **Description**: Users shall have access to command help
- **Status**: ✅ Implemented
- **Commands**:
  - `help` - List all commands
  - `help <command>` - Show specific command usage

### 8. Undo/Redo

#### CLI-REQ-018: Action History
- **Description**: Users shall be able to undo/redo actions
- **Status**: ✅ Implemented
- **Commands**:
  - `undo` - Undo last action
  - `redo` - Redo undone action
- **Supported Actions**: Place, delete, fill, group operations

### 9. Mouse Interaction

#### CLI-REQ-019: Mouse Controls
- **Description**: Users shall be able to interact via mouse
- **Status**: ✅ Implemented
- **Features**:
  - Hover shows green outline preview
  - Left-click places voxel
  - Right-click removes voxel
  - Middle-drag rotates camera
  - Scroll wheel zooms
  - Shift+click for 1cm precision placement

### 10. Status and Information

#### CLI-REQ-020: Status Display
- **Description**: Users shall be able to query system status
- **Status**: ✅ Implemented
- **Commands**:
  - `status` - Show voxel count, memory usage, workspace info
  - `stats` - Detailed statistics
  - `build` - Show build version and configuration

## Non-Functional Requirements

### Performance Requirements

#### CLI-NFR-001: Rendering Performance
- **Description**: Maintain interactive frame rates during editing
- **Target**: 60 FPS with up to 10,000 voxels
- **Status**: ✅ Achieved

#### CLI-NFR-002: Command Response Time
- **Description**: Commands shall execute quickly
- **Target**: <100ms for single voxel operations, <1s for bulk operations
- **Status**: ✅ Achieved

#### CLI-NFR-003: Memory Usage
- **Description**: Efficient memory usage for large projects
- **Target**: <4GB RAM for typical projects
- **Status**: ✅ Achieved (using sparse octree storage)

### Usability Requirements

#### CLI-NFR-004: Error Messages
- **Description**: Clear, actionable error messages
- **Status**: ✅ Implemented
- **Examples**: "Error: Coordinates out of bounds", "Error: Invalid resolution"

#### CLI-NFR-005: Visual Feedback
- **Description**: Immediate visual feedback for operations
- **Status**: ✅ Implemented
- **Features**: Green outline preview, immediate voxel display, status messages

### Compatibility Requirements

#### CLI-NFR-006: Platform Support
- **Description**: Cross-platform compatibility
- **Status**: ✅ Implemented
- **Platforms**: Windows, macOS, Linux
- **Dependencies**: OpenGL 3.3, GLFW

#### CLI-NFR-007: File Format Compatibility
- **Description**: STL export shall be compatible with 3D printing software
- **Status**: ✅ Implemented
- **Format**: Binary STL, watertight meshes

## Implementation Status Summary

### Fully Implemented ✅
- All core voxel operations (place, delete, fill)
- Complete camera and view control
- File save/load with compression
- Basic STL export (no smoothing)
- Mouse interaction with preview
- Selection and grouping
- Undo/redo system
- Auto-completion and help
- Debug visualization modes

### Not Implemented ❌
- Mesh smoothing (commands documented but non-functional)
- Mesh validation and repair
- Advanced export options
- Real-time smoothing preview

### Known Limitations
1. STL export produces blocky voxel geometry only
2. No mesh optimization or smoothing available
3. Large workspaces (>100k voxels) may impact performance
4. No texture or color export support

### 11. Mesh Resolution Control (New)

#### CLI-REQ-021: Mesh Resolution Control
- **Description**: Users shall be able to control mesh subdivision resolution for surface generation
- **Status**: 🚧 In Development
- **Commands**:
  - `mesh resolution` - Show current mesh resolution setting
  - `mesh resolution <1cm|2cm|4cm|8cm|16cm>` - Set mesh subdivision resolution
  - `mesh resolution auto` - Use automatic resolution based on quality settings
- **Note**: Affects subdivision detail when smoothing level is 0 (SimpleMesher)

## Future Enhancements

1. **Phase 1**: Complete mesh smoothing implementation
2. **Phase 2**: Add mesh validation and repair
3. **Phase 3**: Support additional export formats (OBJ, PLY)
4. **Phase 4**: Add material/color support
5. **Phase 5**: Implement real-time smoothing preview