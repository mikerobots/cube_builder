#pragma once
// GLAD stub - replace with actual GLAD generated files
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
typedef void* (*GLADloadproc)(const char *name);
int gladLoadGLLoader(GLADloadproc load);
