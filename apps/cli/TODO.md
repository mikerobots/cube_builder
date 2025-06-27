# CLI Application TODO

## Overview
This document tracks pending work for the CLI application, particularly the implementation of surface smoothing features that are documented but not yet functional.

## Implementation Status (Updated: Current Session)

### ✅ Completed in This Session
1. **Smoothing Commands Implemented**:
   - `smooth` - Display current smoothing settings
   - `smooth <level>` - Set smoothing level (0-10+) with auto algorithm selection
   - `smooth preview on|off` - Toggle preview mode (flag stored, rendering not yet implemented)
   - `smooth algorithm <type>` - Manually set algorithm (laplacian/taubin/bilaplacian)

2. **Mesh Commands Implemented**:
   - `mesh validate` - Validates mesh for watertight/manifold properties
   - `mesh info` - Shows mesh statistics including smoothing info
   - `mesh repair` - Placeholder for future repair functionality

3. **Export Enhancement**:
   - Export command now applies smoothing when level > 0
   - Progress display during smoothing operation
   - Proper error handling

4. **Testing**:
   - Unit tests for all smoothing commands (test_unit_cli_smoothing_commands.cpp)
   - Integration tests for smooth export workflow (test_integration_cli_smoothing_export.cpp)
   - CMakeLists.txt updated to include new tests

5. **Status Command Enhancement**:
   - Shows current smoothing settings in status output

### ⚠️ Pending Dependencies
- Real-time preview rendering (requires renderer integration)
- MeshBuilder for advanced repair functions
- Performance optimization for large meshes

## Current Status

### ✅ Implemented and Working
- All core voxel operations (place, delete, fill)
- Complete camera and view control  
- File save/load with compression
- Basic STL export (blocky geometry only)
- Mouse interaction with preview
- Selection and grouping
- Undo/redo system
- Auto-completion and help
- Debug visualization modes (ray, grid)

### ❌ Not Implemented (But Documented)
- Mesh smoothing commands (`smooth`, `smooth preview`)
- Mesh validation (`mesh validate`, `mesh info`, `mesh repair`)
- Smoothing algorithm selection
- Real-time smoothing preview

## Implementation Plan

### Phase 1: Remove Non-Functional Commands (Option A)
**Pros**: Honest about current capabilities
**Cons**: Breaking change for users expecting features

1. [ ] Remove smooth commands from CommandTypes enum
2. [ ] Remove smooth command handlers from Commands.cpp
3. [ ] Update CLI_GUIDE.md to remove smoothing section
4. [ ] Update help text

### Phase 2: Implement Smoothing (Option B - Recommended) ✅ COMPLETED
**Pros**: Delivers promised functionality
**Cons**: Requires surface_gen work to be completed first

1. [x] Wait for surface_gen smoothing implementation (see core/surface_gen/TODO.md)
2. [x] Add SmoothingSettings to Application state
3. [x] Implement smooth command handlers:
   - [x] `smooth` - Display current level
   - [x] `smooth <level>` - Set smoothing level (0-10)
   - [x] `smooth preview on/off` - Toggle preview (store flag only for now)
   - [x] `smooth algorithm <type>` - Set algorithm type
4. [x] Update export command to use smoothing:
   - Integrated MeshSmoother into export workflow
   - Added progress display during smoothing
   - Proper error handling for failed smoothing
5. [x] Implement mesh commands:
   - [x] `mesh validate` - Calls MeshValidator (when available)
   - [x] `mesh info` - Display vertex/triangle count
   - [x] `mesh repair` - Placeholder for repair functions

### Phase 3: Real-time Preview (Future)
1. [ ] Add preview mesh generation on smoothing change
2. [ ] Render preview with transparency
3. [ ] Performance optimization for <100ms updates
4. [ ] Toggle between preview and actual

## Technical Considerations

### Current Export Pipeline
```
VoxelDataManager → SurfaceGenerator::generateMesh() → STLExporter
                            ↓
                   (Returns blocky mesh)
```

### Required Export Pipeline
```
VoxelDataManager → SurfaceGenerator::generateMesh() → MeshSmoother::smooth() → STLExporter
                            ↓                                ↓
                   (With SurfaceSettings)         (Not implemented yet)
```

### Integration Points
1. **SurfaceSettings struct** needs:
   - `int smoothingLevel` (0-10)
   - `SmoothingAlgorithm algorithm` (enum)
   - `bool preserveTopology` (default true)

2. **Application state** needs:
   - `m_smoothingLevel` member
   - `m_smoothingAlgorithm` member  
   - `m_smoothPreviewEnabled` member

3. **Command handlers** need:
   - Parsing for numeric smoothing levels
   - Validation (0-10 range)
   - Algorithm name mapping

## Dependencies

### Blocked By
- [ ] core/surface_gen MeshSmoother implementation
- [ ] core/surface_gen MeshValidator implementation
- [ ] core/surface_gen TopologyPreserver implementation

### Not Blocked
- [ ] Command parsing infrastructure (ready)
- [ ] Settings storage (can implement now)
- [ ] UI/UX flow (already documented)

## Testing Requirements ✅ COMPLETED

### Unit Tests Needed
- [x] Smooth command parsing and validation (test_unit_cli_smoothing_commands.cpp)
- [x] Settings persistence across commands
- [x] Export with smoothing settings

### Integration Tests Needed  
- [x] Full workflow: build → smooth → export (test_integration_cli_smoothing_export.cpp)
- [x] Smoothing level comparison (when implemented)
- [ ] Performance benchmarks for preview (deferred until preview implemented)

### End-to-End Tests
- [ ] Visual validation of smoothed exports (requires manual testing)
- [ ] STL compatibility with 3D printing software (requires external validation)

## Recommendations

1. **Short Term**: ✅ DONE - Smoothing commands are now implemented and functional
2. **Medium Term**: Focus on real-time preview rendering integration
3. **Long Term**: Add GPU acceleration and advanced smoothing algorithms

## Notes

- Users expect smoothing based on CLI_GUIDE.md documentation
- Current exports are functional but not suitable for 3D printing toys
- External tools (Blender, MeshLab) can smooth STL files as workaround
- Consider adding a startup message about features in development