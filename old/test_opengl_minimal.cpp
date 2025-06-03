#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW" << std::endl;
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
    
    auto* window = glfwCreateWindow(800, 600, "OpenGL Test", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    // Since GLAD is a stub, load functions manually
    typedef void (APIENTRYP PFNGLGENVERTEXARRAYSPROC) (GLsizei n, GLuint *arrays);
    typedef void (APIENTRYP PFNGLBINDVERTEXARRAYPROC) (GLuint array);
    auto glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) glfwGetProcAddress("glGenVertexArrays");
    auto glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC) glfwGetProcAddress("glBindVertexArray");
    
    const char* vertSrc = R"(
#version 330 core
void main() {
    vec2 verts[3] = vec2[3](vec2(-0.5,-0.5), vec2(0.5,-0.5), vec2(0,0.5));
    gl_Position = vec4(verts[gl_VertexID], 0, 1);
}
)";
    
    const char* fragSrc = R"(
#version 330 core
out vec4 color;
void main() {
    color = vec4(1,0,0,1);
}
)";
    
    auto vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertSrc, nullptr);
    glCompileShader(vs);
    
    auto fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragSrc, nullptr);
    glCompileShader(fs);
    
    auto prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    
    GLuint vao;
    glGenVertexArrays(1, &vao);
    
    bool saved = false;
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        if (!saved) {
            // Save screenshot
            GLubyte pixels[800*600*3];
            glReadPixels(0, 0, 800, 600, GL_RGB, GL_UNSIGNED_BYTE, pixels);
            
            FILE* f = fopen("minimal_test.ppm", "wb");
            fprintf(f, "P6\n800 600\n255\n");
            // Flip vertically
            for (int y = 599; y >= 0; y--) {
                fwrite(&pixels[y*800*3], 1, 800*3, f);
            }
            fclose(f);
            saved = true;
            std::cout << "Saved minimal_test.ppm" << std::endl;
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    return 0;
}