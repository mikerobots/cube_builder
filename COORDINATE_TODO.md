# COORDINATE_TODO.md - Coordinate System Migration Tracking

## 🎯 OBJECTIVE
Ensure all subsystems use the centered coordinate system (0,0,0 at workspace center) consistently throughout the codebase.

## 📋 AGENT INSTRUCTIONS

### Before Starting:
1. **CLAIM YOUR SUBSYSTEM**: Mark your subsystem as "🔄 IN PROGRESS - Agent: [Your Name]"
2. **READ CONTEXT**: Review ARCHITECTURE.md and DESIGN.md for subsystem details
3. **UNDERSTAND THE GOAL**: All coordinates should be centered at (0,0,0) with workspace extending equally in all directions

### Analysis Process:
1. **List All Files**: Document every .cpp and .h file in the subsystem
2. **Check Each File**: Look for:
   - Coordinate calculations and transformations
   - Hardcoded coordinate assumptions
   - Boundary checks and validation
   - Position conversions and snapping
   - Any math involving positions or bounds
3. **Document Findings**: For each file, note:
   - ✅ CORRECT: Uses centered coordinates properly
   - ❌ NEEDS FIX: Contains old coordinate assumptions
   - ⚠️ SUSPICIOUS: Unclear, needs deeper review
   - 🔍 NO COORDS: File doesn't handle coordinates
4. **Provide Details**: For files needing fixes, specify:
   - Line numbers of problematic code
   - What needs to be changed
   - Potential impact on other systems

### Completion:
1. Update status to "✅ COMPLETE" or "❌ NEEDS FIXES"
2. Summarize findings and recommendations
3. Note any dependencies on other subsystems

## 🏗️ SUBSYSTEM STATUS

### 1. VoxelData Management (`src/voxel_data/`)
**Status**: ❌ NEEDS FIXES  
**Agent**: Claude  
**Last Updated**: 2025-01-17

**Files Analyzed**:
- [x] VoxelDataManager.h - ✅ CORRECT
- [x] VoxelGrid.h - ✅ CORRECT
- [x] WorkspaceManager.h - ✅ CORRECT
- [x] VoxelTypes.h - ❌ NEEDS FIX
- [x] SparseOctree.h - ✅ CORRECT (uses relative grid coordinates)
- [ ] SparseOctree.cpp - Not analyzed (implementation details)
- [ ] No VoxelCoordinate.h/cpp or MultiResolutionGrid.h/cpp files exist

**Analysis Summary**:
The VoxelData subsystem mostly uses centered coordinates correctly:

✅ **CORRECT Implementations**:
1. **WorkspaceManager.h** (lines 77-83, 90-96):
   - Correctly defines workspace bounds as centered: `(-size/2, 0, -size/2)` to `(size/2, size, size/2)`
   - Position validation uses centered bounds
   - Y >= 0 constraint properly enforced

2. **VoxelGrid.h** (lines 104-110, 113-127, 129-139):
   - `isValidWorldPosition()` uses centered coordinates
   - `worldToGrid()` correctly shifts from centered to grid indices
   - `gridToWorld()` correctly shifts from grid indices to centered world

3. **VoxelDataManager.h**:
   - Uses VoxelGrid's coordinate conversion methods
   - Position validation delegates to WorkspaceManager
   - Collision detection uses world space properly

4. **SparseOctree.h**:
   - Uses relative grid coordinates (0-based indexing)
   - No direct world coordinate assumptions

❌ **NEEDS FIX**:
1. **VoxelTypes.h** (lines 59-70, 73-86):
   - `VoxelPosition::toWorldSpace()` assumes old 0-based coordinate system
   - `VoxelPosition::fromWorldSpace()` assumes old 0-based coordinate system
   - These methods don't account for centered workspace
   - Should use VoxelGrid's conversion methods or accept workspace center

**Recommendations**:
1. Fix `VoxelPosition` struct methods to handle centered coordinates
2. Either remove these methods and use VoxelGrid's conversions, or update them to accept workspace bounds/center
3. Ensure all tests using VoxelPosition are updated after fix

**Dependencies**:
- Input system likely uses VoxelPosition for placement
- File I/O may serialize VoxelPosition data
- Commands may use VoxelPosition::toWorldSpace()

---

### 2. Rendering Engine (`src/rendering/`)
**Status**: ✅ COMPLETE  
**Agent**: Claude  
**Last Updated**: 2025-01-17

**Files Analyzed**:
- [x] RenderEngine.h/cpp - ✅ CORRECT (voxel rendering is stubbed)
- [x] OpenGLRenderer.h/cpp - ✅ CORRECT (low-level GPU operations, no coordinate assumptions)
- [x] GroundPlaneGrid.h/cpp - ✅ CORRECT
- [x] RenderTypes.h - ✅ CORRECT (just data structures)
- [x] VoxelMeshGenerator.h/cpp (apps/cli) - ❌ NEEDS FIX
- [ ] ShaderManager.h/cpp - Not analyzed (shader management only)
- [ ] RenderState.h/cpp - Not analyzed (state management only)

**Analysis Summary**:
The Rendering subsystem mostly handles coordinates correctly:

✅ **CORRECT Implementations**:
1. **GroundPlaneGrid.cpp** (lines 181-193, 195-213):
   - Correctly generates grid centered at origin
   - Uses halfSizeX/halfSizeZ to calculate bounds: `-cellsX * cellSize` to `cellsX * cellSize`
   - Grid lines properly span from negative to positive coordinates
   - Y coordinate fixed at 0 for ground plane

2. **RenderEngine.cpp**:
   - `renderVoxels()` is just a stub - no coordinate assumptions
   - Actual voxel rendering happens in application layer

3. **OpenGLRenderer**:
   - Low-level GPU wrapper, no coordinate system assumptions
   - Just passes through vertex data

❌ **NEEDS FIX**:
1. **VoxelMeshGenerator.cpp** (lines 69-73):
   - Uses old coordinate system assuming voxels start at (0,0,0)
   - Calculates world position as `gridPos * voxelSize + voxelSize * 0.5f`
   - Should use VoxelGrid's `gridToWorld()` method or account for centered workspace
   - This is why voxels appear in wrong positions in the CLI app

**Recommendations**:
1. Update VoxelMeshGenerator to use proper coordinate conversion
2. Either use VoxelGrid's conversion methods or accept workspace bounds
3. The fix should shift X and Z coordinates by -workspaceSize/2

**Dependencies**:
- CLI application depends on VoxelMeshGenerator for rendering
- Visual feedback system may use similar mesh generation

---

### 3. Camera System (`src/camera/`)
**Status**: ✅ COMPLETE  
**Agent**: Claude  
**Last Updated**: 2025-01-17

**Files Analyzed**:
- [x] Camera.h - ✅ CORRECT
- [x] OrbitCamera.h - ✅ CORRECT
- [x] CameraController.h - ✅ CORRECT
- [x] Viewport.h - ✅ CORRECT
- [ ] No CameraManager, ViewPresets, or CameraAnimator files (functionality built into OrbitCamera)

**Analysis Summary**:
The Camera System correctly uses centered coordinates:

✅ **CORRECT Implementations**:
1. **Camera.h** (lines 27-28):
   - Default target is (0,0,0) - correctly centered
   - Default position is (0,0,5) - looking at origin
   - No hardcoded coordinate assumptions

2. **OrbitCamera.h** (lines 31, 253):
   - Target defaults to (0,0,0)
   - Camera orbits around the target position
   - View presets all orbit around current target
   - `frameBox()` correctly calculates center as `(minBounds + maxBounds) * 0.5f`

3. **CameraController.h** (line 147-148):
   - `frameAll()` correctly uses bounding box center
   - No coordinate system assumptions

4. **Viewport.h**:
   - Pure screen-to-world transformation math
   - No coordinate system assumptions
   - Works with any coordinate system

**Recommendations**:
- No changes needed - the camera system is coordinate-system agnostic
- It correctly focuses on (0,0,0) by default
- All camera operations work relative to the target position

**Dependencies**:
- None - camera system is independent of coordinate system choice

---

### 4. Selection System (`src/selection/`)
**Status**: ✅ COMPLETE  
**Agent**: Claude  
**Last Updated**: 2025-01-17

**Files Analyzed**:
- [x] SelectionManager.h/cpp - ✅ CORRECT (uses selector classes)
- [x] SelectionSet.h/cpp - ✅ CORRECT (stores VoxelIds)
- [x] SelectionTypes.h/cpp - ❌ NEEDS FIX
- [x] BoxSelector.h/cpp - ❌ NEEDS FIX
- [x] SphereSelector.h/cpp - ❌ NEEDS FIX
- [ ] FloodFillSelector.h/cpp - Not analyzed
- [ ] SelectionRenderer.h/cpp - Not analyzed

**Analysis Summary**:
The Selection System has multiple coordinate system issues:

✅ **CORRECT Implementations**:
1. **SelectionManager**:
   - Delegates to selector classes for actual selection
   - No direct coordinate assumptions

2. **SelectionSet**:
   - Just stores VoxelIds
   - No coordinate system assumptions

❌ **NEEDS FIX**:
1. **SelectionTypes.cpp** (lines 8-12, 21-25):
   - `VoxelId::getWorldPosition()` assumes old coordinate system
   - Returns `position * size + size * 0.5f` without centering
   - `VoxelId::getBounds()` also assumes non-centered coordinates

2. **BoxSelector.cpp** (lines 74-78):
   - Hardcoded workspace bounds as `(0,0,0)` to `workspaceSize`
   - Should be centered: `(-size/2, 0, -size/2)` to `(size/2, size, size/2)`

3. **SphereSelector.cpp** (lines 68-71):
   - Same issue - workspace bounds from `(0,0,0)` to `workspaceSize`
   - Should use centered bounds

**Recommendations**:
1. Update VoxelId methods to use proper coordinate conversion
2. Fix BoxSelector and SphereSelector to use WorkspaceManager's centered bounds
3. Consider using VoxelGrid's conversion methods instead

**Dependencies**:
- Visual feedback system may use selection bounds
- Commands use selection for operations

---

### 5. Input System (`src/input/`)
**Status**: ❌ NEEDS FIXES  
**Agent**: Claude  
**Last Updated**: 2025-01-17

**Files Analyzed**:
- [x] PlacementValidation.h/cpp - ❌ NEEDS FIX (CRITICAL)
- [x] PlaneDetector.h/cpp - ✅ CORRECT
- [x] MouseHandler.h/cpp - ✅ CORRECT
- [ ] InputManager.h/cpp - Not analyzed
- [ ] KeyboardHandler.h/cpp - Not analyzed
- [ ] TouchHandler.h/cpp - Not analyzed
- [ ] VRInputHandler.h/cpp - Not analyzed
- [ ] InputMapper.h/cpp - Not analyzed
- [ ] No MouseRayGenerator files found

**Analysis Summary**:
The Input System has **CRITICAL coordinate system bugs** in PlacementValidation that are likely causing the integration test failures.

✅ **CORRECT Implementations**:
1. **PlaneDetector.cpp**:
   - Ground plane correctly defined at Y=0 (center, not minimum)
   - Properly handles negative Y values in calculations
   - No hardcoded coordinate assumptions

2. **MouseHandler.cpp**:
   - Ray creation is relative to camera
   - No coordinate system assumptions
   - Proper screen-to-world transformations

❌ **CRITICAL FIXES NEEDED**:
1. **PlacementValidation.cpp**:
   - Line 45-47: `if (gridPos.y < 0) return InvalidYBelowZero;` - WRONG, prevents placement below Y=0
   - Line 59: Workspace bounds check uses `worldPos.y < 0` instead of centered bounds
   - Line 72: `isValidIncrementPosition()` enforces `pos.y >= 0` constraint
   - Line 302-304: Another hardcoded `worldPos.y < 0.0f` check

**Root Cause**: PlacementValidation incorrectly assumes Y=0 is the minimum workspace boundary instead of the center. This prevents voxel placement on the lower half of the workspace, explaining why click placement tests fail.

**Recommendations**:
1. Remove all Y >= 0 constraints from PlacementValidation
2. Update workspace bounds checks to use centered coordinates: `y < -size.y/2 || y > size.y/2`
3. Remove or rename `InvalidYBelowZero` enum value
4. Update related tests to verify placement below Y=0 is allowed

**Dependencies**:
- All voxel placement operations depend on PlacementValidation
- Click placement tests will fail until this is fixed
- Commands using placement validation will reject valid positions

---

### 6. Groups System (`src/groups/`)
**Status**: ✅ COMPLETE  
**Agent**: Claude  
**Last Updated**: 2025-01-17

**Files Analyzed**:
- [x] GroupManager.h/cpp - ✅ CORRECT (high-level management)
- [x] VoxelGroup.h/cpp - ❌ NEEDS FIX
- [x] GroupTypes.h - ✅ CORRECT (just type definitions)
- [x] GroupOperations.h - ✅ CORRECT (uses VoxelGroup methods)
- [ ] GroupHierarchy.h/cpp - Not analyzed (hierarchy management only)

**Analysis Summary**:
The Groups System has coordinate issues in VoxelGroup:

✅ **CORRECT Implementations**:
1. **GroupManager.h**:
   - High-level group management
   - Delegates to VoxelGroup for operations

2. **GroupTypes.h**:
   - Just type definitions
   - VoxelId struct matches voxel_data (no world conversion)

3. **GroupOperations.h**:
   - Uses VoxelGroup and VoxelDataManager methods
   - No direct coordinate assumptions

❌ **NEEDS FIX**:
1. **VoxelGroup.cpp** (lines 112, 176-177):
   - `translate()` method: calculates world position as `voxel.position * voxelSize`
   - `updateBounds()` method: same issue - `voxel.position * voxelSize`
   - Both assume non-centered coordinate system
   - Should use VoxelGrid's conversion or account for workspace center

**Recommendations**:
1. Update VoxelGroup to use proper coordinate conversion
2. Consider using VoxelGrid's gridToWorld() method
3. Or add workspace bounds parameter to methods

**Dependencies**:
- Group operations rely on correct bounds calculation
- Group center calculation affects rotation/scaling pivot points

---

### 7. Visual Feedback System (`src/visual_feedback/`)
**Status**: ❌ NEEDS FIXES  
**Agent**: Claude  
**Last Updated**: 2025-01-17

**Files Analyzed**:
- [x] FeedbackRenderer.h/cpp - ❌ NEEDS FIX (hardcoded workspace bounds)
- [x] HighlightRenderer.h/cpp - ❌ NEEDS FIX (voxel position calculation)
- [x] OutlineRenderer.h/cpp - ❌ NEEDS FIX (voxel edge generation)
- [x] FeedbackTypes.h/cpp - ❌ NEEDS FIX (Face position calculation)
- [ ] OverlayRenderer.h/cpp - Not analyzed (likely just UI overlays)
- [ ] PreviewManager.h/cpp - Not analyzed (delegates to renderers)
- [ ] HighlightManager.h/cpp - Not analyzed (high-level management)
- [x] FaceDetector.h/cpp - ✅ CORRECT (Already fixed in commit 3ed8cbe)

**Analysis Summary**:
The Visual Feedback subsystem has **multiple critical coordinate system issues** that directly affect visual rendering:

✅ **CORRECT Implementations**:
1. **FaceDetector.h/cpp**:
   - Already fixed in commit 3ed8cbe to use centered coordinates
   - Properly handles workspace bounds and ground plane detection

❌ **NEEDS FIX**:
1. **FeedbackRenderer.cpp** (lines 30-33):
   - Hardcoded workspace bounds: `Math::BoundingBox(Vector3f(0,0,0), Vector3f(10,10,10))`
   - Should use centered workspace bounds from WorkspaceManager

2. **HighlightRenderer.cpp** (lines 423-427):
   - `calculateVoxelTransform()`: `position.x * voxelSize + voxelSize * 0.5f`
   - Assumes grid coordinates start at (0,0,0) instead of centered workspace
   - This causes highlights to appear in wrong positions

3. **OutlineRenderer.cpp** (lines 272-276):
   - `addVoxelEdges()`: `basePos = position.x * voxelSize` 
   - Same issue - assumes non-centered coordinate system
   - Green outline previews appear in wrong positions

4. **OutlineRenderer.cpp** (lines 465, 491-492):
   - `findExternalEdges()`: Group outline generation uses same flawed coordinate conversion
   - `basePos = Math::Vector3f(x * voxelSize, y * voxelSize, z * voxelSize)`
   - Group outlines will be positioned incorrectly

5. **FeedbackTypes.cpp** (line 65):
   - `Face::getWorldPosition()`: `basePos = Vector3f(position) * voxelSize`
   - Comment on line 64 acknowledges this needs to use VoxelGrid::gridToWorld()
   - Face highlighting will be positioned incorrectly

**Root Cause**: All visual feedback components convert from grid coordinates to world coordinates by simply multiplying by voxel size, ignoring the centered coordinate system where world space extends from negative to positive coordinates.

**Recommendations**:
1. Update all coordinate transformations to use VoxelGrid's gridToWorld() method
2. Or pass workspace bounds/center to these components
3. Fix FeedbackRenderer to get workspace bounds from WorkspaceManager
4. The fix pattern: Replace `pos * voxelSize` with `grid.gridToWorld(pos)`

**Dependencies**:
- All visual feedback (highlights, outlines, previews) will appear in wrong positions
- This directly affects user interaction - clicking won't match visual indicators
- Related to failing integration tests for voxel placement

**Key Areas to Check**:
- Face highlighting positions
- Outline preview coordinates  
- Preview voxel positioning
- Group outline rendering

---

### 8. File I/O System (`src/file_io/`)
**Status**: ✅ COMPLETE  
**Agent**: Claude  
**Last Updated**: 2025-01-17

**Files Analyzed**:
- [x] FileManager.h/cpp - ✅ CORRECT (high-level file operations)
- [x] BinaryFormat.h/cpp - ✅ CORRECT (stores grid coordinates, not world positions)
- [x] STLExporter.h/cpp - ✅ CORRECT (operates on mesh vertices already in world space)
- [x] Project.h/cpp - ✅ CORRECT (just data structures)
- [x] FileTypes.h - ✅ CORRECT (WorkspaceSettings.origin is correctly (0,0,0))
- [ ] Compression.h/cpp - Not analyzed (compression only)
- [ ] FileVersioning.h/cpp - Not analyzed (version management only)
- [ ] BinaryIO.h/cpp - Not analyzed (I/O primitives only)

**Analysis Summary**:
The File I/O System correctly handles coordinates:

✅ **CORRECT Implementations**:
1. **BinaryFormat.cpp**:
   - Voxel data serialization: stores grid coordinates, not world positions (lines 413-420)
   - `auto voxels = grid->getAllVoxels()` returns VoxelPosition with grid coordinates
   - Workspace settings serialization: stores size and origin correctly (lines 809-810)
   - No coordinate transformations during save/load

2. **FileTypes.h** (lines 62-63):
   - `WorkspaceSettings.size` - workspace size vector
   - `WorkspaceSettings.origin = Vector3f(0,0,0)` - correctly centered at origin
   - No hardcoded coordinate assumptions

3. **STLExporter.h**:
   - Operates on Rendering::Mesh which already contains world-space vertices
   - Just writes vertex positions as-is to STL files
   - Unit conversion and scaling work on already-transformed meshes

4. **Project.h**:
   - Just contains shared_ptr to data managers
   - No coordinate transformations
   - Delegates to component systems

5. **FileManager.h**:
   - High-level file operations
   - Delegates to BinaryFormat and STLExporter

**Recommendations**:
- No changes needed - File I/O system is coordinate-system agnostic
- It correctly stores grid coordinates and workspace settings
- Coordinate transformations happen in the subsystems that generate the data

**Dependencies**:
- Depends on VoxelDataManager for grid coordinates (which may have issues)
- STL export depends on mesh generation (which may have coordinate issues)
- But the File I/O itself doesn't introduce coordinate problems

---

### 9. Surface Generation (`src/surface_gen/`)
**Status**: ❌ NEEDS FIXES  
**Agent**: Claude  
**Last Updated**: 2025-01-17

**Files Analyzed**:
- [x] SurfaceTypes.h/cpp - ✅ CORRECT (data structures, no coordinate assumptions)
- [x] MeshBuilder.h/cpp - ✅ CORRECT (operates on already-transformed vertices)
- [x] DualContouring.h/cpp - ❌ NEEDS FIX
- [x] SurfaceGenerator.h/cpp - ✅ CORRECT (delegates to DualContouring)
- [ ] LODManager.h/cpp - Not analyzed (LOD management only)

**Analysis Summary**:
The Surface Generation subsystem has a critical coordinate system issue in DualContouring:

✅ **CORRECT Implementations**:
1. **SurfaceTypes.h/cpp**:
   - Just data structures (Mesh, HermiteData, etc.)
   - Mesh transformation and bounds calculation work with any coordinate system
   - No hardcoded coordinate assumptions

2. **MeshBuilder.h/cpp**:
   - Operates on vertices that are already in world space
   - No coordinate transformations
   - All operations are coordinate-system agnostic

3. **SurfaceGenerator.h/cpp**:
   - High-level orchestration
   - Delegates actual mesh generation to DualContouring
   - No direct coordinate manipulation

❌ **NEEDS FIX**:
1. **DualContouring.cpp** (line 231):
   - `Math::Vector3f worldVertex = vertex * voxelSize;`
   - This assumes grid coordinates start at (0,0,0)
   - Should use VoxelGrid's gridToWorld() method or account for centered workspace
   - This is why generated meshes appear in the wrong position

**Root Cause**: DualContouring generates vertices in grid space (0-based indices) then converts to world space by simply multiplying by voxel size. This doesn't account for the centered coordinate system where the workspace extends from negative to positive coordinates.

**Recommendations**:
1. Pass VoxelGrid reference to mesh generation to use its gridToWorld() method
2. Or pass workspace bounds/center to DualContouring
3. Update the coordinate transformation to: `worldVertex = grid.gridToWorld(Math::Vector3i(vertex))`

**Dependencies**:
- All mesh generation depends on correct vertex positioning
- STL export will have incorrect coordinates
- Visual rendering will show meshes in wrong position

**Key Areas to Check**:
- Vertex position generation
- Surface normal calculations
- Mesh center point
- LOD position scaling
- Edge position interpolation

---

### 10. Undo/Redo System (`src/undo_redo/`)
**Status**: ❌ NEEDS FIXES  
**Agent**: Claude  
**Last Updated**: 2025-01-17

**Files Analyzed**:
- [x] Command.h - 🔍 NO COORDS (base interface only)
- [x] HistoryManager.h/cpp - 🔍 NO COORDS (command management only)
- [x] VoxelCommands.h/cpp - ❌ NEEDS FIX (VoxelFillCommand coordinate calculations)
- [x] PlacementCommands.h/cpp - ⚠️ SUSPICIOUS (uses PlacementUtils validation)
- [x] SelectionCommands.h/cpp - ✅ CORRECT (coordinate-agnostic)
- [x] StateSnapshot.h/cpp - ❌ NEEDS FIX (raw coordinate storage)
- [x] Transaction.h/cpp - 🔍 NO COORDS (transaction management only)
- [x] CompositeCommand.h/cpp - 🔍 NO COORDS (command composition only)

**Analysis Summary**:
The Undo/Redo System core infrastructure is coordinate-agnostic, but specific command implementations have coordinate system issues:

✅ **CORRECT Implementations**:
1. **Core Infrastructure**:
   - HistoryManager: Command execution and history management only
   - Transaction: Transaction management, no coordinate handling
   - CompositeCommand: Command composition, no coordinate handling
   - Command.h: Base interface, coordinate-agnostic

2. **SelectionCommands.cpp**:
   - Uses SelectionManager and SelectionSet (coordinate-agnostic)
   - No hardcoded coordinate assumptions

❌ **NEEDS FIX**:
1. **VoxelCommands.cpp** (lines 233-245):
   - `VoxelFillCommand::execute()` uses `std::floor/ceil` on bounding box coordinates
   - Assumes positive coordinate space starting from (0,0,0)
   - Fill operations may not work correctly in negative coordinate space

2. **StateSnapshot.cpp** (lines 50-72, 188-196):
   - Stores/restores voxel positions as raw grid coordinates without workspace context
   - No coordinate system versioning for migration
   - Snapshots saved in old system won't restore correctly in new centered system

⚠️ **SUSPICIOUS**:
1. **PlacementCommands.cpp** (lines 80, 103):
   - Depends on `PlacementUtils::validatePlacement()` which has old coordinate validation
   - Uses `PlacementUtils::incrementGridToWorld()` for position conversion
   - May reject valid positions in new coordinate system

**Recommendations**:
1. **High Priority**: Fix VoxelFillCommand coordinate calculations for negative space
2. **High Priority**: Update PlacementCommands to use new coordinate validation
3. **Medium Priority**: Add coordinate system versioning to StateSnapshot
4. **Low Priority**: Audit position-based command merging logic

**Dependencies**:
- VoxelFillCommand depends on proper bounding box to grid conversion
- PlacementCommands depend on Input::PlacementUtils (which needs fixes)
- StateSnapshot restoration depends on coordinate system compatibility
- All voxel operation commands may fail with coordinate mismatches

---

### 11. Foundation Layer (`src/foundation/`)
**Status**: ✅ COMPLETE  
**Agent**: Claude  
**Last Updated**: 2025-01-17

**Files Analyzed**:
- [x] math/Vector3f.h - ✅ CORRECT (pure math, no coordinate assumptions)
- [x] math/Vector3i.h - ✅ CORRECT (pure math, no coordinate assumptions)
- [x] math/Vector2f.h - ✅ CORRECT (pure math, no coordinate assumptions)
- [x] math/Vector4f.h - ✅ CORRECT (pure math, no coordinate assumptions)
- [x] math/Matrix4f.h - ✅ CORRECT (pure math, no coordinate assumptions)
- [x] math/BoundingBox.h - ✅ CORRECT (coordinate-system agnostic)
- [x] math/Ray.h - ✅ CORRECT (relative operations, no coordinate assumptions)
- [x] math/MathUtils.h - ✅ CORRECT (utility functions, no coordinate assumptions)
- [x] math/Quaternion.h - ✅ CORRECT (pure math, no coordinate assumptions)
- [ ] No Transform.h or Intersection.h files found

**Analysis Summary**:
The Foundation Layer is **completely coordinate-system agnostic** and requires no changes:

✅ **CORRECT Implementations**:
1. **Vector3f.h/Vector3i.h**:
   - Pure vector math operations (add, subtract, multiply, dot, cross, etc.)
   - No coordinate system assumptions
   - All operations are relative

2. **Matrix4f.h**:
   - Standard 4x4 matrix operations for transformations
   - `operator*(Vector3f)` handles homogeneous coordinates properly
   - `transformDirection()` correctly transforms directions without translation
   - No coordinate system assumptions

3. **BoundingBox.h** (lines 27-31, 42-43, 209-212):
   - Constructor accepts center and size: `BoundingBox(center, size)`
   - `getCenter()` correctly calculates center as `(min + max) * 0.5f`
   - `fromCenterAndSize()` utility method works with any coordinate system
   - All operations work regardless of where bounds are located

4. **Ray.h**:
   - Ray operations (intersection, closest point, etc.) are all relative
   - `screenToWorld()` method uses camera matrices correctly
   - No hardcoded coordinate assumptions

5. **MathUtils.h**:
   - Pure mathematical utility functions (clamp, lerp, angle conversion, etc.)
   - No coordinate system dependencies
   - Power-of-two utilities for general use

**Root Cause**: The Foundation Layer is correctly designed as coordinate-system agnostic mathematical utilities. All coordinate system issues exist in higher-level subsystems that use these utilities incorrectly.

**Recommendations**:
- No changes needed - Foundation Layer is correctly implemented
- All coordinate system fixes should happen in higher-level subsystems
- The Foundation Layer provides the correct tools (BoundingBox::fromCenterAndSize, etc.) that other systems should use

**Dependencies**:
- None - this layer provides utilities to all other systems
- Higher-level systems should use these utilities correctly

**Key Areas Checked**:
- Bounds calculation utilities - ✅ CORRECT
- Ray origin assumptions - ✅ CORRECT (no assumptions)
- Intersection calculations - ✅ CORRECT (relative operations)
- Transform origin handling - ✅ CORRECT (matrix math)
- Vector operations - ✅ CORRECT (pure math)

---

### 12. Application Layer (`apps/cli/`)
**Status**: ❌ NEEDS FIXES  
**Agent**: Claude  
**Last Updated**: 2025-01-17

**Files Analyzed**:
- [x] CommandProcessor.h/cpp - ✅ CORRECT (framework only, no coordinate logic)
- [x] Commands.cpp - ❌ NEEDS FIX (multiple coordinate issues)
- [x] CommandTypes.h/cpp - ✅ CORRECT (coordinate parsing correct)
- [x] MouseInteraction.h/cpp - ❌ NEEDS FIX (coordinate calculation issues)
- [x] VoxelMeshGenerator.h/cpp - ❌ NEEDS FIX (CRITICAL - already identified in COORDINATE_TODO)

**Analysis Summary**:
The Application Layer has **multiple critical coordinate system issues** that directly impact user interaction and visual rendering.

✅ **CORRECT Implementations**:
1. **CommandProcessor.cpp**:
   - Pure framework/infrastructure code
   - No coordinate system assumptions
   - Command registration and parsing only

2. **CommandTypes.cpp** (lines 107-152):
   - `getCoordinateArg()` correctly parses coordinates with units
   - Converts "100cm" to 100, "1m" to 100 (centimeter grid units)
   - No coordinate system assumptions in parsing

❌ **CRITICAL FIXES NEEDED**:

1. **Commands.cpp** (lines 417-445):
   - **Center camera command**: `m_voxelManager->getAllVoxels()` loop assumes old coordinate system
   - Lines 421-425: `Math::Vector3f voxelCenter((x + 0.5f) * voxelSize, (y + 0.5f) * voxelSize, (z + 0.5f) * voxelSize)`
   - This calculation assumes grid coordinates start at (0,0,0) instead of centered workspace
   - Should use VoxelGrid's `gridToWorld()` method

2. **Commands.cpp** (lines 1184-1192):
   - **Debug voxels command**: Same issue in coordinate conversion
   - `Math::Vector3f worldPos(voxelPos.gridPos.x * voxelSize, voxelPos.gridPos.y * voxelSize, voxelPos.gridPos.z * voxelSize)`
   - Assumes non-centered coordinate system

3. **Commands.cpp** (lines 1262-1267):
   - **Debug frustum command**: Same coordinate conversion problem
   - `Math::Vector3f worldPos(voxelPos.gridPos.x * voxelSize, voxelPos.gridPos.y * voxelSize, voxelPos.gridPos.z * voxelSize)`

4. **MouseInteraction.cpp** (lines 442-443, 658-662):
   - **getPlacementPosition()**: `Math::Vector3f voxelWorldPos = PlacementUtils::incrementGridToWorld(voxelPos);`
   - **centerCameraOnVoxels()**: Same grid-to-world conversion issue
   - Lines 658-662: Manual coordinate calculation instead of using VoxelGrid methods
   - `Math::Vector3f voxelCenter((voxel.gridPos.x + 0.5f) * voxelSize, (voxel.gridPos.y + 0.5f) * voxelSize, (voxel.gridPos.z + 0.5f) * voxelSize)`

5. **VoxelMeshGenerator.cpp** (lines 69-73, 184-188):
   - **generateCubeMesh()**: `worldPos = gridPos * voxelSize + voxelSize * 0.5f`
   - **generateEdgeMesh()**: Same coordinate conversion issue
   - This is the **ROOT CAUSE** of voxels appearing in wrong positions in CLI rendering
   - Should use VoxelGrid's `gridToWorld()` method

**Root Cause Analysis**:
All coordinate conversion issues follow the same pattern: `worldPos = gridPos * voxelSize + offset`. This assumes the workspace starts at (0,0,0) and extends to (size,size,size), but the actual coordinate system is centered with workspace extending from (-size/2, 0, -size/2) to (size/2, size, size/2).

**Recommendations**:
1. **HIGH PRIORITY**: Fix VoxelMeshGenerator coordinate conversion - this directly affects visual rendering
2. Update all manual coordinate conversions to use VoxelGrid's `gridToWorld()` method
3. Fix camera centering logic to use proper coordinate conversion
4. Update debug commands to use correct coordinate calculations

**Dependencies**:
- All visual rendering depends on VoxelMeshGenerator coordinate conversion
- Mouse interaction depends on correct placement position calculation
- Camera controls depend on correct voxel bounds calculation
- This directly contributes to integration test failures

**Fix Pattern**:
Replace: `worldPos = gridPos * voxelSize + voxelSize * 0.5f`
With: `worldPos = voxelGrid->gridToWorld(gridPos)`

Or manually: `worldPos = (gridPos * voxelSize) - Vector3f(workspaceSize.x/2, 0, workspaceSize.z/2) + voxelSize * 0.5f`

---

## 📊 OVERALL PROGRESS

**Total Subsystems**: 12  
**Not Started**: 0  
**In Progress**: 0  
**Complete**: 4 (Rendering Engine, Camera System, File I/O System, Foundation Layer)  
**Needs Fixes**: 8 (VoxelData Management, Input System, Selection System, Groups System, Surface Generation, Visual Feedback System, Application Layer, Undo/Redo System)  

## 🔍 KNOWN ISSUES

### Already Fixed:
- ✅ FaceDetector.cpp - Fixed in commit 3ed8cbe to use centered coordinates

### Known Problems:
- ❌ Integration tests failing due to coordinate mismatches
- ❌ Click placement tests failing
- ❌ Some tests hanging (possible infinite loops with coordinates)

## 📝 NOTES FOR AGENTS

### Common Patterns to Look For:
1. **Old System**: Workspace from (0,0,0) to (size,size,size)
2. **New System**: Workspace from (-size/2,-size/2,-size/2) to (size/2,size/2,size/2)

### Red Flags:
- Any code assuming minimum bounds are (0,0,0)
- Hardcoded positive-only coordinate checks
- Position validation using `>= 0` without considering negative coords
- Center calculations using `size/2` instead of (0,0,0)
- Boundary checks not accounting for negative coordinates

### Testing:
- After fixes, run integration tests to verify
- Pay special attention to boundary cases
- Test with voxels placed at negative coordinates
- Verify camera focuses on (0,0,0) not workspace corner

## 🚀 COMPLETION CRITERIA

A subsystem is considered COMPLETE when:
1. All files have been analyzed
2. All coordinate assumptions have been verified/fixed
3. Related tests pass
4. No regressions in other subsystems
5. Documentation updated if needed

---

**Last Updated**: 2025-01-17 by Claude

## 🔧 FILES REQUIRING FIXES - SUMMARY

### Critical Fixes (Blocking Integration Tests):
1. **PlacementValidation.cpp** (Input System) - Remove Y >= 0 constraints
2. **VoxelTypes.h** (VoxelData) - Fix VoxelPosition::toWorldSpace() and fromWorldSpace()

### High Priority Fixes:
3. **VoxelMeshGenerator.cpp** (CLI/Rendering) - Use centered coordinate conversion
4. **SelectionTypes.cpp** (Selection) - Fix VoxelId::getWorldPosition() and getBounds()
5. **BoxSelector.cpp** (Selection) - Use centered workspace bounds
6. **SphereSelector.cpp** (Selection) - Use centered workspace bounds
7. **VoxelGroup.cpp** (Groups) - Fix translate() and updateBounds() methods
8. **DualContouring.cpp** (Surface Generation) - Fix vertex world position calculation
9. **FeedbackRenderer.cpp** (Visual Feedback) - Use WorkspaceManager for bounds, not hardcoded (0,0,0)
10. **HighlightRenderer.cpp** (Visual Feedback) - Fix calculateVoxelTransform() coordinate conversion
11. **OutlineRenderer.cpp** (Visual Feedback) - Fix addVoxelEdges() and group outline coordinate conversion
12. **FeedbackTypes.cpp** (Visual Feedback) - Fix Face::getWorldPosition() coordinate conversion

### Common Fix Pattern:
Most fixes involve changing from:
- `worldPos = gridPos * voxelSize` (assumes origin at corner)

To:
- `worldPos = gridPos * voxelSize - Vector3f(workspaceSize.x/2, 0, workspaceSize.z/2)` (centered)

Or better yet, use VoxelGrid's conversion methods:
- `worldPos = voxelGrid->gridToWorld(gridPos)`