# Ray Visualization Debug Checklist

## 1. Shader Validation

### 1.1 Vertex Shader
- [ ] Verify shader compilation succeeds without warnings
- [ ] Check that vertex shader actually receives position data
- [ ] Validate that mvpMatrix uniform location is not -1
- [ ] Confirm gl_Position calculation: `gl_Position = mvpMatrix * vec4(position, 1.0)`
- [ ] Check if vertex shader outputs are correctly declared
- [ ] Verify shader version matches OpenGL context (#version 330 core)

### 1.2 Fragment Shader
- [ ] Verify fragment shader outputs yellow color: `color = vec4(1.0, 1.0, 0.0, 1.0)`
- [ ] Check that output variable name matches (out vec4 color)
- [ ] Ensure no discard statements that might cull fragments
- [ ] Verify shader compilation succeeds without warnings

### 1.3 Shader Program Linking
- [ ] Confirm program linking succeeds
- [ ] Verify glUseProgram actually binds the correct shader
- [ ] Check that vertex attributes are at expected locations (0 for position, 1 for color)
- [ ] Validate program with glValidateProgram before use

## 2. Vertex Data Validation

### 2.1 Ray Generation
- [ ] Verify ray origin is reasonable (camera position)
- [ ] Confirm ray direction is normalized
- [ ] Check ray length (currently 50.0f) puts end point in view frustum
- [ ] Validate start and end points are different
- [ ] Print exact coordinates of start/end points

### 2.2 Vertex Buffer Data
- [ ] Verify vertex data is correctly interleaved (x,y,z,r,g,b,a per vertex)
- [ ] Check that exactly 2 vertices are added for the line
- [ ] Confirm vertex data size matches expected (2 vertices * 7 floats * 4 bytes)
- [ ] Validate color values are all in [0,1] range
- [ ] Check vertex data is not NaN or infinity

### 2.3 Buffer Upload
- [ ] Verify buffer was resized if needed (check our resize code)
- [ ] Confirm glBufferSubData offset is 0
- [ ] Check that data size in glBufferSubData matches actual data
- [ ] Validate no GL errors after buffer upload
- [ ] Verify buffer binding is correct (GL_ARRAY_BUFFER)

## 3. Vertex Attribute Setup

### 3.1 VAO Configuration
- [ ] Confirm VAO is created and bound
- [ ] Verify vertex attributes are enabled (glEnableVertexAttribArray)
- [ ] Check attribute 0 (position): size=3, type=GL_FLOAT, stride=28, offset=0
- [ ] Check attribute 1 (color): size=4, type=GL_FLOAT, stride=28, offset=12
- [ ] Ensure VAO is bound when drawing

### 3.2 Vertex Attribute Bindings
- [ ] Verify attribute locations match shader expectations
- [ ] Check that stride calculation is correct (7 * sizeof(float) = 28)
- [ ] Confirm offset calculations for each attribute
- [ ] Validate that attributes are not disabled elsewhere

## 4. Matrix Math Validation

### 4.1 Camera Matrices
- [ ] Print camera position and verify it's reasonable
- [ ] Print view matrix and check it's not identity
- [ ] Print projection matrix and verify it's not identity
- [ ] Verify view matrix inverse(camera transform) relationship
- [ ] Check projection matrix for reasonable FOV/aspect/near/far

### 4.2 MVP Matrix Calculation
- [ ] Verify matrix multiplication order: projection * view
- [ ] Check that matrices are compatible sizes (4x4)
- [ ] Print MVP matrix values to ensure not all zeros
- [ ] Verify no NaN or infinity values in MVP matrix

### 4.3 Matrix Upload
- [ ] Confirm GL_TRUE transpose flag (our matrices are row-major)
- [ ] Verify uniform location is valid (not -1)
- [ ] Check GL errors after uniform upload
- [ ] Test with simple identity matrix to isolate issues

### 4.4 Vertex Transformation
- [ ] Transform vertices manually and print results
- [ ] Check clip space coordinates (before perspective divide)
- [ ] Verify w component is not 0 (would cause division by zero)
- [ ] Calculate NDC coordinates (after perspective divide)
- [ ] Verify NDC coordinates are in [-1, 1] range
- [ ] Calculate screen coordinates and verify they're on screen

## 5. OpenGL State Validation

### 5.1 Render State
- [ ] Check current OpenGL context is valid
- [ ] Verify depth test setting (should be disabled for overlay)
- [ ] Confirm blend function (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
- [ ] Check if blending is enabled
- [ ] Verify cull face is disabled
- [ ] Check polygon mode (should not be GL_POINT or GL_FILL for lines)

### 5.2 Viewport and Scissor
- [ ] Print current viewport with glGetIntegerv(GL_VIEWPORT)
- [ ] Verify viewport matches window size
- [ ] Check if scissor test is enabled
- [ ] If scissor enabled, verify scissor box includes ray area
- [ ] Confirm viewport hasn't changed between setup and draw

### 5.3 Line Rendering State
- [ ] Check line width (glLineWidth) - note many drivers only support 1.0
- [ ] Verify GL_LINE_SMOOTH is appropriate for platform
- [ ] Check if line stipple is disabled (legacy, but worth checking)
- [ ] Confirm primitive type is GL_LINES

## 6. Draw Call Validation

### 6.1 glDrawArrays Call
- [ ] Verify primitive type is GL_LINES
- [ ] Confirm first = 0
- [ ] Check count = 2 (for one line)
- [ ] Ensure no GL errors before draw call
- [ ] Ensure no GL errors after draw call

### 6.2 GPU Validation
- [ ] Use glFinish() after draw to ensure GPU completion
- [ ] Check if GL_KHR_debug is available for GPU validation
- [ ] Enable debug output for additional GPU feedback

## 7. Framebuffer and Output Validation

### 7.1 Framebuffer State
- [ ] Verify rendering to default framebuffer (0)
- [ ] Check framebuffer completeness
- [ ] Confirm color attachment format supports alpha
- [ ] Verify no multisampling issues

### 7.2 Pixel Readback
- [ ] Confirm glReadPixels happens after all rendering
- [ ] Verify glReadPixels parameters (0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE)
- [ ] Check that pixel buffer size matches width*height*4
- [ ] Test reading a smaller region where line should be

### 7.3 Color Detection
- [ ] Print some pixel values where the line should be
- [ ] Verify yellow detection thresholds (R>200, G>200, B<50)
- [ ] Try broader color detection (any non-black pixels)
- [ ] Check if pixels are being read upside down (OpenGL vs screen coordinates)

## 8. Render Order Issues

### 8.1 Clear Operations
- [ ] Check if glClear is called after overlay rendering
- [ ] Verify clear color isn't yellow (masking the lines)
- [ ] Ensure depth buffer isn't cleared between overlay and readback

### 8.2 Rendering Pipeline Order
- [ ] Confirm overlay renders after main scene
- [ ] Verify no other rendering happens after overlay
- [ ] Check that feedback renderer's beginFrame/endFrame are called
- [ ] Ensure render window swap doesn't happen before pixel read

### 8.3 Multiple Render Passes
- [ ] Check if rendering happens to an FBO first
- [ ] Verify we're reading from the correct buffer
- [ ] Ensure all render passes complete before pixel read

## 9. Platform-Specific Issues

### 9.1 macOS Specific
- [ ] Check if VAO functions are loaded correctly
- [ ] Verify OpenGL context version (should be 3.3 core)
- [ ] Check for retina display scaling issues
- [ ] Verify forward compatibility context

### 9.2 Driver-Specific
- [ ] Test with different line widths (some drivers only support 1.0)
- [ ] Check for driver-specific line rendering bugs
- [ ] Verify no driver warnings in console

## 10. Simplified Test Cases

### 10.1 Minimal Line Test
- [ ] Render a single line with no transforms (identity MVP)
- [ ] Use clip space coordinates directly (-1,-1,0 to 1,1,0)
- [ ] Disable all state except the minimum needed
- [ ] Use solid color clear before rendering

### 10.2 Progressive Complexity
- [ ] Add view matrix only (no projection)
- [ ] Add projection matrix
- [ ] Add proper ray coordinates
- [ ] Enable blending
- [ ] Add other scene elements

### 10.3 Alternative Rendering
- [ ] Try rendering as GL_POINTS instead of GL_LINES
- [ ] Render a triangle strip to make a thick line
- [ ] Use a different color (red) to rule out color detection

## 11. Debugging Tools

### 11.1 RenderDoc
- [ ] Capture frame with RenderDoc
- [ ] Verify draw call is present
- [ ] Check vertex data in mesh viewer
- [ ] Inspect shader uniforms
- [ ] View framebuffer at each stage

### 11.2 OpenGL Debug Output
- [ ] Enable KHR_debug if available
- [ ] Set debug message callback
- [ ] Check for any warnings or errors

### 11.3 Visual Debugging
- [ ] Save framebuffer to image file
- [ ] Render to specific quadrant only
- [ ] Use different colors for start/end vertices
- [ ] Draw multiple parallel lines for visibility

## 12. Common Pitfalls to Check

- [ ] Coordinate system confusion (Y-up vs Y-down)
- [ ] Row-major vs column-major matrix storage
- [ ] Vertex winding order (though shouldn't matter for lines)
- [ ] Integer division in shader (use floating point)
- [ ] Shader precision issues (use highp)
- [ ] Z-fighting with depth test enabled
- [ ] Accidental early returns in render functions
- [ ] Exception handling hiding errors
- [ ] Thread safety issues if rendering is multithreaded
- [ ] Resource cleanup happening too early

## 13. Fix Verification

Once issue is found and fixed:
- [ ] Verify fix works with different ray angles
- [ ] Test with different camera positions
- [ ] Ensure fix doesn't break other rendering
- [ ] Check performance impact
- [ ] Verify fix works on all platforms
- [ ] Add regression test