// Simple OpenGL test to isolate rendering issues
#include <iostream>
#include <vector>
#include <cstring>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

void checkGLError(const char* label) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "GL Error at " << label << ": " << err << std::endl;
    }
}

void testCase1_Clear(GLFWwindow* window) {
    std::cout << "\n=== Test 1: Clear Color ===" << std::endl;
    
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);  // Red
    glClear(GL_COLOR_BUFFER_BIT);
    checkGLError("glClear");
    
    glfwSwapBuffers(window);
    
    // Read back pixel
    unsigned char pixel[3];
    glReadPixels(640, 360, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    checkGLError("glReadPixels");
    
    std::cout << "Center pixel: R=" << (int)pixel[0] << " G=" << (int)pixel[1] << " B=" << (int)pixel[2] << std::endl;
    if (pixel[0] > 250) {
        std::cout << "✓ Clear color works!" << std::endl;
    } else {
        std::cout << "✗ Clear color FAILED!" << std::endl;
    }
}

void testCase2_ImmediateMode(GLFWwindow* window) {
    std::cout << "\n=== Test 2: Immediate Mode ===" << std::endl;
    
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Draw green triangle
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f( 0.5f, -0.5f);
    glVertex2f( 0.0f,  0.5f);
    glEnd();
    checkGLError("immediate mode");
    
    glfwSwapBuffers(window);
    
    // Check for green
    unsigned char pixel[3];
    glReadPixels(640, 400, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    
    std::cout << "Triangle pixel: R=" << (int)pixel[0] << " G=" << (int)pixel[1] << " B=" << (int)pixel[2] << std::endl;
    if (pixel[1] > 250) {
        std::cout << "✓ Immediate mode works!" << std::endl;
    } else {
        std::cout << "✗ Immediate mode FAILED!" << std::endl;
    }
}

void testCase3_SimpleVBO(GLFWwindow* window) {
    std::cout << "\n=== Test 3: Simple VBO ===" << std::endl;
    
    // Simple triangle data
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // pos + color
         0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f
    };
    
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    checkGLError("VBO creation");
    
    // Simple shader
    const char* vertSrc = 
        "#version 120\n"
        "attribute vec3 aPos;\n"
        "attribute vec3 aColor;\n"
        "varying vec3 vColor;\n"
        "void main() {\n"
        "    gl_Position = vec4(aPos, 1.0);\n"
        "    vColor = aColor;\n"
        "}\n";
    
    const char* fragSrc = 
        "#version 120\n"
        "varying vec3 vColor;\n"
        "void main() {\n"
        "    gl_FragColor = vec4(vColor, 1.0);\n"
        "}\n";
    
    // Compile shaders
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertSrc, nullptr);
    glCompileShader(vertShader);
    
    GLint success;
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(vertShader, 512, nullptr, log);
        std::cerr << "Vertex shader error: " << log << std::endl;
    }
    
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragSrc, nullptr);
    glCompileShader(fragShader);
    
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(fragShader, 512, nullptr, log);
        std::cerr << "Fragment shader error: " << log << std::endl;
    }
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cerr << "Shader link error: " << log << std::endl;
    }
    
    // Render
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(program);
    
    GLint posLoc = glGetAttribLocation(program, "aPos");
    GLint colorLoc = glGetAttribLocation(program, "aColor");
    
    glEnableVertexAttribArray(posLoc);
    glEnableVertexAttribArray(colorLoc);
    
    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    
    glDrawArrays(GL_TRIANGLES, 0, 3);
    checkGLError("draw arrays");
    
    glfwSwapBuffers(window);
    
    // Check center
    unsigned char pixel[3];
    glReadPixels(640, 400, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    
    std::cout << "VBO pixel: R=" << (int)pixel[0] << " G=" << (int)pixel[1] << " B=" << (int)pixel[2] << std::endl;
    if (pixel[0] > 100 || pixel[1] > 100 || pixel[2] > 100) {
        std::cout << "✓ VBO rendering works!" << std::endl;
    } else {
        std::cout << "✗ VBO rendering FAILED!" << std::endl;
    }
    
    // Cleanup
    glDeleteProgram(program);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    glDeleteBuffers(1, &vbo);
}

void testCase4_FramebufferState(GLFWwindow* window) {
    std::cout << "\n=== Test 4: Framebuffer State ===" << std::endl;
    
    GLint drawFBO, readFBO, viewport[4];
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFBO);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFBO);
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    std::cout << "Draw FBO: " << drawFBO << " (should be 0)" << std::endl;
    std::cout << "Read FBO: " << readFBO << " (should be 0)" << std::endl;
    std::cout << "Viewport: " << viewport[0] << "," << viewport[1] << " " 
              << viewport[2] << "x" << viewport[3] << std::endl;
    
    // Check window framebuffer size
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    std::cout << "Framebuffer size: " << width << "x" << height << std::endl;
    
    int winWidth, winHeight;
    glfwGetWindowSize(window, &winWidth, &winHeight);
    std::cout << "Window size: " << winWidth << "x" << winHeight << std::endl;
    
    if (width != winWidth || height != winHeight) {
        std::cout << "! High DPI scaling detected" << std::endl;
    }
}

void testCase5_MatrixTest(GLFWwindow* window) {
    std::cout << "\n=== Test 5: Matrix Upload ===" << std::endl;
    
    // Test matrix
    float testMatrix[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0.5, 0.5, 0, 1  // Translation in last row (row-major)
    };
    
    // Simple shader with matrix
    const char* vertSrc = 
        "#version 120\n"
        "attribute vec2 aPos;\n"
        "uniform mat4 uMatrix;\n"
        "void main() {\n"
        "    gl_Position = uMatrix * vec4(aPos, 0.0, 1.0);\n"
        "}\n";
    
    const char* fragSrc = 
        "#version 120\n"
        "void main() {\n"
        "    gl_FragColor = vec4(1.0, 1.0, 0.0, 1.0);\n"  // Yellow
        "}\n";
    
    // Create shader
    GLuint program = glCreateProgram();
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    
    glShaderSource(vertShader, 1, &vertSrc, nullptr);
    glShaderSource(fragShader, 1, &fragSrc, nullptr);
    glCompileShader(vertShader);
    glCompileShader(fragShader);
    
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    
    // Square at origin
    float vertices[] = {
        -0.1f, -0.1f,
         0.1f, -0.1f,
         0.1f,  0.1f,
        -0.1f,  0.1f
    };
    
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Render
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(program);
    
    GLint matrixLoc = glGetUniformLocation(program, "uMatrix");
    glUniformMatrix4fv(matrixLoc, 1, GL_FALSE, testMatrix);  // GL_FALSE = row major
    checkGLError("matrix upload");
    
    GLint posLoc = glGetAttribLocation(program, "aPos");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    glfwSwapBuffers(window);
    
    // Check if translated (should be in upper right)
    unsigned char pixel[3];
    glReadPixels(960, 540, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);  // Upper right quadrant
    
    std::cout << "Translated pixel: R=" << (int)pixel[0] << " G=" << (int)pixel[1] << " B=" << (int)pixel[2] << std::endl;
    if (pixel[0] > 250 && pixel[1] > 250) {
        std::cout << "✓ Matrix upload works!" << std::endl;
    } else {
        std::cout << "✗ Matrix upload FAILED!" << std::endl;
        std::cout << "  Try with GL_TRUE (column major)..." << std::endl;
        
        // Try column major
        glClear(GL_COLOR_BUFFER_BIT);
        glUniformMatrix4fv(matrixLoc, 1, GL_TRUE, testMatrix);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glfwSwapBuffers(window);
        
        glReadPixels(960, 540, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
        if (pixel[0] > 250 && pixel[1] > 250) {
            std::cout << "  ✓ Works with GL_TRUE!" << std::endl;
        }
    }
    
    // Cleanup
    glDeleteProgram(program);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    glDeleteBuffers(1, &vbo);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Same window hints as main app
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    
    GLFWwindow* window = glfwCreateWindow(1280, 720, "OpenGL Test", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    
    // Run tests
    testCase1_Clear(window);
    glfwPollEvents();
    
    testCase2_ImmediateMode(window);
    glfwPollEvents();
    
    testCase3_SimpleVBO(window);
    glfwPollEvents();
    
    testCase4_FramebufferState(window);
    
    testCase5_MatrixTest(window);
    glfwPollEvents();
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    std::cout << "Press any key to exit..." << std::endl;
    
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}