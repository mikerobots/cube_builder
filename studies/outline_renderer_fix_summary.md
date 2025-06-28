# OutlineRenderer GL_INVALID_VALUE Fix Summary

## Problem
The OutlineRenderer was generating GL_INVALID_VALUE (1281) errors after calling glDrawElements with 24 indices. This occurred during ray visualization tests when rendering box outlines.

## Root Causes Identified

1. **Batch Accumulation**: Batches were being added to `m_batches` but never cleared after rendering, causing old batches to accumulate and potentially reference stale data.

2. **Missing Index Validation**: No validation was performed to ensure indices were within the valid range of vertices before uploading to GPU or drawing.

3. **OpenGL Error State**: Previous OpenGL errors were not being cleared before operations, making it difficult to identify where errors actually occurred.

4. **Initialization Order**: The renderer was checking for data before ensuring initialization, which could lead to invalid OpenGL state.

## Fixes Applied

### 1. Clear Batches After Rendering
```cpp
// In renderBatch() after the render loop:
// Clear batches after rendering to prevent accumulation
m_batches.clear();
```

### 2. Add Index Validation
```cpp
// In renderBatch() when checking batches:
// Validate indices are within bounds
for (uint32_t idx : batch.indices) {
    if (idx >= batch.vertices.size()) {
        std::cerr << "OutlineRenderer: Invalid index " << idx 
                 << " >= vertex count " << batch.vertices.size() << std::endl;
        return;
    }
}

// In updateBuffers() before uploading:
// Validate indices before uploading
for (uint32_t idx : batch.indices) {
    if (idx >= batch.vertices.size()) {
        std::cerr << "OutlineRenderer::updateBuffers: Invalid index " << idx 
                 << " >= vertex count " << batch.vertices.size() << std::endl;
        return;
    }
}
```

### 3. Proper Error Handling
```cpp
// Clear previous errors at the start of renderBatch()
while (glGetError() != GL_NO_ERROR) {}

// Better error handling for buffer size queries
GLint bufferSize = 0;
glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
GLenum error = glGetError();
if (error != GL_NO_ERROR) {
    std::cerr << "OutlineRenderer::updateBuffers: Error getting buffer size: " << error << std::endl;
    bufferSize = 0; // Force reallocation
}
```

### 4. Fix Initialization Order
```cpp
// Ensure initialization before checking batch data
ensureInitialized();
```

## How the Fix Works

1. **Prevents Stale Data**: By clearing batches after each render, we ensure that old batch data doesn't accumulate and cause issues in subsequent frames.

2. **Early Error Detection**: Index validation catches invalid indices before they reach OpenGL, preventing GL_INVALID_VALUE errors from glDrawElements.

3. **Clean Error State**: Clearing OpenGL errors at the start ensures we can accurately track where new errors occur.

4. **Robust Buffer Management**: Better error handling for buffer operations ensures we don't use invalid buffer sizes.

## Testing the Fix

The fix should resolve GL_INVALID_VALUE errors when:
- Running ray visualization tests
- Rendering box outlines with 24 indices (12 edges × 2 vertices per edge)
- Using OutlineRenderer in non-batch mode

## Additional Improvements

Consider these future improvements:
1. Add debug assertions in debug builds to catch index issues early
2. Implement a maximum batch size to prevent excessive memory usage
3. Add performance metrics to track batch sizes and render times
4. Consider using a persistent mapped buffer for better performance