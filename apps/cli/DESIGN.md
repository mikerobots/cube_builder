# CLI Application Design Document

## Overview

The CLI application is the primary user interface for the Voxel Editor, providing an interactive command-line interface with integrated 3D visualization. It combines text-based commands with mouse interaction in a rendered window, offering a unique hybrid approach to voxel editing.

## Architecture

### Component Overview

```
┌─────────────────────────────────────────────────────────────┐
│                      CLI Application                        │
├─────────────────────────────────────────────────────────────┤
│  ┌───────────────┐  ┌──────────────┐  ┌─────────────────┐ │
│  │   Application  │  │   Command    │  │     Render      │ │
│  │   Controller   │  │  Processor   │  │     Window      │ │
│  └───────┬───────┘  └──────┬───────┘  └────────┬────────┘ │
│          │                  │                    │          │
│  ┌───────┴───────┐  ┌──────┴───────┐  ┌────────┴────────┐ │
│  │     Mouse     │  │   Command    │  │   Voxel Mesh    │ │
│  │  Interaction  │  │    Types     │  │   Generator     │ │
│  └───────────────┘  └──────────────┘  └─────────────────┘ │
└─────────────────────────────────────────────────────────────┤
                              │                                │
┌─────────────────────────────┴────────────────────────────────┐
│                    Core Library Layer                         │
│  ┌─────────────┐ ┌──────────────┐ ┌───────────────────────┐ │
│  │ VoxelData   │ │  Rendering   │ │  Visual Feedback      │ │
│  │ Manager     │ │  Engine      │ │  (Face Detection)     │ │
│  └─────────────┘ └──────────────┘ └───────────────────────┘ │
│  ┌─────────────┐ ┌──────────────┐ ┌───────────────────────┐ │
│  │ Selection   │ │  Surface     │ │  File I/O             │ │
│  │ Manager     │ │  Generator   │ │  (Save/Load/Export)   │ │
│  └─────────────┘ └──────────────┘ └───────────────────────┘ │
└───────────────────────────────────────────────────────────────┘
```

### Key Components

#### 1. Application Controller (`Application.h/cpp`)
- **Purpose**: Main application lifecycle and coordination
- **Responsibilities**:
  - Initialize OpenGL context and window
  - Coordinate between command processor and renderer
  - Manage application state and configuration
  - Handle main event loop
- **Key Methods**:
  - `run()` - Main application loop
  - `processCommand()` - Route commands to processor
  - `updateAndRender()` - Render frame and handle events

#### 2. Command Processor (`CommandProcessor.h/cpp`)
- **Purpose**: Parse and execute text commands
- **Responsibilities**:
  - Command parsing and validation
  - Delegate to appropriate command handlers
  - Maintain command history
  - Provide command completion hints
- **Key Methods**:
  - `processCommand(string)` - Parse and execute
  - `getCompletions(string)` - Auto-completion
  - `validateCommand()` - Check syntax

#### 3. Command Types (`CommandTypes.h/cpp`)
- **Purpose**: Define all available commands and their parameters
- **Structure**: Enum-based command definitions with parameter specifications
- **Categories**:
  - Voxel operations (place, delete, fill)
  - Camera control (camera, rotate, zoom)
  - File operations (save, load, export)
  - Selection (select, selectbox)
  - Groups (group, hide, show)
  - Rendering (grid, edges, screenshot)

#### 4. Mouse Interaction (`MouseInteraction.h/cpp`)
- **Purpose**: Handle mouse input for voxel editing
- **Responsibilities**:
  - Ray casting from screen to world
  - Face detection for voxel placement
  - Preview visualization
  - Camera orbit control
- **Key Methods**:
  - `handleMouseMove()` - Update hover preview
  - `handleMouseClick()` - Place/remove voxels
  - `updateRay()` - Calculate mouse ray
  - `getPlacementPosition()` - Determine voxel placement

#### 5. Render Window (`RenderWindow.h/cpp`)
- **Purpose**: Manage OpenGL window and rendering
- **Responsibilities**:
  - Window creation and management
  - OpenGL context setup
  - Input event handling
  - Frame buffer management
- **Integration**: Uses GLFW for cross-platform window management

#### 6. Voxel Mesh Generator (`VoxelMeshGenerator.h/cpp`)
- **Purpose**: Convert voxel data to renderable meshes
- **Responsibilities**:
  - Generate cube geometry for each voxel
  - Optimize mesh by merging faces
  - Support multiple voxel resolutions
  - Generate preview outlines
- **Optimization**: Face culling for hidden surfaces

## Data Flow

### Command Execution Flow
```
User Input → Command Line
    ↓
Command Processor (parsing)
    ↓
Command Validation
    ↓
Command Handler (in Commands.cpp)
    ↓
Core Library Call (VoxelDataManager, etc.)
    ↓
State Update
    ↓
Render Update → Visual Feedback
```

### Mouse Interaction Flow
```
Mouse Event → GLFW Callback
    ↓
Mouse Interaction Handler
    ↓
Ray Generation (screen → world)
    ↓
Face Detection (via FaceDetector)
    ↓
Preview Update (green outline)
    ↓
Click → Voxel Placement/Removal
    ↓
Mesh Regeneration → Render
```

## Key Design Decisions

### 1. Hybrid Interface Approach
- **Decision**: Combine CLI with real-time 3D visualization
- **Rationale**: 
  - Precise control via commands
  - Intuitive interaction via mouse
  - Best of both worlds
- **Trade-off**: More complex than pure CLI or GUI

### 2. Immediate Mode Rendering
- **Decision**: Regenerate meshes on voxel changes
- **Rationale**:
  - Simpler implementation
  - Adequate performance for target scale
  - Easier debugging
- **Alternative**: Incremental mesh updates (future optimization)

### 3. Command-Based Architecture
- **Decision**: All operations go through command system
- **Rationale**:
  - Consistent undo/redo support
  - Easy to add new commands
  - Clear separation of concerns
- **Implementation**: Command pattern with parameter validation

### 4. Coordinate System
- **Decision**: Require units (cm/m) for all coordinates
- **Rationale**:
  - Unambiguous positioning
  - Support for precise placement
  - Real-world scale reference
- **Format**: "100cm" or "1m", decimals supported

### 5. Export Pipeline Design
- **Current State**: Direct voxel → STL conversion
- **Design**:
  ```
  VoxelDataManager → SurfaceGenerator → Mesh → STL Exporter
                           ↓
                    (Future: Smoother)
  ```
- **Note**: Smoothing pipeline exists but not implemented

## Mouse Interaction System

### Ray Casting Implementation
1. **Screen to World**: Convert mouse coordinates to world ray
2. **Face Detection**: Use DDA algorithm to find voxel faces
3. **Preview System**: Show green outline at placement position
4. **Precision Mode**: Shift+click for 1cm placement precision

### Camera Control
- **Orbit Camera**: Middle mouse drag for rotation
- **Zoom**: Scroll wheel with logarithmic scaling
- **Constraints**: Prevent camera flipping, maintain up vector

## Rendering Pipeline

### Current Implementation
```
VoxelData → VoxelMeshGenerator → Vertex Buffers
                                        ↓
                                  OpenGL Renderer
                                        ↓
                              Shaders (Basic/Enhanced)
                                        ↓
                                  Frame Buffer
```

### Mesh Generation Strategy
1. **Per-Voxel**: Generate 6 faces per voxel
2. **Face Culling**: Skip faces between adjacent voxels
3. **Batching**: One draw call per resolution level
4. **Preview**: Separate mesh for hover outline

## File Format Integration

### Save/Load (.cvef)
- **Format**: Custom binary with LZ4 compression
- **Contents**: 
  - Header with version/metadata
  - Compressed voxel octree data
  - Group definitions
  - Camera state
- **Integration**: Via FileManager in core

### Export (STL)
- **Current**: Raw voxel geometry export
- **Process**:
  1. Generate surface mesh via SurfaceGenerator
  2. Convert to triangle list
  3. Write binary STL format
- **Limitation**: No smoothing applied

## Surface Generation Integration

### Current State (Working)
- Basic dual contouring algorithm
- Generates blocky voxel-like meshes
- STL export functional but not smooth

### Planned Integration (Not Implemented)
```
Export Command → SurfaceSettings Configuration
                        ↓
                 SurfaceGenerator::generateMesh()
                        ↓
                 MeshSmoother::smooth() [TODO]
                        ↓
                 MeshValidator::validate() [TODO]
                        ↓
                 STLExporter::write()
```

### Why Smoothing Commands Don't Work
1. **UI Exists**: Commands are parsed and accepted
2. **Backend Missing**: MeshSmoother not implemented
3. **Settings Ignored**: smoothingLevel has no effect
4. **Result**: All exports are blocky regardless of smooth setting

## Error Handling

### Command Errors
- **Validation**: Check before execution
- **Feedback**: Clear error messages
- **Recovery**: No state change on error

### Rendering Errors
- **OpenGL**: Check for context loss
- **Fallback**: Basic rendering if shaders fail
- **Logging**: Debug info for troubleshooting

## Performance Considerations

### Optimization Strategies
1. **Sparse Storage**: Only store occupied voxels
2. **Frustum Culling**: Only render visible voxels
3. **Level-of-Detail**: Larger voxels for distant objects
4. **Mesh Caching**: Reuse meshes when possible

### Current Bottlenecks
1. **Mesh Regeneration**: Full rebuild on changes
2. **Large Scenes**: >100k voxels impact FPS
3. **Memory**: Mesh data duplicates voxel data

## Testing Strategy

### Unit Tests
- Command parsing and validation
- Coordinate system constraints
- Mesh generation algorithms
- File format I/O

### Integration Tests
- Command execution flow
- Mouse interaction accuracy
- Render pipeline validation
- Save/load round trips

### End-to-End Tests
- Complete workflows (build house, export)
- Visual validation via screenshots
- Performance benchmarks

## Future Enhancements

### Phase 1: Complete Surface Generation
1. Implement MeshSmoother with 3 algorithms
2. Add MeshValidator for 3D printing checks
3. Enable smooth preview rendering
4. Update export pipeline

### Phase 2: Advanced Features
1. Multiple selection modes
2. Copy/paste operations
3. Transform tools (move, rotate, scale)
4. Layers system

### Phase 3: Performance
1. Incremental mesh updates
2. GPU-based voxel rendering
3. Octree rendering optimizations
4. Multi-threaded mesh generation

### Phase 4: Usability
1. GUI overlay for tools
2. Visual command palette
3. Macro recording
4. Plugin system

## Known Issues and Limitations

1. **Smoothing Not Functional**: Commands exist but do nothing
2. **Large Workspace Performance**: Degrades >100k voxels
3. **No Texture Support**: Only geometry export
4. **Limited Undo Buffer**: Memory constrained
5. **Single Window**: No multi-viewport support

## Conclusion

The CLI application successfully provides a functional voxel editor with a unique hybrid interface. While the current implementation meets core requirements, the planned surface smoothing features remain unimplemented. The architecture is sound and extensible, ready for the completion of the smoothing pipeline that will unlock the full potential for 3D printing workflows.