#include "window.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

bool Window::s_glfwInitialized = false;

Window::Window() {
}

Window::~Window() {
    destroy();
}

void Window::errorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

bool Window::create(int width, int height, const std::string& title) {
    if (!s_glfwInitialized) {
        glfwSetErrorCallback(errorCallback);
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        s_glfwInitialized = true;
    }
    
    // OpenGL 2.1 context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        return false;
    }
    
    m_width = width;
    m_height = height;
    
    makeContextCurrent();
    
    // Load OpenGL functions
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    
    // Print OpenGL info
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    
    // Disable depth test and culling initially (like working triangle test)
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    
    return true;
}

void Window::destroy() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
}

void Window::makeContextCurrent() {
    if (m_window) {
        glfwMakeContextCurrent(m_window);
    }
}

void Window::swapBuffers() {
    if (m_window) {
        glfwSwapBuffers(m_window);
    }
}

void Window::pollEvents() {
    glfwPollEvents();
}

bool Window::shouldClose() {
    return m_window ? glfwWindowShouldClose(m_window) : true;
}

bool Window::saveScreenshot(const std::string& filename) {
    if (!m_window) return false;
    
    makeContextCurrent();
    
    // Get framebuffer size (may be different from window size on high DPI)
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
    
    std::cout << "Screenshot: window=" << m_width << "x" << m_height 
              << " framebuffer=" << fbWidth << "x" << fbHeight << std::endl;
    
    // Read pixels
    std::vector<unsigned char> pixels(fbWidth * fbHeight * 3);
    glReadBuffer(GL_BACK);
    glFinish();
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, fbWidth, fbHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    
    // Flip vertically (OpenGL renders upside down)
    std::vector<unsigned char> flipped(fbWidth * fbHeight * 3);
    for (int y = 0; y < fbHeight; ++y) {
        memcpy(&flipped[y * fbWidth * 3], 
               &pixels[(fbHeight - 1 - y) * fbWidth * 3], 
               fbWidth * 3);
    }
    
    // Save as PPM
    std::string ppmFilename = filename;
    if (ppmFilename.find(".ppm") == std::string::npos) {
        ppmFilename += ".ppm";
    }
    
    std::ofstream file(ppmFilename, std::ios::binary);
    if (!file) return false;
    
    file << "P6\n" << fbWidth << " " << fbHeight << "\n255\n";
    file.write(reinterpret_cast<const char*>(flipped.data()), flipped.size());
    
    std::cout << "Screenshot saved: " << ppmFilename << std::endl;
    return true;
}