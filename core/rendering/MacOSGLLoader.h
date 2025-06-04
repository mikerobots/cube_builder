#pragma once

#ifdef __APPLE__

#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#include <dlfcn.h>

namespace VoxelEditor {
namespace Rendering {

// Function pointer types for VAO functions
typedef void (*PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
typedef void (*PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint *arrays);
typedef void (*PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef GLboolean (*PFNGLISVERTEXARRAYPROC)(GLuint array);

// Global function pointers
extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
extern PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
extern PFNGLISVERTEXARRAYPROC glIsVertexArray;

// Loader function
bool LoadOpenGLExtensions();

} // namespace Rendering
} // namespace VoxelEditor

#endif // __APPLE__