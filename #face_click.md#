# Face Click System Documentation

## High-Level Overview

The face clicking system in the voxel editor enables users to place and remove voxels by clicking on existing voxel faces or the ground plane. The system converts 2D mouse positions into 3D rays, detects intersections with voxel faces, and places new voxels at appropriate positions.

### Key Components

1. **Mouse Input Processing** (`apps/cli/MouseInteraction`)
2. **Ray Generation** (`core/camera/CameraController` & `Viewport`)
3. **Face Detection** (`core/visual_feedback/FaceDetector`)
4. **Placement Validation** (`core/input/PlacementValidation`)
5. **Visual Feedback** (`core/visual_feedback/VisualFeedbackManager`)

## Detailed Architecture

### 1. Mouse Click to Ray Conversion

The conversion pipeline transforms 2D screen coordinates to 3D world rays:

```
Mouse Click (2D) → NDC Coordinates → Clip Space → World Space → Ray
```

#### Step-by-Step Process:

1. **Mouse Event Capture** (`MouseInteraction::handleMouseButton`)
   - Captures mouse position in screen coordinates (pixels)
   - Triggers on left/right click events

2. **Ray Generation** (`CameraController::getMouseRay`)
   - Delegates to `Viewport::screenToWorldRay`
   - Passes current view and projection matrices

3. **Screen to World Ray** (`Viewport::screenToWorldRay`)
   ```cpp
   // Convert screen to normalized device coordinates [-1, 1]
   Vector2f ndc = screenToNormalized(screenPos);
   
   // Create ray endpoints in clip space
   Vector4f nearPoint(ndc.x, ndc.y, -1.0f, 1.0f);
   Vector4f farPoint(ndc.x, ndc.y, 1.0f, 1.0f);
   
   // Transform to world space
   Matrix4f invViewProj = (projectionMatrix * viewMatrix).inverse();
   nearPoint = invViewProj * nearPoint;
   farPoint = invViewProj * farPoint;
   
   // Perspective divide
   nearPoint /= nearPoint.w;
   farPoint /= farPoint.w;
   
   // Create ray
   Vector3f origin = nearPoint.xyz();
   Vector3f direction = (farPoint - nearPoint).xyz().normalized();
   ```

### 2. Face Detection System

The `FaceDetector` class implements ray-voxel intersection using a 3D DDA algorithm:

#### Key Methods:

- **`detectFace(ray, maxDistance)`**
  - Traverses voxel grid along ray path
  - Uses 1cm increment steps for precision
  - Returns first voxel face hit

- **`detectGroundPlane(ray)`**
  - Special case for Y=0 plane intersection
  - Used when no voxel is hit

- **`detectFaceOrGround(ray, maxDistance)`**
  - Combines both detection methods
  - Falls back to ground plane if no voxel hit

#### DDA Algorithm Implementation:

```cpp
// Initialize ray traversal
Vector3f currentPos = ray.origin;
Vector3f step = ray.direction * INCREMENT_SIZE; // 1cm steps

while (distance < maxDistance) {
    // Check current position for voxel
    if (hasVoxelAt(currentPos)) {
        // Determine which face was hit
        Face face = calculateHitFace(previousPos, currentPos);
        return FaceDetectionResult(voxelPos, face);
    }
    
    // Advance along ray
    currentPos += step;
    distance += INCREMENT_SIZE;
}
```

### 3. Face to Placement Position

Once a face is detected, the system calculates where to place the new voxel:

#### `FaceDetector::calculatePlacementPosition`

1. **Get face normal** - Direction perpendicular to clicked face
2. **Calculate offset** - Distance based on clicked voxel size and new voxel size
3. **Apply offset** - Move from face center along normal
4. **Snap to grid** - Ensure 1cm increment alignment

```cpp
// Example for top face click
Vector3f faceNormal(0, 1, 0);
float clickedVoxelRadius = clickedVoxelSize / 2.0f;
float newVoxelRadius = newVoxelSize / 2.0f;
float offset = clickedVoxelRadius + newVoxelRadius;

Vector3f placementPos = clickedVoxelCenter + faceNormal * offset;
placementPos = snapToIncrement(placementPos); // Snap to 1cm grid
```

### 4. Placement Validation

The `PlacementValidation` system ensures voxels are placed at valid positions:

#### Validation Checks:

1. **Increment Alignment** - All positions must be at 1cm increments
2. **Workspace Bounds** - Within defined workspace limits
3. **Ground Constraint** - Y coordinate >= 0
4. **Collision Detection** - No overlap with existing voxels

#### Smart Snapping for Face Placement:

When placing smaller voxels on larger voxel faces, the system uses intelligent snapping:

```cpp
// For placing 1cm voxel on 8cm voxel face
// Snaps to nearest 1cm position that keeps voxel fully on face
Vector3f snappedPos = snapToSurfaceFaceGrid(
    worldPos,           // Desired position
    targetResolution,   // 1cm
    surfaceResolution,  // 8cm
    faceNormal         // Which face we're on
);
```

### 5. Visual Feedback Integration

Throughout the process, visual feedback is provided:

1. **Hover State** - Yellow highlight on face under cursor
2. **Placement Preview** - Green/red voxel showing where placement will occur
3. **Grid Alignment** - Visual grid showing 1cm increments
4. **Face Indicators** - Face-specific placement guides

## Data Flow Diagram

```
Mouse Move/Click
    ↓
MouseInteraction::handleMouseButton/Motion
    ↓
CameraController::getMouseRay
    ↓
Viewport::screenToWorldRay
    ↓
Ray (origin, direction)
    ↓
FaceDetector::detectFaceOrGround
    ↓
Face Detection Result (position, face, normal)
    ↓
FaceDetector::calculatePlacementPosition
    ↓
PlacementValidation::validatePlacement
    ↓
VisualFeedbackManager::showPreview (if valid)
    ↓
VoxelCommands::PlaceVoxelCommand (on click)
    ↓
VoxelDataManager::setVoxel
```

## Design Analysis

### Strengths

1. **Precision** - 1cm increment stepping ensures accurate detection at all positions
2. **Multi-resolution Support** - Handles 10 different voxel sizes seamlessly
3. **Visual Feedback** - Clear indication of placement before committing
4. **Undo/Redo Integration** - All operations are reversible
5. **Ground Plane Fallback** - Intuitive behavior when clicking empty space

### Potential Improvements

1. **Performance** - DDA stepping at 1cm could be optimized with hierarchical traversal
2. **Edge Cases** - Some corner/edge clicks might benefit from additional logic
3. **Face Priority** - When multiple faces are near click point, selection could be smarter
4. **Preview Updates** - Could be smoother during rapid mouse movement

### Design Patterns Used

1. **Command Pattern** - For undo/redo functionality
2. **Observer Pattern** - For visual feedback updates
3. **Strategy Pattern** - For different validation rules
4. **Single Responsibility** - Each class has a clear, focused purpose

## Conclusion

The face clicking system is well-designed with clear separation of concerns. The architecture properly handles the complex task of converting 2D input to 3D operations while maintaining precision and providing good user feedback. The use of 1cm increment stepping ensures compatibility with the multi-resolution voxel system, though there may be room for performance optimizations in the ray traversal algorithm.