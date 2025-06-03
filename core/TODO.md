# Core Subsystems Review TODO

## Camera Subsystem
- [X] Camera.h - Interface looks correct
- [X] CameraController.h - Interface looks correct  
- [X] OrbitCamera.h - Interface looks correct
- [X] Viewport.h - Interface looks correct
- [X] **CRITICAL BUG: FOV conversion issue in Camera.h** ✅ FIXED
  - Line 143: Passing degrees to Matrix4f::perspective() which expects radians
  - This causes extremely narrow FOV (~2.6° instead of 45°)
  - **Fix**: Convert m_fov to radians: `Math::Matrix4f::perspective(m_fov * M_PI / 180.0f, ...)`
  - **FIXED**: Added `Math::toRadians(m_fov)` in updateProjectionMatrix()
- [X] **Matrix inverse safety issue in Viewport.h**
  - Line 88: Matrix inverse could fail silently
  - **Fix**: Add determinant check before inverse operation

## Rendering Subsystem
- [X] OpenGLRenderer.cpp/h - **MAJOR ISSUE: Completely stubbed implementation**
  - createVertexBuffer() - TODO only
  - createIndexBuffer() - TODO only
  - bindVertexBuffer() - Empty
  - bindIndexBuffer() - Empty
  - drawElements() - Empty
  - useProgram() - Empty
  - setUniform() - Empty
  - **This is why voxels aren't rendering!**
- [X] RenderEngine.cpp/h - Minimal implementation, mostly TODOs
- [X] ShaderManager.h - Good implementation but NOT USED
- [X] RenderTypes.cpp/h - Data structures look correct
- [X] shaders/basic_voxel.vert - Shader looks correct
- [X] shaders/basic_voxel.frag - Shader looks correct

### Key Finding
The CLI Application is bypassing the broken rendering subsystem and using direct OpenGL calls. This works for debug rendering but has issues with voxel rendering.

## Other Subsystems to Review
- [ ] Voxel Data
- [ ] Visual Feedback
- [ ] Surface Generation
- [ ] Selection
- [ ] Groups
- [ ] File I/O
- [ ] Input
- [ ] Undo/Redo

## Priority Fixes for Voxel Rendering
1. **Fix FOV conversion bug in Camera.h** (most critical)
2. Either:
   - Complete OpenGLRenderer implementation, OR
   - Fix the direct OpenGL implementation in CLI Application
3. Verify vertex attribute bindings match between CPU and GPU
4. Check matrix uniform uploads