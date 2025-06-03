# Pipeline Test Learnings

## Key Insights from Main App Debugging

### What Works ✅
1. **Triangle rendering with simple vertex format** - Confirmed working with identity matrices
2. **GLFW OpenGL 2.1 context creation** - Window creation and context work
3. **Shader compilation (GLSL 120)** - Both vertex and fragment shaders compile correctly
4. **Screenshot capture** - PPM format works, high DPI scaling detected (2560x1440)
5. **Manual vertex attribute setup** - VAOs not needed for OpenGL 2.1

### Issues Discovered ❌
1. **Complex vertex structures fail** - Voxel rendering with `Rendering::Vertex` doesn't work
2. **Potential vertex attribute stride issues** - 48-byte vertex struct vs simple array
3. **Camera transformation complexity** - MVP matrices might clip geometry outside view
4. **Color attribute mismatch** - vec3 vs vec4 confusion in shader/vertex setup

### Technical Details
- **OpenGL Version**: 2.1 Metal - 89.3 (macOS)
- **GLSL Version**: 1.20
- **Extensions**: GL_APPLE_vertex_array_object available but not used
- **Viewport**: 1280x720 logical, 2560x1440 physical (2x scaling)
- **Working test case**: Simple triangle with 9-float stride (pos+normal+color)

## Testing Strategy
1. Start with proven working case (simple triangle)
2. Incrementally add complexity
3. Test each feature in isolation
4. Document exact working parameters for each test

## Framework Design
Created isolated testing framework with:
- **Test 1**: Simple triangle (9-float stride: pos+normal+color)
- **Test 2**: Indexed quad with simple vertices (proves VBO/EBO works)
- **Test 3**: Complex cube with voxel-like vertices (48-byte stride with texCoords)

### Key Design Decisions
- Use same OpenGL setup as working triangle test
- Same shader (GLSL 120) as main app
- Automated screenshot analysis for pass/fail detection
- Shell scripts for automated building/testing

## TEST RESULTS ✅

### All Tests Pass Successfully!
1. **Simple Triangle**: 12.5% coverage (240k/1.92M pixels) ✅
2. **Simple Quad**: 25.0% coverage (480k/1.92M pixels) ✅  
3. **Complex Cube**: 25.0% coverage (480k/1.92M pixels) ✅

### Critical Discoveries:
- ✅ **48-byte vertex structures work perfectly**
- ✅ **Vec3 color reading from Vec4 data works**
- ✅ **Indexed rendering (VBO/EBO) works**
- ✅ **Complex cube geometry renders correctly**

### Implication:
**The voxel vertex format is NOT the issue in the main app!** 
The problem must be in:
- Camera transformation matrices (clipping voxels out of view)
- Shader uniform values in main app
- Different OpenGL state in main app context
- Voxel world positioning relative to camera