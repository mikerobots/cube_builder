# Visual Feedback Subsystem Requirements
*Created: June 18, 2025*

## Overview
The Visual Feedback subsystem handles all visual cues including grid rendering, preview outlines, face highlighting, and overlay rendering.

## Requirements from Main Requirements Document

### Grid Rendering and Visualization
- **REQ-1.1.1**: The ground plane shall display a grid with 32cm x 32cm squares
- **REQ-1.1.3**: Grid lines shall use RGB(180, 180, 180) at 35% opacity
- **REQ-1.1.4**: Major grid lines every 160cm shall use RGB(200, 200, 200) and be thicker
- **REQ-1.2.2**: Grid opacity shall increase to 65% within 2 grid squares of cursor during placement
- **REQ-6.2.2**: Grid size shall scale with workspace (up to 8m x 8m)

### Preview Rendering and Outlines
- **REQ-2.2.1**: When hovering over the ground plane, a green outline preview shall be displayed
- **REQ-2.2.2**: The preview shall snap to the nearest valid 1cm increment position
- **REQ-2.2.3**: The preview shall update in real-time as the mouse moves
- **REQ-2.2.4**: All voxel sizes (1cm to 512cm) shall be placeable at any valid 1cm increment position on the ground plane
- **REQ-3.2.1**: A green outline preview shall show exact placement position
- **REQ-3.2.3**: The preview shall snap to nearest valid position
- **REQ-4.1.1**: All placement previews shall use green outline rendering
- **REQ-4.1.2**: Invalid placements shall show red outline preview
- **REQ-4.3.2**: Invalid placement attempts shall show red preview
- **REQ-4.3.3**: Valid placements shall show green preview

### Face Highlighting
- **REQ-2.3.1**: When hovering over an existing voxel, the face under the cursor shall be highlighted
- **REQ-2.3.2**: Face highlighting shall clearly indicate which face is selected
- **REQ-4.2.1**: Face highlighting shall use yellow color (as per DESIGN.md)
- **REQ-4.2.2**: Only one face shall be highlighted at a time
- **REQ-4.2.3**: Highlighting shall be visible from all camera angles

### Placement Plane and Interaction
- **REQ-3.1.2**: Holding Shift shall allow placement at any valid 1cm increment
- **REQ-3.1.3**: Aligned placement means edges match perfectly with the target face
- **REQ-3.3.1**: Placement plane shall snap to the smaller voxel's face
- **REQ-3.3.4**: Plane only changes when preview completely clears current height voxels
- **REQ-5.4.1**: Shift key shall override auto-snap for same-size voxels

### Real-Time Updates and Performance
- **REQ-4.1.3**: Preview updates shall be smooth and responsive (< 16ms)
- **REQ-5.1.3**: Mouse movement shall update preview position in real-time
- **REQ-6.1.1**: Grid rendering shall maintain 60 FPS minimum (90+ FPS for VR)
- **REQ-6.1.2**: Preview updates shall complete within 16ms
- **REQ-6.1.3**: Face highlighting shall update within one frame
- **REQ-6.2.1**: System shall handle 10,000+ voxels without degradation
- **REQ-6.3.3**: Rendering buffers shall not exceed 512MB
- **REQ-7.1.3**: System shall use OpenGL 3.3+ core profile minimum

## Implementation Notes
- Uses OutlineRenderer for voxel placement previews
- Uses HighlightRenderer for face highlighting
- Uses OverlayRenderer for grid rendering and UI overlays
- FaceDetector handles ray-casting for face selection
- PreviewManager coordinates preview state and updates
- Supports dynamic opacity changes for grid interaction
- Optimized for real-time performance with large voxel counts