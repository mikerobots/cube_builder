// Standard library includes first
#include <iostream>
#include <vector>
#include <stack>
#include <mutex>
#include <memory>
#include <algorithm>

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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
    
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
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Enable backface culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
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

} // namespace VoxelEditor