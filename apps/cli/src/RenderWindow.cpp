// Standard library includes first
#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <mutex>
#include <memory>
#include <algorithm>
#include <cstring>

// Third-party includes
#include <GLFW/glfw3.h>
extern "C" {
#include <glad/glad.h>
}
#include <glm/gtc/matrix_transform.hpp>

// Application headers
#include "cli/RenderWindow.h"
#include "cli/Application.h"

namespace VoxelEditor {

bool RenderWindow::s_glfwInitialized = false;

RenderWindow::RenderWindow(Application* app)
    : m_app(app) {
}

RenderWindow::~RenderWindow() {
    destroy();
}

bool RenderWindow::create(int width, int height, const std::string& title) {
    if (!initializeGLFW()) {
        return false;
    }
    
    // Set OpenGL version and profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    // Use compatibility profile for debugging
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Don't use forward compat in compatibility mode
    // #ifdef __APPLE__
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // #endif
    
    // Create window
    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        return false;
    }
    
    m_width = width;
    m_height = height;
    
    // Set user pointer for callbacks
    glfwSetWindowUserPointer(m_window, this);
    
    // Set callbacks
    glfwSetMouseButtonCallback(m_window, onMouseButton);
    glfwSetCursorPosCallback(m_window, onMouseMove);
    glfwSetScrollCallback(m_window, onMouseScroll);
    glfwSetKeyCallback(m_window, onKey);
    glfwSetFramebufferSizeCallback(m_window, onResize);
    
    // Make context current
    makeContextCurrent();
    
    // Load OpenGL functions
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        destroy();
        return false;
    }
    
    // Set initial viewport
    glViewport(0, 0, width, height);
    
    // Disable depth testing and culling initially to debug rendering
    // TODO: Re-enable these once rendering is working
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    
    // // Enable depth testing
    // glEnable(GL_DEPTH_TEST);
    // 
    // // Enable backface culling
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);
    
    return true;
}

void RenderWindow::destroy() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
}

bool RenderWindow::isOpen() const {
    return m_window && !glfwWindowShouldClose(m_window);
}

void RenderWindow::pollEvents() {
    glfwPollEvents();
}

void RenderWindow::swapBuffers() {
    if (m_window) {
        glfwSwapBuffers(m_window);
    }
}

glm::vec2 RenderWindow::getMousePosition() const {
    return glm::vec2(m_mouseX, m_mouseY);
}

glm::vec2 RenderWindow::getNormalizedMousePosition() const {
    return glm::vec2(
        (2.0f * m_mouseX) / m_width - 1.0f,
        1.0f - (2.0f * m_mouseY) / m_height
    );
}

glm::vec3 RenderWindow::getMouseRay(const glm::mat4& viewMatrix, const glm::mat4& projMatrix) const {
    // Get normalized device coordinates
    glm::vec2 ndc = getNormalizedMousePosition();
    
    // Create ray in clip space
    glm::vec4 rayClip(ndc.x, ndc.y, -1.0f, 1.0f);
    
    // Transform to eye space
    glm::vec4 rayEye = glm::inverse(projMatrix) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    
    // Transform to world space
    glm::vec3 rayWorld = glm::vec3(glm::inverse(viewMatrix) * rayEye);
    
    return glm::normalize(rayWorld);
}

void RenderWindow::makeContextCurrent() {
    if (m_window) {
        glfwMakeContextCurrent(m_window);
        
        // Verify context is current
        if (glfwGetCurrentContext() != m_window) {
            std::cerr << "ERROR: Failed to make context current!" << std::endl;
        }
    }
}

bool RenderWindow::initializeGLFW() {
    if (s_glfwInitialized) {
        return true;
    }
    
    glfwSetErrorCallback(onError);
    
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    s_glfwInitialized = true;
    return true;
}

void RenderWindow::onMouseButton(GLFWwindow* window, int button, int action, int mods) {
    RenderWindow* self = static_cast<RenderWindow*>(glfwGetWindowUserPointer(window));
    if (!self || !self->m_mouseCallback) return;
    
    MouseEvent event;
    event.x = static_cast<float>(self->m_mouseX);
    event.y = static_cast<float>(self->m_mouseY);
    event.deltaX = 0.0f;
    event.deltaY = 0.0f;
    event.button = static_cast<MouseButton>(button);
    event.pressed = (action == GLFW_PRESS);
    event.shift = (mods & GLFW_MOD_SHIFT) != 0;
    event.ctrl = (mods & GLFW_MOD_CONTROL) != 0;
    event.alt = (mods & GLFW_MOD_ALT) != 0;
    
    self->m_mouseCallback(event);
}

void RenderWindow::onMouseMove(GLFWwindow* window, double x, double y) {
    RenderWindow* self = static_cast<RenderWindow*>(glfwGetWindowUserPointer(window));
    if (!self) return;
    
    self->m_lastMouseX = self->m_mouseX;
    self->m_lastMouseY = self->m_mouseY;
    self->m_mouseX = x;
    self->m_mouseY = y;
    
    if (self->m_mouseCallback) {
        MouseEvent event;
        event.x = static_cast<float>(x);
        event.y = static_cast<float>(y);
        event.deltaX = static_cast<float>(x - self->m_lastMouseX);
        event.deltaY = static_cast<float>(y - self->m_lastMouseY);
        event.button = MouseButton::Left; // Not used for move events
        event.pressed = false;
        
        // Check which buttons are pressed
        int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
        event.pressed = (state == GLFW_PRESS);
        
        // Get modifiers
        event.shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                     glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        event.ctrl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                    glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
        event.alt = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
                   glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
        
        self->m_mouseCallback(event);
    }
}

void RenderWindow::onMouseScroll(GLFWwindow* window, double xoffset, double yoffset) {
    RenderWindow* self = static_cast<RenderWindow*>(glfwGetWindowUserPointer(window));
    if (!self || !self->m_mouseCallback) return;
    
    MouseEvent event;
    event.x = static_cast<float>(self->m_mouseX);
    event.y = static_cast<float>(self->m_mouseY);
    event.deltaX = static_cast<float>(xoffset);
    event.deltaY = static_cast<float>(yoffset);
    event.button = MouseButton::Middle;
    event.pressed = false;
    
    // Get modifiers
    event.shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                 glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    event.ctrl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    event.alt = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
               glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
    
    self->m_mouseCallback(event);
}

void RenderWindow::onKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
    RenderWindow* self = static_cast<RenderWindow*>(glfwGetWindowUserPointer(window));
    if (!self || !self->m_keyCallback) return;
    
    KeyEvent event;
    event.key = key;
    event.scancode = scancode;
    event.pressed = (action == GLFW_PRESS);
    event.repeat = (action == GLFW_REPEAT);
    event.shift = (mods & GLFW_MOD_SHIFT) != 0;
    event.ctrl = (mods & GLFW_MOD_CONTROL) != 0;
    event.alt = (mods & GLFW_MOD_ALT) != 0;
    
    self->m_keyCallback(event);
}

void RenderWindow::onResize(GLFWwindow* window, int width, int height) {
    RenderWindow* self = static_cast<RenderWindow*>(glfwGetWindowUserPointer(window));
    if (!self) return;
    
    self->m_width = width;
    self->m_height = height;
    
    // Update viewport
    glViewport(0, 0, width, height);
    
    if (self->m_resizeCallback) {
        self->m_resizeCallback(width, height);
    }
}

void RenderWindow::onError(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

bool RenderWindow::saveScreenshot(const std::string& filename) {
    if (!m_window) {
        return false;
    }
    
    makeContextCurrent();
    
    // Get framebuffer size (may be different from window size on high DPI displays)
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
    
    // Allocate buffer for pixels (RGB format)
    std::vector<unsigned char> pixels(fbWidth * fbHeight * 3);
    
    // Read from the back buffer (where we just rendered)
    glReadBuffer(GL_BACK);
    
    // Force all pending GL commands to execute
    glFlush();
    
    // Ensure all OpenGL commands are finished
    glFinish();
    
    // Read pixels from framebuffer (back buffer)
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, fbWidth, fbHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    
    // OpenGL gives us the image upside down, so we need to flip it
    std::vector<unsigned char> flipped(fbWidth * fbHeight * 3);
    for (int y = 0; y < fbHeight; ++y) {
        memcpy(&flipped[y * fbWidth * 3], 
               &pixels[(fbHeight - 1 - y) * fbWidth * 3], 
               fbWidth * 3);
    }
    
    // For now, save as PPM format (simple, no external dependencies)
    // TODO: Add PNG support with stb_image_write or similar
    std::string actualFilename = filename;
    
    // If filename ends with .png, replace with .ppm for now
    size_t dotPos = actualFilename.find_last_of('.');
    if (dotPos != std::string::npos && actualFilename.substr(dotPos) == ".png") {
        actualFilename = actualFilename.substr(0, dotPos) + ".ppm";
    } else if (dotPos == std::string::npos) {
        actualFilename += ".ppm";
    }
    
    // Write PPM file
    std::ofstream file(actualFilename, std::ios::binary);
    if (!file) {
        return false;
    }
    
    // PPM header
    file << "P6\n" << fbWidth << " " << fbHeight << "\n255\n";
    
    // Write pixel data
    file.write(reinterpret_cast<const char*>(flipped.data()), flipped.size());
    file.close();
    
    // Log that we saved as PPM instead of PNG
    if (filename.find(".png") != std::string::npos) {
        std::cout << "Note: Saved as " << actualFilename << " (PPM format). "
                  << "Convert to PNG with: convert " << actualFilename << " " << filename << std::endl;
    }
    
    return true;
}

} // namespace VoxelEditor