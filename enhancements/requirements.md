# Voxel Editor Enhancement Requirements

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
- **REQ-6.1.1**: Grid rendering shall maintain 60 FPS minimum
  - *Subsystems: **Rendering** (performance optimization), **Visual Feedback** (efficient grid rendering)*
- **REQ-6.1.2**: Preview updates shall complete within 16ms
  - *Subsystems: **Visual Feedback** (update performance), **Input** (event processing), **Rendering** (frame timing)*
- **REQ-6.1.3**: Face highlighting shall update within one frame
  - *Subsystems: **Visual Feedback** (immediate updates), **Input** (low-latency processing)*

#### 6.2 Scalability
- **REQ-6.2.1**: System shall handle 10,000+ voxels without degradation
  - *Subsystems: **Voxel Data** (sparse storage), **Rendering** (LOD/culling), **Visual Feedback** (instancing)*
- **REQ-6.2.2**: Grid size shall scale with workspace (up to 8m x 8m)
  - *Subsystems: **Voxel Data** (workspace bounds), **Rendering** (dynamic grid generation), **Visual Feedback** (grid scaling)*

---

## Subsystem Impact Summary

### Most Impacted Subsystems (by requirement count):

1. **Visual Feedback** (23 requirements)
   - Grid visualization and dynamics
   - Preview rendering (green/red outlines)
   - Face highlighting
   - Placement plane indicators
   - Real-time updates

2. **Input** (22 requirements)
   - Mouse tracking and ray casting
   - Click handling for placement/removal
   - Position snapping and validation
   - Modifier key handling
   - Plane detection and persistence

3. **Voxel Data** (21 requirements)
   - Position validation and bounds checking
   - Collision detection
   - Resolution management
   - Workspace coordination
   - Adjacent position calculations

4. **Rendering** (11 requirements)
   - Grid rendering
   - Shader support for highlights/outlines
   - Performance optimization
   - Coordinate transforms
   - Frame timing

5. **Undo/Redo** (3 requirements)
   - Command creation for placements
   - Command creation for removals
   - History management

6. **Camera** (3 requirements)
   - Ray transformation
   - View matrices for grid
   - View-independent highlighting

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