#pragma once
// GLAD stub - replace with actual GLAD generated files

// Don't include OpenGL headers directly - let MacOSGLLoader handle it on macOS
#ifndef __APPLE__
#include <GL/gl.h>
#endif

typedef void* (*GLADloadproc)(const char *name);
int gladLoadGLLoader(GLADloadproc load);
