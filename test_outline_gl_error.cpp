#include <iostream>
#include <vector>
#include <GLFW/glfw3.h>

#ifdef __APPLE__
    #include <OpenGL/gl3.h>
#else
    #include <GL/gl.h>
#endif

// Simple test to reproduce the GL_INVALID_VALUE error

void checkGLError(const char* operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "GL Error after " << operation << ": " << error;
        if (error == GL_INVALID_VALUE) {
            std::cerr << " (GL_INVALID_VALUE)";
        }
        std::cerr << std::endl;
    }
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Set OpenGL version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(800, 600, "GL Error Test", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    // Clear any existing errors
    while (glGetError() != GL_NO_ERROR) {}
    
    // Create VAO
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    checkGLError("glBindVertexArray");
    
    // Create buffers
    GLuint vbo, ibo;
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);
    
    // Setup vertex data (24 vertices for box outline)
    float vertices[24 * 3]; // 24 vertices, 3 floats each
    for (int i = 0; i < 24 * 3; i++) {
        vertices[i] = 0.0f;
    }
    
    // Setup index data (24 indices for 12 line segments)
    unsigned int indices[24];
    for (unsigned int i = 0; i < 24; i++) {
        indices[i] = i;
    }
    
    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    checkGLError("glBufferData vertices");
    
    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    checkGLError("glBufferData indices");
    
    // Set vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    checkGLError("glVertexAttribPointer");
    
    // Try to draw with 24 indices
    std::cout << "Drawing with 24 indices (should work)..." << std::endl;
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);
    checkGLError("glDrawElements with 24 indices");
    
    // Now try with an invalid scenario - more indices than vertices
    unsigned int badIndices[24];
    for (unsigned int i = 0; i < 24; i++) {
        badIndices[i] = i + 20; // These will exceed vertex count
    }
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(badIndices), badIndices, GL_STATIC_DRAW);
    checkGLError("glBufferData bad indices");
    
    std::cout << "Drawing with bad indices (should fail)..." << std::endl;
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);
    checkGLError("glDrawElements with bad indices");
    
    // Clean up
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
    glDeleteVertexArrays(1, &vao);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}