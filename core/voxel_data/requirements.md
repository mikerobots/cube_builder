# Voxel Data Subsystem Requirements
*Created: June 18, 2025*

## Overview
The Voxel Data subsystem manages multi-resolution voxel storage, workspace bounds, position validation, and collision detection.

## Requirements from Main Requirements Document

### Coordinate System and Workspace
- **REQ-1.1.5**: The grid origin (0,0,0) shall be at the center of the workspace
- **REQ-1.2.3**: The grid shall extend to cover the entire workspace area
- **REQ-6.2.2**: Grid size shall scale with workspace (up to 8m x 8m)

### Position Validation and Bounds
- **REQ-2.1.1**: Voxels shall be placed at any 1cm increment position without resolution-based snapping
- **REQ-2.1.2**: Voxels of all sizes shall maintain their exact placement position in 1cm increments
- **REQ-2.1.3**: Voxels shall always be axis-aligned (no rotation)
- **REQ-2.1.4**: No voxels shall be placed below Y=0
- **REQ-2.1.5**: The entire voxel extent shall fit within workspace bounds (placement position + voxel size)
- **REQ-5.2.3**: Only positions with Y ≥ 0 shall be valid

### Multi-Resolution Support
- **REQ-2.2.4**: All voxel sizes (1cm to 512cm) shall be placeable at any valid 1cm increment position on the ground plane
- **REQ-5.3.1**: Current voxel size controlled by active resolution setting
- **REQ-5.3.3**: Available resolutions: 1cm, 2cm, 4cm, 8cm, 16cm, 32cm, 64cm, 128cm, 256cm, 512cm
- **REQ-5.3.4**: Changing resolution shall NOT clear or remove existing voxels
- **REQ-5.3.5**: Voxels of different resolutions shall coexist in the same workspace
- **REQ-5.3.6**: The voxel count query shall return counts for the currently active resolution only
- **REQ-5.3.7**: A separate total voxel count query shall be available for counting across all resolutions

### Voxel Placement and Management
- **REQ-3.1.1**: When placing a same-size voxel on an existing voxel's face, the new voxel shall automatically align so its edges match perfectly with the clicked face
- **REQ-3.1.2**: Holding Shift shall override auto-alignment, allowing placement at any valid 1cm increment position on the face
- **REQ-3.1.3**: Auto-aligned placement ensures the new voxel is positioned adjacent to the clicked face with edges matching perfectly
- **REQ-3.2.2**: Placement shall respect 1cm increment positions on the target face
- **REQ-2.3.3**: Clicking on a highlighted face shall place the new voxel adjacent to that face
- **REQ-5.1.1**: Left-click shall place a voxel at the current preview position
- **REQ-5.1.2**: Right-click on a voxel shall remove that voxel

### Collision Detection and Spatial Queries
- **REQ-3.3.2**: Placement plane shall maintain height while preview overlaps any voxel at current height
- **REQ-3.3.3**: When multiple voxels at different heights are under cursor, highest takes precedence
- **REQ-4.3.1**: System shall prevent overlapping voxel placements of same or larger size
- **REQ-5.2.1**: Voxels shall not overlap with existing voxels of same or smaller size
- **REQ-5.2.2**: System shall validate placement before allowing it
- **REQ-5.2.5**: Smaller voxels may be placed on the faces of larger voxels for detailed work

### Validation and Error Handling
- **REQ-4.1.2**: Invalid placements shall show red outline preview
- **REQ-4.3.2**: Invalid placement attempts shall show red preview
- **REQ-4.3.3**: Valid placements shall show green preview
- **REQ-4.3.4**: Collision detection shall apply between voxels of different resolutions (smaller voxels allowed on larger ones)
- **REQ-4.3.5**: Failed placement or fill commands shall make no state changes (atomic operations)
- **REQ-5.2.4**: Redundant operations (placing/removing at same position with same value) shall fail

### Performance and Scalability
- **REQ-6.1.4**: Resolution switching shall complete within 100ms
- **REQ-6.2.1**: System shall handle 10,000+ voxels without degradation
- **REQ-6.3.1**: Total application memory shall not exceed 4GB (Meta Quest 3 constraint)
- **REQ-6.3.2**: Voxel data storage shall not exceed 2GB
- **REQ-6.3.5**: System shall detect and respond to memory pressure

### File Format Storage
- **REQ-8.1.2**: Format shall store workspace dimensions and settings
- **REQ-8.1.3**: Format shall store multi-resolution voxel data for all 10 levels
- **REQ-8.1.4**: Format shall store current active resolution level

### Workspace Resizing Behavior
- **REQ-9.3.6**: Workspace resize to smaller dimensions shall fail if any voxels would be outside the new bounds
- **REQ-9.3.3**: Workspace resize to larger dimensions shall preserve all existing voxels
- **REQ-9.3.4**: Workspace resize failure shall leave the workspace and all voxels unchanged

### Command Line Interface
- **REQ-9.2.3**: CLI shall support voxel commands (resolution, workspace, render)

## Implementation Notes
- Uses centered coordinate system with origin at workspace center
- Supports sparse storage for memory efficiency (sparse octree data structure)
- Provides collision detection for all resolution combinations
- Manages workspace bounds dynamically (2m³ min, 8m³ max, 5m³ default)
- Validates positions at 1cm increment precision (no resolution-based snapping)
- **Validates that entire voxel extent fits within workspace bounds, not just placement position**
- Auto-alignment ONLY applies to same-size voxel placement (e.g., 4cm on 4cm)
- Different-size voxels place at any valid 1cm increment without snapping
- Memory pooling and object reuse for performance
- Automatic memory pressure detection and cleanup
- Real-time editing across all 10 resolution levels
- Resolution changes preserve all existing voxels (multi-resolution coexistence)