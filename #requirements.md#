# Voxel Editor Enhancement Requirements
*Created: June 18, 2025*

**IMPORTANT: Requirement IDs (e.g., REQ-1.1.1) must remain stable and should NEVER be changed when updating this document. New requirements should be added with new IDs, and obsolete requirements should be marked as deprecated rather than removed or renumbered.**

## Clarified Design Decisions

### Grid and Coordinate System
1. **Grid Origin**: Origin (0,0,0) is at the center of the workspace
   - 5m³ default workspace has coordinates from -2.5m to +2.5m on each axis
2. **Grid Visibility**: Always visible at 35% opacity, brightens to 65% within 2 grid squares of cursor when placing
3. **Negative Y**: Voxels can only be placed at Y ≥ 0 (above ground plane)

### Voxel Placement Precision
1. **1cm Offset Constraint**: Constrained to 1cm increments (0, 1, 2...31cm within each 32cm cell)
2. **Rotation**: No rotation - all voxels are axis-aligned
3. **Default Alignment**: Same-size voxels auto-snap to perfect alignment, Shift key overrides for 1cm increment placement


### Visual Feedback
1. **Grid Transparency**: 35% opacity default, 65% when placing nearby
2. **Grid Colors**: 
   - Standard lines: RGB(180, 180, 180)
   - Major lines (every 160cm): RGB(200, 200, 200) and thicker
3. **Preview Style**: Green outline as specified in DESIGN.md

### Placement Behavior
1. **Plane Persistence**: Placement plane "sticks" to voxel height until preview completely clears
2. **Multiple Heights**: Highest voxel under cursor takes precedence
3. **Placement Validation**: System prevents overlapping placements, shows red preview for invalid positions

### User Interface
1. **Click Behavior**: As per DESIGN.md - Left click places, right click removes
2. **Voxel Size Selection**: Via `resolution <size>` command
3. **Undo/Redo**: Supported with 5-10 operation limit

---

## Requirements

### 1. Ground Plane Grid System

#### 1.1 Grid Specifications
- **REQ-1.1.1**: The ground plane shall display a grid with 32cm x 32cm squares
  - *Subsystems: **Rendering** (grid rendering), **Visual Feedback** (overlay rendering)*
- **REQ-1.1.2**: The grid shall be positioned at Y=0 (ground level)
  - *Subsystems: **Rendering** (coordinate system), **Camera** (view matrices)*
- **REQ-1.1.3**: Grid lines shall use RGB(180, 180, 180) at 35% opacity
  - *Subsystems: **Rendering** (shader/material), **Visual Feedback** (OverlayRenderer)*
- **REQ-1.1.4**: Major grid lines every 160cm shall use RGB(200, 200, 200) and be thicker
  - *Subsystems: **Rendering** (line rendering), **Visual Feedback** (OverlayRenderer)*
- **REQ-1.1.5**: The grid origin (0,0,0) shall be at the center of the workspace
  - *Subsystems: **Voxel Data** (WorkspaceManager), **Rendering** (coordinate transforms)*

#### 1.2 Grid Interaction
- **REQ-1.2.1**: The grid shall be clickable for voxel placement
  - *Subsystems: **Input** (mouse ray casting), **Visual Feedback** (FaceDetector), **Voxel Data** (placement)*
- **REQ-1.2.2**: Grid opacity shall increase to 65% within 2 grid squares of cursor during placement
  - *Subsystems: **Visual Feedback** (dynamic opacity), **Input** (cursor position tracking)*
- **REQ-1.2.3**: The grid shall extend to cover the entire workspace area
  - *Subsystems: **Voxel Data** (workspace bounds), **Rendering** (grid mesh generation)*

### 2. Voxel Placement System

#### 2.1 Placement Precision
- **REQ-2.1.1**: Voxels shall be placeable only at 1cm increment positions
  - *Subsystems: **Voxel Data** (position validation), **Input** (position snapping)*
- **REQ-2.1.2**: Within each 32cm grid cell, there shall be exactly 32 valid positions per axis
  - *Subsystems: **Voxel Data** (coordinate system), **Visual Feedback** (preview snapping)*
- **REQ-2.1.3**: Voxels shall always be axis-aligned (no rotation)
  - *Subsystems: **Voxel Data** (voxel representation), **Rendering** (mesh generation)*
- **REQ-2.1.4**: No voxels shall be placed below Y=0
  - *Subsystems: **Voxel Data** (bounds validation), **Input** (placement validation)*

#### 2.2 Placement on Ground Plane
- **REQ-2.2.1**: When hovering over the ground plane, a green outline preview shall be displayed
  - *Subsystems: **Visual Feedback** (OutlineRenderer), **Input** (hover detection)*
- **REQ-2.2.2**: The preview shall snap to the nearest valid 1cm increment position
  - *Subsystems: **Input** (position calculation), **Visual Feedback** (preview positioning)*
- **REQ-2.2.3**: The preview shall update in real-time as the mouse moves
  - *Subsystems: **Input** (mouse tracking), **Visual Feedback** (preview updates), **Rendering** (frame updates)*
- **REQ-2.2.4**: All voxel sizes (1cm to 512cm) shall be placeable at any valid 1cm increment position on the ground plane
  - *Subsystems: **Voxel Data** (multi-resolution positioning), **Input** (size-independent snapping), **Visual Feedback** (varying size preview)*

#### 2.3 Placement on Existing Voxels
- **REQ-2.3.1**: When hovering over an existing voxel, the face under the cursor shall be highlighted
  - *Subsystems: **Visual Feedback** (FaceDetector, HighlightRenderer), **Input** (ray casting)*
- **REQ-2.3.2**: Face highlighting shall clearly indicate which face is selected
  - *Subsystems: **Visual Feedback** (face rendering), **Rendering** (highlight shaders)*
- **REQ-2.3.3**: Clicking on a highlighted face shall place the new voxel adjacent to that face
  - *Subsystems: **Input** (click handling), **Voxel Data** (adjacent position calculation), **Undo/Redo** (command creation)*

### 3. Voxel Size Relationships

#### 3.1 Same-Size Voxel Placement
- **REQ-3.1.1**: Same-size voxels shall auto-snap to perfect alignment by default
  - *Subsystems: **Input** (snap logic), **Voxel Data** (alignment calculation)*
- **REQ-3.1.2**: Holding Shift shall allow placement at any valid 1cm increment
  - *Subsystems: **Input** (modifier key handling), **Visual Feedback** (preview behavior)*
- **REQ-3.1.3**: Aligned placement means edges match perfectly with the target face
  - *Subsystems: **Voxel Data** (position calculation), **Visual Feedback** (preview alignment)*

#### 3.2 Smaller Voxel on Larger Voxel
- **REQ-3.2.1**: A green outline preview shall show exact placement position
  - *Subsystems: **Visual Feedback** (OutlineRenderer), **Voxel Data** (size relationships)*
- **REQ-3.2.2**: Placement shall respect 1cm increment positions on the target face
  - *Subsystems: **Voxel Data** (sub-grid positioning), **Input** (position calculation)*
- **REQ-3.2.3**: The preview shall snap to nearest valid position
  - *Subsystems: **Input** (snap calculation), **Visual Feedback** (preview positioning)*

#### 3.3 Larger Voxel on Smaller Voxel
- **REQ-3.3.1**: Placement plane shall snap to the smaller voxel's face
  - *Subsystems: **Input** (plane detection), **Visual Feedback** (plane visualization)*
- **REQ-3.3.2**: Placement plane shall maintain height while preview overlaps any voxel at current height
  - *Subsystems: **Input** (plane persistence logic), **Voxel Data** (collision detection)*
- **REQ-3.3.3**: When multiple voxels at different heights are under cursor, highest takes precedence
  - *Subsystems: **Input** (height sorting), **Voxel Data** (spatial queries)*
- **REQ-3.3.4**: Plane only changes when preview completely clears current height voxels
  - *Subsystems: **Input** (overlap detection), **Visual Feedback** (plane updates)*

### 4. Visual Feedback System

#### 4.1 Preview Rendering
- **REQ-4.1.1**: All placement previews shall use green outline rendering
  - *Subsystems: **Visual Feedback** (OutlineRenderer), **Rendering** (shader support)*
- **REQ-4.1.2**: Invalid placements shall show red outline preview
  - *Subsystems: **Visual Feedback** (color switching), **Voxel Data** (validation)*
- **REQ-4.1.3**: Preview updates shall be smooth and responsive (< 16ms)
  - *Subsystems: **Visual Feedback** (performance), **Rendering** (frame timing), **Input** (event handling)*

#### 4.2 Face Highlighting
- **REQ-4.2.1**: Face highlighting shall use yellow color (as per DESIGN.md)
  - *Subsystems: **Visual Feedback** (HighlightRenderer), **Rendering** (color management)*
- **REQ-4.2.2**: Only one face shall be highlighted at a time
  - *Subsystems: **Visual Feedback** (highlight state), **Input** (face selection)*
- **REQ-4.2.3**: Highlighting shall be visible from all camera angles
  - *Subsystems: **Visual Feedback** (depth testing), **Camera** (view independence)*

#### 4.3 Placement Validation
- **REQ-4.3.1**: System shall prevent overlapping voxel placements
  - *Subsystems: **Voxel Data** (collision detection), **Input** (validation logic)*
- **REQ-4.3.2**: Invalid placement attempts shall show red preview
  - *Subsystems: **Visual Feedback** (invalid state rendering), **Voxel Data** (validation results)*
- **REQ-4.3.3**: Valid placements shall show green preview
  - *Subsystems: **Visual Feedback** (valid state rendering), **Voxel Data** (validation results)*

### 5. Interaction Model

#### 5.1 Mouse Controls
- **REQ-5.1.1**: Left-click shall place a voxel at the current preview position
  - *Subsystems: **Input** (click handling), **Voxel Data** (voxel creation), **Undo/Redo** (command creation)*
- **REQ-5.1.2**: Right-click on a voxel shall remove that voxel
  - *Subsystems: **Input** (click handling), **Voxel Data** (voxel removal), **Undo/Redo** (command creation)*
- **REQ-5.1.3**: Mouse movement shall update preview position in real-time
  - *Subsystems: **Input** (mouse tracking), **Visual Feedback** (preview updates)*
- **REQ-5.1.4**: Ray-casting shall determine face/position under cursor
  - *Subsystems: **Input** (ray generation), **Visual Feedback** (FaceDetector), **Camera** (ray transformation)*

#### 5.2 Placement Rules
- **REQ-5.2.1**: Voxels shall not overlap with existing voxels
  - *Subsystems: **Voxel Data** (collision detection), **Input** (validation)*
- **REQ-5.2.2**: System shall validate placement before allowing it
  - *Subsystems: **Voxel Data** (validation logic), **Input** (pre-placement checks)*
- **REQ-5.2.3**: Only positions with Y ≥ 0 shall be valid
  - *Subsystems: **Voxel Data** (bounds checking), **Input** (position validation)*

#### 5.3 Resolution Selection
- **REQ-5.3.1**: Current voxel size controlled by active resolution setting
  - *Subsystems: **Voxel Data** (active resolution), **Input** (resolution state)*
- **REQ-5.3.2**: Resolution changed via `resolution <size>` command
  - *Subsystems: **Input** (command processing), **Voxel Data** (resolution switching)*
- **REQ-5.3.3**: Available resolutions: 1cm, 2cm, 4cm, 8cm, 16cm, 32cm, 64cm, 128cm, 256cm, 512cm
  - *Subsystems: **Voxel Data** (resolution definitions)*

#### 5.4 Modifier Keys
- **REQ-5.4.1**: Shift key shall override auto-snap for same-size voxels
  - *Subsystems: **Input** (modifier key state), **Visual Feedback** (snap behavior)*
- **REQ-5.4.2**: No rotation controls (voxels always axis-aligned)
  - *Subsystems: **Voxel Data** (no rotation support), **Input** (no rotation controls)*

### 6. Performance Requirements

#### 6.1 Rendering Performance
- **REQ-6.1.1**: Grid rendering shall maintain 60 FPS minimum (90+ FPS for VR)
  - *Subsystems: **Rendering** (performance optimization), **Visual Feedback** (efficient grid rendering)*
- **REQ-6.1.2**: Preview updates shall complete within 16ms
  - *Subsystems: **Visual Feedback** (update performance), **Input** (event processing), **Rendering** (frame timing)*
- **REQ-6.1.3**: Face highlighting shall update within one frame
  - *Subsystems: **Visual Feedback** (immediate updates), **Input** (low-latency processing)*
- **REQ-6.1.4**: Resolution switching shall complete within 100ms
  - *Subsystems: **Voxel Data** (resolution management), **Visual Feedback** (preview updates)*

#### 6.2 Scalability
- **REQ-6.2.1**: System shall handle 10,000+ voxels without degradation
  - *Subsystems: **Voxel Data** (sparse storage), **Rendering** (LOD/culling), **Visual Feedback** (instancing)*
- **REQ-6.2.2**: Grid size shall scale with workspace (up to 8m x 8m)
  - *Subsystems: **Voxel Data** (workspace bounds), **Rendering** (dynamic grid generation), **Visual Feedback** (grid scaling)*

#### 6.3 Memory Management (VR Constraints)
- **REQ-6.3.1**: Total application memory shall not exceed 4GB (Meta Quest 3 constraint)
  - *Subsystems: **Voxel Data** (memory monitoring), **Rendering** (buffer management), **Surface Generation** (mesh optimization)*
- **REQ-6.3.2**: Voxel data storage shall not exceed 2GB
  - *Subsystems: **Voxel Data** (sparse storage optimization), **Groups** (efficient group storage)*
- **REQ-6.3.3**: Rendering buffers shall not exceed 512MB
  - *Subsystems: **Rendering** (buffer management), **Visual Feedback** (texture optimization)*
- **REQ-6.3.4**: Application overhead shall not exceed 1GB
  - *Subsystems: **Foundation** (memory pools), **Undo/Redo** (limited history), **File I/O** (streaming)*
- **REQ-6.3.5**: System shall detect and respond to memory pressure
  - *Subsystems: **Foundation** (memory monitoring), **Voxel Data** (cleanup triggers)*

### 7. Technical Architecture Requirements

#### 7.1 Platform Support
- **REQ-7.1.1**: System shall support cross-platform desktop compatibility
  - *Subsystems: **Foundation** (platform abstraction), **Rendering** (OpenGL compatibility)*
- **REQ-7.1.2**: System shall support Meta Quest 3 VR platform
  - *Subsystems: **Input** (VR input handling), **Rendering** (VR optimizations)*
- **REQ-7.1.3**: System shall use OpenGL 3.3+ core profile minimum
  - *Subsystems: **Rendering** (OpenGL abstraction), **Visual Feedback** (shader compatibility)*

#### 7.2 Build System
- **REQ-7.2.1**: System shall use CMake for cross-platform builds
  - *Subsystems: **Build System** (CMake configuration)*
- **REQ-7.2.2**: System shall use Ninja as the primary build generator
  - *Subsystems: **Build System** (Ninja integration)*
- **REQ-7.2.3**: System shall support modular library structure for shared core
  - *Subsystems: **Core Library** (modular design), **Applications** (shared library usage)*

#### 7.3 Dependencies
- **REQ-7.3.1**: System shall use Qt6 for desktop GUI application
  - *Subsystems: **Applications** (Qt integration), **Input** (Qt touch handling)*
- **REQ-7.3.2**: System shall use OpenXR SDK for VR interface
  - *Subsystems: **Input** (VR input), **Rendering** (VR rendering)*
- **REQ-7.3.3**: System shall use Meta Hand Tracking SDK for hand tracking
  - *Subsystems: **Input** (hand tracking), **VR Application** (gesture recognition)*
- **REQ-7.3.4**: System shall use LZ4 compression for file storage
  - *Subsystems: **File I/O** (compression), **Groups** (metadata compression)*
- **REQ-7.3.5**: System shall use Google Test framework for unit testing
  - *Subsystems: **Testing** (unit tests), **All Subsystems** (test coverage)*

### 8. File Format and Storage Requirements

#### 8.1 Binary Format Specification
- **REQ-8.1.1**: Custom binary format shall include file header with version and metadata
  - *Subsystems: **File I/O** (binary format), **File Versioning** (compatibility)*
- **REQ-8.1.2**: Format shall store workspace dimensions and settings
  - *Subsystems: **File I/O** (workspace storage), **Voxel Data** (workspace management)*
- **REQ-8.1.3**: Format shall store multi-resolution voxel data for all 10 levels
  - *Subsystems: **File I/O** (voxel serialization), **Voxel Data** (multi-resolution)*
- **REQ-8.1.4**: Format shall store current active resolution level
  - *Subsystems: **File I/O** (state storage), **Voxel Data** (resolution state)*
- **REQ-8.1.5**: Format shall store camera position and view settings
  - *Subsystems: **File I/O** (camera storage), **Camera** (state persistence)*
- **REQ-8.1.6**: Format shall store limited undo history (10-20 operations)
  - *Subsystems: **File I/O** (history storage), **Undo/Redo** (history persistence)*
- **REQ-8.1.7**: Format shall store vertex selection state
  - *Subsystems: **File I/O** (selection storage), **Selection** (state persistence)*
- **REQ-8.1.8**: Format shall store group definitions and metadata
  - *Subsystems: **File I/O** (group storage), **Groups** (metadata persistence)*
- **REQ-8.1.9**: Format shall store group visibility states
  - *Subsystems: **File I/O** (visibility storage), **Groups** (state management)*
- **REQ-8.1.10**: Format shall include creation and modification timestamps
  - *Subsystems: **File I/O** (timestamp management)*

#### 8.2 Export and Compression
- **REQ-8.2.1**: System shall export STL files for 3D printing and sharing
  - *Subsystems: **File I/O** (STL export), **Surface Generation** (mesh export)*
- **REQ-8.2.2**: System shall support format versioning for backward compatibility
  - *Subsystems: **File I/O** (versioning), **File Versioning** (compatibility)*
- **REQ-8.2.3**: System shall use LZ4 compression for efficient storage
  - *Subsystems: **File I/O** (compression), **Compression** (LZ4 implementation)*

### 9. Command Line Interface Requirements

#### 9.1 Interactive Commands
- **REQ-9.1.1**: CLI shall provide interactive command prompt with auto-completion
  - *Subsystems: **CLI Application** (command processor), **Input** (command parsing)*
- **REQ-9.1.2**: CLI shall provide built-in help system and command documentation
  - *Subsystems: **CLI Application** (help system)*
- **REQ-9.1.3**: CLI shall support tab completion for command names, filenames, and group names
  - *Subsystems: **CLI Application** (auto-completion)*
- **REQ-9.1.4**: CLI shall provide context-sensitive parameter suggestions
  - *Subsystems: **CLI Application** (parameter completion)*

#### 9.2 Core Commands
- **REQ-9.2.1**: CLI shall support help commands (help, help <command>)
  - *Subsystems: **CLI Application** (help system)*
- **REQ-9.2.2**: CLI shall support camera commands (zoom, view, rotate, reset)
  - *Subsystems: **CLI Application** (command processing), **Camera** (view control)*
- **REQ-9.2.3**: CLI shall support voxel commands (resolution, workspace, render)
  - *Subsystems: **CLI Application** (command processing), **Voxel Data** (resolution control)*
- **REQ-9.2.4**: CLI shall support file commands (save, load, export)
  - *Subsystems: **CLI Application** (command processing), **File I/O** (file operations)*
- **REQ-9.2.5**: CLI shall support group commands (group create/hide/show/list)
  - *Subsystems: **CLI Application** (command processing), **Groups** (group operations)*
- **REQ-9.2.6**: CLI shall support undo/redo commands
  - *Subsystems: **CLI Application** (command processing), **Undo/Redo** (history navigation)*

### 10. Surface Generation Requirements

#### 10.1 Algorithm Requirements
- **REQ-10.1.1**: System shall use Dual Contouring algorithm for surface generation
  - *Subsystems: **Surface Generation** (dual contouring implementation)*
- **REQ-10.1.2**: Algorithm shall provide better feature preservation than Marching Cubes
  - *Subsystems: **Surface Generation** (algorithm quality)*
- **REQ-10.1.3**: System shall support adaptive mesh generation based on voxel resolution
  - *Subsystems: **Surface Generation** (adaptive meshing)*
- **REQ-10.1.4**: System shall support multi-resolution surface generation (LOD)
  - *Subsystems: **Surface Generation** (LOD management)*
- **REQ-10.1.5**: System shall provide real-time preview with simplified mesh
  - *Subsystems: **Surface Generation** (preview generation), **Rendering** (preview rendering)*
- **REQ-10.1.6**: System shall generate high-quality export meshes
  - *Subsystems: **Surface Generation** (export quality)*
- **REQ-10.1.7**: System shall preserve sharp edges for architectural details
  - *Subsystems: **Surface Generation** (edge preservation)*

### 11. Testing and Validation Requirements

#### 11.1 Unit Testing
- **REQ-11.1.1**: System shall maintain 895+ unit tests across all subsystems
  - *Subsystems: **All Subsystems** (unit test coverage)*
- **REQ-11.1.2**: System shall use Google Test framework for all unit tests
  - *Subsystems: **Testing Framework** (Google Test usage)*
- **REQ-11.1.3**: All tests shall include proper timeout constraints
  - *Subsystems: **Testing Framework** (timeout management)*

#### 11.2 Integration Testing
- **REQ-11.2.1**: System shall provide visual validation via screenshot analysis
  - *Subsystems: **Testing Framework** (visual validation), **Rendering** (screenshot capture)*
- **REQ-11.2.2**: System shall provide integration tests for cross-component functionality
  - *Subsystems: **Testing Framework** (integration tests)*
- **REQ-11.2.3**: System shall provide CLI validation for end-to-end workflows
  - *Subsystems: **Testing Framework** (CLI validation), **CLI Application** (test interface)*

#### 11.3 Performance Testing
- **REQ-11.3.1**: System shall include performance benchmarking in tests
  - *Subsystems: **Testing Framework** (performance tests), **All Subsystems** (benchmarks)*
- **REQ-11.3.2**: System shall support headless test execution
  - *Subsystems: **Testing Framework** (headless mode), **Rendering** (headless rendering)*

### 13. Foundation Layer Requirements

#### 13.1 Event System
- **REQ-13.1.1**: System shall provide Foundation EventDispatcher for decoupled communication
  - *Subsystems: **Foundation** (event system), **All Subsystems** (event usage)*
- **REQ-13.1.2**: Event system shall be type-safe across all subsystems
  - *Subsystems: **Foundation** (type safety), **All Subsystems** (event types)*
- **REQ-13.1.3**: System shall support event subscription/unsubscription patterns
  - *Subsystems: **Foundation** (event management), **All Subsystems** (event handling)*
- **REQ-13.1.4**: Event handling shall be thread-safe
  - *Subsystems: **Foundation** (thread safety), **All Subsystems** (concurrent access)*

#### 13.2 Memory Management
- **REQ-13.2.1**: System shall provide object pooling via MemoryPool for performance
  - *Subsystems: **Foundation** (memory pool), **Voxel Data** (object reuse)*
- **REQ-13.2.2**: System shall detect memory pressure and trigger cleanup
  - *Subsystems: **Foundation** (memory monitoring), **All Subsystems** (cleanup)*
- **REQ-13.2.3**: System shall track memory allocations
  - *Subsystems: **Foundation** (allocation tracking)*

#### 13.3 Math Utilities
- **REQ-13.3.1**: System shall provide vector and matrix operations
  - *Subsystems: **Foundation** (math utils), **All Subsystems** (calculations)*
- **REQ-13.3.2**: System shall provide intersection calculations
  - *Subsystems: **Foundation** (geometry), **Input** (ray-casting)*
- **REQ-13.3.3**: System shall provide geometric utilities
  - *Subsystems: **Foundation** (geometry), **Visual Feedback** (positioning)*

#### 13.4 Configuration Management
- **REQ-13.4.1**: System shall manage application settings
  - *Subsystems: **Foundation** (config), **All Subsystems** (settings)*
- **REQ-13.4.2**: System shall support platform-specific configurations
  - *Subsystems: **Foundation** (platform config), **Applications** (platform settings)*
- **REQ-13.4.3**: System shall allow runtime parameter adjustment
  - *Subsystems: **Foundation** (runtime config), **All Subsystems** (dynamic settings)*

#### 13.5 Logging
- **REQ-13.5.1**: System shall provide multi-level logging
  - *Subsystems: **Foundation** (logger), **All Subsystems** (logging)*
- **REQ-13.5.2**: System shall support performance profiling
  - *Subsystems: **Foundation** (profiling), **All Subsystems** (performance data)*
- **REQ-13.5.3**: System shall manage debug output
  - *Subsystems: **Foundation** (debug), **All Subsystems** (debug info)*

### 12. Development Workflow Requirements

#### 12.1 Build and Execution
- **REQ-12.1.1**: All executables shall be run via execute_command.sh script
  - *Subsystems: **Build System** (execution wrapper)*
- **REQ-12.1.2**: System shall support comprehensive test coverage for core library
  - *Subsystems: **Testing Framework** (coverage), **All Subsystems** (testability)*
- **REQ-12.1.3**: System shall maintain shared codebase across applications
  - *Subsystems: **Core Library** (shared implementation), **Applications** (library usage)*

#### 12.2 Development Constraints
- **REQ-12.2.1**: System shall focus on single-user editing (no collaboration features)
  - *Subsystems: **All Subsystems** (single-user design)*
- **REQ-12.2.2**: System shall provide testable architecture with isolated subsystems
  - *Subsystems: **Architecture** (modularity), **All Subsystems** (testability)*
- **REQ-12.2.3**: System shall support CI/CD integration for automated testing
  - *Subsystems: **Build System** (CI/CD support), **Testing Framework** (automation)*

---

## Subsystem Impact Summary

### Most Impacted Subsystems (by requirement count):

1. **File I/O** (13 requirements)
   - Binary format specification
   - Project state storage (workspace, camera, history, groups)
   - STL export functionality
   - Compression and versioning
   - File operations integration

2. **Visual Feedback** (26 requirements)
   - Grid visualization and dynamics
   - Preview rendering (green/red outlines)
   - Face highlighting
   - Placement plane indicators
   - Real-time updates
   - Performance optimization

3. **Voxel Data** (25 requirements)
   - Position validation and bounds checking
   - Collision detection
   - Resolution management
   - Memory management and sparse storage
   - Workspace coordination
   - Multi-resolution positioning

4. **Input** (24 requirements)
   - Mouse tracking and ray casting
   - Click handling for placement/removal
   - Position snapping and validation
   - Modifier key handling
   - VR input processing
   - Command line interface

5. **Rendering** (15 requirements)
   - Grid rendering
   - Shader support for highlights/outlines
   - Performance optimization (60+ FPS, VR 90+ FPS)
   - OpenGL abstraction
   - Cross-platform compatibility

6. **Surface Generation** (7 requirements)
   - Dual Contouring algorithm
   - Adaptive mesh generation
   - LOD management
   - Real-time preview
   - Export quality

7. **Testing Framework** (8 requirements)
   - Unit test coverage (895+ tests)
   - Visual validation
   - Performance benchmarking
   - Headless execution

8. **CLI Application** (6 requirements)
   - Interactive command prompt
   - Auto-completion
   - Help system
   - Core command support

9. **Foundation** (17 requirements)
   - Event system (4 requirements)
   - Memory management (3 requirements)
   - Math utilities (3 requirements)
   - Configuration management (3 requirements)
   - Logging system (3 requirements)
   - Platform abstraction

10. **Groups** (4 requirements)
    - Group operations and hierarchy
    - Metadata management
    - State persistence

11. **Undo/Redo** (4 requirements)
    - Command creation for placements/removals
    - History management
    - Limited history for VR

12. **Camera** (4 requirements)
    - Ray transformation
    - View matrices
    - State persistence
    - View control commands

13. **Selection** (2 requirements)
    - Selection management
    - State persistence

### Key Integration Points:

- **Input ↔ Visual Feedback**: Mouse position drives preview updates and face detection
- **Input ↔ Voxel Data**: Validation before placement, collision detection
- **Visual Feedback ↔ Voxel Data**: Preview positioning based on valid positions
- **Visual Feedback ↔ Rendering**: Shader support for outlines and highlights
- **All subsystems → Undo/Redo**: Command creation for reversible operations

---

## Open Questions (Resolved)

The following questions were raised during requirements gathering and have been resolved through design decisions:

### Grid and Coordinate System
1. **Grid Origin**: Center of workspace (0,0,0 at center)
2. **Grid Size**: Matches workspace bounds (2m³ min, 8m³ max)
3. **Grid Visibility**: Always visible with dynamic opacity
4. **Negative Y**: Not allowed - ground plane is a floor

### Voxel Placement Precision
1. **1cm Offset Constraint**: Constrained to 1cm increments
2. **Rotation**: No rotation - axis-aligned only
3. **Default Alignment**: Auto-snap with Shift override

### Visual Feedback
1. **Transparency Values**: 35% default, 65% when active
2. **Highlight Colors**: Grid lines gray, preview green, invalid red, face highlight yellow
3. **Shadow vs Outline**: Outline rendering

### Placement Behavior
1. **Plane Persistence**: Plane "sticks" until clear
2. **Multiple Voxels**: Highest takes precedence
3. **Placement Validation**: Prevents overlaps

### User Interface
1. **Click Behavior**: Left click place, right click remove
2. **Current Voxel Size**: Via resolution command
3. **Undo/Redo**: Yes, with 5-10 operation limit