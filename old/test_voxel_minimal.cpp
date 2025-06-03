// Minimal test that exactly mimics voxel rendering
#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Exact vertex structure from Rendering::Vertex
struct Vertex {
    float position[3];  // 0
    float normal[3];    // 12
    float texCoords[2]; // 24
    float color[4];     // 32
};  // Total: 48 bytes

int main() {
    if (!glfwInit()) return -1;
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Voxel Test", nullptr, nullptr);
    if (!window) return -1;
    
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
    
    // Exact shader from main app
    const char* vertSrc = 
        "#version 120\n"
        "attribute vec3 aPos;\n"
        "attribute vec3 aNormal;\n"
        "attribute vec3 aColor;\n"
        "\n"
        "varying vec3 FragPos;\n"
        "varying vec3 Normal;\n"
        "varying vec3 Color;\n"
        "\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "\n"
        "void main() {\n"
        "    FragPos = vec3(model * vec4(aPos, 1.0));\n"
        "    Normal = mat3(model) * aNormal;\n"
        "    Color = aColor;\n"
        "    \n"
        "    gl_Position = projection * view * vec4(FragPos, 1.0);\n"
        "}\n";
    
    const char* fragSrc = 
        "#version 120\n"
        "varying vec3 FragPos;\n"
        "varying vec3 Normal;\n"
        "varying vec3 Color;\n"
        "\n"
        "void main() {\n"
        "    gl_FragColor = vec4(Color, 1.0);\n"
        "}\n";
    
    // Compile shaders
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertSrc, nullptr);
    glCompileShader(vertShader);
    
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragSrc, nullptr);
    glCompileShader(fragShader);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    
    // Check for errors
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cerr << "Shader link error: " << log << std::endl;
    }
    
    // Create a simple cube with Vertex structure
    std::vector<Vertex> vertices;
    
    // Front face - bright red
    vertices.push_back({{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {0, 0}, {1, 0, 0, 1}});
    vertices.push_back({{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1, 0}, {1, 0, 0, 1}});
    vertices.push_back({{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1, 1}, {1, 0, 0, 1}});
    vertices.push_back({{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {0, 1}, {1, 0, 0, 1}});
    
    std::vector<uint32_t> indices = {
        0, 1, 2,  2, 3, 0  // Front face
    };
    
    // Create buffers
    GLuint vbo, ebo;
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * indices.size(), indices.data(), GL_STATIC_DRAW);
    
    // Get locations
    GLint posLoc = glGetAttribLocation(program, "aPos");
    GLint normalLoc = glGetAttribLocation(program, "aNormal");
    GLint colorLoc = glGetAttribLocation(program, "aColor");
    
    std::cout << "Attribute locations: pos=" << posLoc << " normal=" << normalLoc << " color=" << colorLoc << std::endl;
    
    // Identity matrices
    float identity[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    
    // Simple orthographic projection
    float ortho[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, -1, 0,
        0, 0, 0, 1
    };
    
    // Render loop
    for (int frame = 0; frame < 3; frame++) {
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(program);
        
        // Set uniforms
        glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, identity);
        glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, identity);
        glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, ortho);
        
        // Bind buffers
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        
        // Setup attributes
        if (posLoc >= 0) {
            glEnableVertexAttribArray(posLoc);
            glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        }
        if (normalLoc >= 0) {
            glEnableVertexAttribArray(normalLoc);
            glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)12);
        }
        if (colorLoc >= 0) {
            glEnableVertexAttribArray(colorLoc);
            glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)32);
        }
        
        // Draw
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "GL Error: " << err << std::endl;
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Take screenshot
    std::vector<unsigned char> pixels(1280 * 720 * 3);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, 1280, 720, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    
    // Count red pixels
    int red_count = 0;
    for (int i = 0; i < 1280 * 720; i++) {
        if (pixels[i * 3] > 200 && pixels[i * 3 + 1] < 50 && pixels[i * 3 + 2] < 50) {
            red_count++;
        }
    }
    
    std::cout << "Red pixels: " << red_count << " / " << (1280 * 720) << std::endl;
    
    if (red_count > 0) {
        std::cout << "✓ Voxel shader rendering WORKS!" << std::endl;
    } else {
        std::cout << "✗ Voxel shader rendering FAILED!" << std::endl;
        
        // Try immediate mode as sanity check
        glUseProgram(0);
        glColor3f(0, 1, 0);
        glBegin(GL_TRIANGLES);
        glVertex2f(-0.5f, -0.5f);
        glVertex2f( 0.5f, -0.5f);
        glVertex2f( 0.0f,  0.5f);
        glEnd();
        
        glReadPixels(0, 0, 1280, 720, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        int green_count = 0;
        for (int i = 0; i < 1280 * 720; i++) {
            if (pixels[i * 3 + 1] > 200) green_count++;
        }
        std::cout << "Immediate mode green pixels: " << green_count << std::endl;
    }
    
    // Keep window open
    sleep(2);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}