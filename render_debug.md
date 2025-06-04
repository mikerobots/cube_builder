# Voxel Rendering Debug Plan

## Context
The voxel editor is not displaying voxels on screen despite:
- Mesh generation producing correct vertex data (24 vertices per voxel)
- Vertex transformations calculating correct screen positions
- OpenGL draw calls being executed with proper parameters
- Shaders compiling and linking successfully

The symptoms indicate that while the rendering pipeline is executing, the final pixels are not being written to the framebuffer.

## Debug Strategy
We will systematically validate each component of the rendering pipeline from bottom to top, ensuring each layer works correctly before moving to the next.

## Phase 1: Core System Validation

### 1.1 Voxel Data Layer (`core/voxel_data/`)
- [ ] **Run existing tests**: `ctest -R VoxelGrid`
- [ ] **Add test**: Verify voxel positions match expected world coordinates
- [ ] **Add logging**: Log voxel count and positions when queried
- [ ] **Validate**: Resolution calculations are correct (0.08m for 8cm voxels)

### 1.2 Math Library (`foundation/math/`)
- [ ] **Run existing tests**: `ctest -R Matrix4f`
- [ ] **Add test**: Matrix multiplication order (MVP = P * V * M)
- [ ] **Add test**: Perspective projection with known values
- [ ] **Add test**: LookAt matrix generation
- [ ] **Validate**: All transformations match expected OpenGL conventions

### 1.3 Camera System (`core/camera/`)
- [ ] **Run existing tests**: `ctest -R Camera`
- [ ] **Add test**: Verify view matrix for isometric preset
- [ ] **Add test**: Verify projection matrix with aspect ratio
- [ ] **Add logging**: Log camera position, target, and matrices
- [ ] **Add CLI command**: `debug camera` to print camera state

### 1.4 Surface Generation (`core/surface_gen/`)
- [ ] **Run existing tests**: `ctest -R MeshBuilder`
- [ ] **Add test**: Verify cube mesh has 24 unique vertices
- [ ] **Add test**: Verify vertex winding order (counter-clockwise)
- [ ] **Add test**: Verify normal directions (outward facing)
- [ ] **Add logging**: Log vertex positions and normals

## Phase 2: Rendering Pipeline Validation

### 2.1 OpenGL Renderer (`core/rendering/OpenGLRenderer`)
- [ ] **Add test**: Create minimal shader program
- [ ] **Add test**: Set and retrieve uniform values
- [ ] **Add test**: Vertex buffer creation and binding
- [ ] **Add logging**: Log all OpenGL state changes
- [ ] **Add logging**: Check glGetError after each operation
- [ ] **Validate**: Depth test, blending, culling states

### 2.2 Shader Manager (`core/rendering/ShaderManager`)
- [ ] **Add test**: Compile vertex shader independently
- [ ] **Add test**: Compile fragment shader independently
- [ ] **Add test**: Link simple pass-through shader
- [ ] **Add logging**: Log shader compilation/link info
- [ ] **Create test shader**: Minimal shader that outputs constant color

### 2.3 Render Engine (`core/rendering/RenderEngine`)
- [ ] **Add test**: Render single triangle with known coordinates
- [ ] **Add test**: Verify MVP matrix multiplication
- [ ] **Add logging**: Log all uniform values being set
- [ ] **Add logging**: Log viewport and clear operations
- [ ] **Validate**: Frame buffer operations in correct order

## Phase 3: Integration Testing

### 3.1 CLI Application Debug Commands
- [ ] **Add command**: `debug voxels` - List all voxel positions
- [ ] **Add command**: `debug camera` - Show camera matrices and position
- [ ] **Add command**: `debug render` - Show render statistics
- [ ] **Add command**: `debug frustum` - Test if voxels are in frustum
- [ ] **Add command**: `test triangle` - Render test triangle with shader

### 3.2 Incremental Rendering Tests
1. [ ] **Test 1**: Clear to solid color (no rendering)
2. [ ] **Test 2**: Render immediate mode triangle (no shader)
3. [ ] **Test 3**: Render shader triangle with identity MVP
4. [ ] **Test 4**: Render shader triangle with view matrix only
5. [ ] **Test 5**: Render shader triangle with full MVP
6. [ ] **Test 6**: Render single voxel at origin
7. [ ] **Test 7**: Render voxel at actual position

### 3.3 State Debugging
- [ ] **Check depth buffer**: Disable depth test temporarily
- [ ] **Check blending**: Ensure alpha blending is off
- [ ] **Check culling**: Disable backface culling
- [ ] **Check viewport**: Verify viewport matches window size
- [ ] **Check scissor test**: Ensure scissor test is disabled

## Phase 4: Shader Debugging

### 4.1 Progressive Shader Simplification
1. [ ] **Shader 1**: Output position directly as color
2. [ ] **Shader 2**: Fixed position, fixed color
3. [ ] **Shader 3**: Transform position, fixed color
4. [ ] **Shader 4**: Full MVP transform, fixed color
5. [ ] **Shader 5**: Full MVP transform, vertex color

### 4.2 Shader Validation
- [ ] **Verify attributes**: Check attribute locations match expectations
- [ ] **Verify uniforms**: Print all uniform locations
- [ ] **Test coordinates**: Output NDC coordinates as colors
- [ ] **Test clipping**: Ensure vertices aren't being clipped

## Phase 5: Screenshot Analysis

### 5.1 PPM Output Validation
- [ ] **Add command**: `screenshot analyze` - Count colored pixels
- [ ] **Add debug overlay**: Draw coordinate axes
- [ ] **Add debug overlay**: Draw frustum bounds
- [ ] **Compare outputs**: Immediate mode vs shader rendering

## Implementation Order

1. **Start with Phase 1.3**: Add camera debug command
2. **Then Phase 3.1**: Add all debug commands
3. **Run Phase 1**: Validate all core systems with tests
4. **Run Phase 2.1-2.2**: Validate OpenGL and shaders
5. **Run Phase 3.2**: Incremental rendering tests
6. **Run Phase 4**: Shader debugging if needed
7. **Run Phase 5**: Final validation with screenshots

## Success Criteria

The debugging is complete when:
1. Yellow voxels appear in the screenshot at expected positions
2. All unit tests pass
3. Debug commands show consistent state
4. No OpenGL errors are reported

## Common Issues to Check

1. **Matrix multiplication order**: OpenGL uses column-major matrices
2. **Coordinate systems**: Ensure Y-up convention is consistent
3. **Depth range**: OpenGL uses [-1, 1] for NDC depth
4. **Attribute binding**: Must happen before shader linking
5. **Buffer binding**: Current buffer affects subsequent operations
6. **State leakage**: Previous operations may affect current state

## Running the Debug Plan

```bash
# 1. Build with debug logging
cmake -B build_debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build_debug

# 2. Run core tests
cd build_debug && ctest -V

# 3. Run CLI with debug commands
./voxel-cli
> debug camera
> debug voxels
> place 2 2 2
> debug render
> screenshot debug.ppm

# 4. Analyze results
python3 analyze_debug.py debug.ppm
```

This methodical approach will identify exactly where the rendering pipeline is failing.