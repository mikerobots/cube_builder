#pragma once

#include <gtest/gtest.h>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#include "../MacOSGLLoader.h"
#else
extern "C" {
#include <glad/glad.h>
}
#endif

#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>

namespace VoxelEditor {
namespace Rendering {
namespace Tests {

/**
 * Base test fixture for OpenGL-based rendering tests.
 * Provides proper window creation, context initialization, and cleanup.
 */
class OpenGLTestFixture : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    int windowWidth = 800;
    int windowHeight = 600;
    bool contextValid = false;
    
    // Static initialization flag
    static bool glfwInitialized;
    
    void SetUp() override {
        // Initialize GLFW once
        if (!glfwInitialized) {
            if (!glfwInit()) {
                std::cerr << "Failed to initialize GLFW\n";
                GTEST_SKIP() << "GLFW initialization failed";
            }
            glfwInitialized = true;
            
            // Set error callback
            glfwSetErrorCallback([](int error, const char* description) {
                std::cerr << "GLFW Error " << error << ": " << description << "\n";
            });
        }
        
        // Create window with offscreen rendering support
        createWindow();
        
        if (!window) {
            GTEST_SKIP() << "Window creation failed - likely running in headless environment";
        }
        
        // Initialize OpenGL context
        initializeOpenGL();
    }
    
    void TearDown() override {
        if (window) {
            glfwDestroyWindow(window);
            window = nullptr;
        }
        contextValid = false;
    }
    
    static void TearDownTestSuite() {
        if (glfwInitialized) {
            glfwTerminate();
            glfwInitialized = false;
        }
    }
    
    /**
     * Create a GLFW window suitable for testing
     */
    void createWindow() {
        // Configure for offscreen rendering
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window
        
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        
        // Try to create window
        window = glfwCreateWindow(windowWidth, windowHeight, "OpenGL Test", nullptr, nullptr);
        
        if (!window) {
            // Try with a different configuration
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
            
            window = glfwCreateWindow(windowWidth, windowHeight, "OpenGL Test", nullptr, nullptr);
        }
        
        if (window) {
            glfwMakeContextCurrent(window);
            
            // Enable vsync to avoid tearing in tests
            glfwSwapInterval(0);
        }
    }
    
    /**
     * Initialize OpenGL context and load functions
     */
    void initializeOpenGL() {
        if (!window) {
            return;
        }
        
#ifdef __APPLE__
        // Load OpenGL extensions on macOS
        if (!VoxelEditor::Rendering::LoadOpenGLExtensions()) {
            std::cerr << "Failed to load OpenGL extensions on macOS\n";
            // Continue anyway - the Apple-specific functions might work
        }
#else
        // Load OpenGL functions
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "Failed to initialize GLAD\n";
            glfwDestroyWindow(window);
            window = nullptr;
            return;
        }
#endif
        
        // Verify OpenGL version
        const GLubyte* version = glGetString(GL_VERSION);
        if (version) {
            std::cout << "OpenGL Version: " << version << "\n";
        }
        
        // Set default OpenGL state
        glViewport(0, 0, windowWidth, windowHeight);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        
        // Clear any errors
        while (glGetError() != GL_NO_ERROR) {}
        
        contextValid = true;
    }
    
    /**
     * Check if OpenGL context is valid
     */
    bool hasValidContext() const {
        return window != nullptr && contextValid;
    }
    
    /**
     * Capture framebuffer to pixels
     */
    std::vector<uint8_t> captureFramebuffer() {
        if (!hasValidContext()) {
            return {};
        }
        
        std::vector<uint8_t> pixels(windowWidth * windowHeight * 3);
        glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        return pixels;
    }
    
    /**
     * Save framebuffer to PPM file for debugging
     */
    void saveFramebufferToPPM(const std::string& filename) {
        auto pixels = captureFramebuffer();
        if (pixels.empty()) return;
        
        std::ofstream file(filename);
        if (!file) return;
        
        file << "P3\n" << windowWidth << " " << windowHeight << "\n255\n";
        
        // Flip vertically
        for (int y = windowHeight - 1; y >= 0; --y) {
            for (int x = 0; x < windowWidth; ++x) {
                int idx = (y * windowWidth + x) * 3;
                file << (int)pixels[idx] << " " 
                     << (int)pixels[idx + 1] << " " 
                     << (int)pixels[idx + 2] << "\n";
            }
        }
    }
    
    /**
     * Clear framebuffer
     */
    void clearFramebuffer() {
        if (!hasValidContext()) return;
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    
    /**
     * Swap buffers (for double-buffered rendering)
     */
    void swapBuffers() {
        if (!hasValidContext()) return;
        
        glfwSwapBuffers(window);
    }
    
    /**
     * Process events (useful for interactive tests)
     */
    void processEvents() {
        glfwPollEvents();
    }
    
    /**
     * Simple shader compilation helper
     */
    GLuint compileShader(GLenum type, const char* source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Shader compilation failed: " << infoLog << "\n";
            glDeleteShader(shader);
            return 0;
        }
        
        return shader;
    }
    
    /**
     * Simple program creation helper
     */
    GLuint createProgram(const char* vertexSource, const char* fragmentSource) {
        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
        if (!vertexShader) return 0;
        
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
        if (!fragmentShader) {
            glDeleteShader(vertexShader);
            return 0;
        }
        
        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            std::cerr << "Program linking failed: " << infoLog << "\n";
            glDeleteProgram(program);
            program = 0;
        }
        
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        
        return program;
    }
};

// Initialize static member inline (C++17)
inline bool OpenGLTestFixture::glfwInitialized = false;

/**
 * Extended fixture for tests that require a framebuffer object
 */
class OpenGLFramebufferTestFixture : public OpenGLTestFixture {
protected:
    GLuint framebuffer = 0;
    GLuint colorTexture = 0;
    GLuint depthRenderbuffer = 0;
    
    void SetUp() override {
        OpenGLTestFixture::SetUp();
        
        if (!hasValidContext()) {
            return;
        }
        
        createFramebuffer();
    }
    
    void TearDown() override {
        if (framebuffer) {
            glDeleteFramebuffers(1, &framebuffer);
        }
        if (colorTexture) {
            glDeleteTextures(1, &colorTexture);
        }
        if (depthRenderbuffer) {
            glDeleteRenderbuffers(1, &depthRenderbuffer);
        }
        
        OpenGLTestFixture::TearDown();
    }
    
    void createFramebuffer() {
        // Create framebuffer
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        
        // Create color texture
        glGenTextures(1, &colorTexture);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windowWidth, windowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
        
        // Create depth renderbuffer
        glGenRenderbuffers(1, &depthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, windowWidth, windowHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
        
        // Check completeness
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Framebuffer not complete\n";
            glDeleteFramebuffers(1, &framebuffer);
            glDeleteTextures(1, &colorTexture);
            glDeleteRenderbuffers(1, &depthRenderbuffer);
            framebuffer = 0;
            colorTexture = 0;
            depthRenderbuffer = 0;
        }
        
        // Bind back to default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void bindFramebuffer() {
        if (framebuffer) {
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        }
    }
    
    void unbindFramebuffer() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};

} // namespace Tests
} // namespace Rendering
} // namespace VoxelEditor