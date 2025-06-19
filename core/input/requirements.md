# Input Subsystem Requirements
*Created: June 18, 2025*

## Overview
The Input subsystem handles mouse, keyboard, touch, and VR input processing, including ray-casting, position snapping, and placement validation.

## Requirements from Main Requirements Document

### Mouse Input and Ray-Casting
- **REQ-1.2.1**: The grid shall be clickable for voxel placement
- **REQ-1.2.2**: Grid opacity shall increase to 65% within 2 grid squares of cursor during placement
- **REQ-5.1.3**: Mouse movement shall update preview position in real-time
- **REQ-5.1.4**: Ray-casting shall determine face/position under cursor

### Click Handling
- **REQ-5.1.1**: Left-click shall place a voxel at the current preview position
- **REQ-5.1.2**: Right-click on a voxel shall remove that voxel
- **REQ-2.3.3**: Clicking on a highlighted face shall place the new voxel adjacent to that face

### Position Snapping and Calculation
- **REQ-2.1.1**: Voxels shall be placeable only at 1cm increment positions
- **REQ-2.2.2**: The preview shall snap to the nearest valid 1cm increment position
- **REQ-2.2.4**: All voxel sizes (1cm to 512cm) shall be placeable at any valid 1cm increment position on the ground plane
- **REQ-3.1.1**: Same-size voxels shall auto-snap to perfect alignment by default
- **REQ-3.2.2**: Placement shall respect 1cm increment positions on the target face
- **REQ-3.2.3**: The preview shall snap to nearest valid position

### Placement Plane Detection
- **REQ-3.3.1**: Placement plane shall snap to the smaller voxel's face
- **REQ-3.3.2**: Placement plane shall maintain height while preview overlaps any voxel at current height
- **REQ-3.3.3**: When multiple voxels at different heights are under cursor, highest takes precedence
- **REQ-3.3.4**: Plane only changes when preview completely clears current height voxels

### Validation and Error Handling
- **REQ-2.1.4**: No voxels shall be placed below Y=0
- **REQ-5.2.1**: Voxels shall not overlap with existing voxels
- **REQ-5.2.2**: System shall validate placement before allowing it
- **REQ-5.2.3**: Only positions with Y ≥ 0 shall be valid

### Modifier Keys and Controls
- **REQ-3.1.2**: Holding Shift shall allow placement at any valid 1cm increment
- **REQ-5.4.1**: Shift key shall override auto-snap for same-size voxels
- **REQ-5.4.2**: No rotation controls (voxels always axis-aligned)

### Resolution Management
- **REQ-5.3.1**: Current voxel size controlled by active resolution setting
- **REQ-5.3.2**: Resolution changed via `resolution <size>` command

### Performance Requirements
- **REQ-4.1.3**: Preview updates shall be smooth and responsive (< 16ms)
- **REQ-6.1.2**: Preview updates shall complete within 16ms
- **REQ-6.1.3**: Face highlighting shall update within one frame

### Platform Support
- **REQ-7.1.2**: System shall support Meta Quest 3 VR platform
- **REQ-7.3.1**: System shall use Qt6 for desktop GUI application
- **REQ-7.3.2**: System shall use OpenXR SDK for VR interface
- **REQ-7.3.3**: System shall use Meta Hand Tracking SDK for hand tracking

### Command Line Interface
- **REQ-9.1.1**: CLI shall provide interactive command prompt with auto-completion
- **REQ-9.2.2**: CLI shall support camera commands (zoom, view, rotate, reset)

## Implementation Notes
- MouseHandler processes mouse events and generates rays
- KeyboardHandler manages modifier keys (Shift for snap override)
- PlacementValidation ensures valid positions before placement
- PlaneDetector manages placement plane persistence logic
- Supports unified input across desktop (mouse/keyboard), Qt (touch), and VR platforms
- InputManager coordinates all input types and maps to actions
- Real-time position calculation with 1cm precision snapping