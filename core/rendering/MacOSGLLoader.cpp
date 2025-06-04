#ifdef __APPLE__

#include "MacOSGLLoader.h"
#include <iostream>

namespace VoxelEditor {
namespace Rendering {

// Global function pointers
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
PFNGLISVERTEXARRAYPROC glIsVertexArray = nullptr;

bool LoadOpenGLExtensions() {
    // Get the OpenGL framework handle
    void* openglFramework = dlopen("/System/Library/Frameworks/OpenGL.framework/OpenGL", RTLD_LAZY);
    if (!openglFramework) {
        std::cerr << "Failed to load OpenGL framework" << std::endl;
        return false;
    }

    // Load VAO functions
    glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)dlsym(openglFramework, "glGenVertexArrays");
    glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)dlsym(openglFramework, "glDeleteVertexArrays");
    glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)dlsym(openglFramework, "glBindVertexArray");
    glIsVertexArray = (PFNGLISVERTEXARRAYPROC)dlsym(openglFramework, "glIsVertexArray");

    // Check if all functions were loaded
    bool success = glGenVertexArrays && glDeleteVertexArrays && glBindVertexArray && glIsVertexArray;
    
    if (!success) {
        std::cerr << "Failed to load VAO functions:" << std::endl;
        if (!glGenVertexArrays) std::cerr << "  - glGenVertexArrays not found" << std::endl;
        if (!glDeleteVertexArrays) std::cerr << "  - glDeleteVertexArrays not found" << std::endl;
        if (!glBindVertexArray) std::cerr << "  - glBindVertexArray not found" << std::endl;
        if (!glIsVertexArray) std::cerr << "  - glIsVertexArray not found" << std::endl;
        
        // Try Apple-specific versions as fallback
        glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)dlsym(openglFramework, "glGenVertexArraysAPPLE");
        glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)dlsym(openglFramework, "glDeleteVertexArraysAPPLE");
        glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)dlsym(openglFramework, "glBindVertexArrayAPPLE");
        glIsVertexArray = (PFNGLISVERTEXARRAYPROC)dlsym(openglFramework, "glIsVertexArrayAPPLE");
        
        success = glGenVertexArrays && glDeleteVertexArrays && glBindVertexArray && glIsVertexArray;
        if (success) {
            std::cout << "Loaded Apple-specific VAO functions" << std::endl;
        }
    } else {
        std::cout << "Successfully loaded OpenGL VAO functions" << std::endl;
    }

    return success;
}

} // namespace Rendering
} // namespace VoxelEditor

#endif // __APPLE__