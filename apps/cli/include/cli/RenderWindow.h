#pragma once

#include <string>
#include <type_traits>
#include <functional>
#include <glm/glm.hpp>

// Forward declarations
struct GLFWwindow;
namespace VoxelEditor {
namespace Rendering {
    class RenderEngine;
    class RenderContext;
}
}

namespace VoxelEditor {

class Application;

// Mouse button definitions
enum class MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2
};

// Mouse event data
struct MouseEvent {
    float x;
    float y;
    float deltaX;
    float deltaY;
    MouseButton button;
    bool pressed;
    bool shift;
    bool ctrl;
    bool alt;
};

// Key event data
struct KeyEvent {
    int key;
    int scancode;
    bool pressed;
    bool repeat;
    bool shift;
    bool ctrl;
    bool alt;
};

class RenderWindow {
public:
    RenderWindow(Application* app);
    ~RenderWindow();
    
    // Window management
    bool create(int width, int height, const std::string& title);
    void destroy();
    bool isOpen() const;
    void pollEvents();
    void swapBuffers();
    
    // Window properties
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getAspectRatio() const { return static_cast<float>(m_width) / m_height; }
    
    // Coordinate conversion
    glm::vec2 getMousePosition() const;
    glm::vec2 getNormalizedMousePosition() const;
    glm::vec3 getMouseRay(const glm::mat4& viewMatrix, const glm::mat4& projMatrix) const;
    
    // OpenGL context
    void makeContextCurrent();
    
    // Get the underlying GLFW window handle
    GLFWwindow* getWindow() const { return m_window; }
    
    // Screenshot functionality
    bool saveScreenshot(const std::string& filename);
    
    // Render engine integration
    void setRenderEngine(Rendering::RenderEngine* engine) { m_renderEngine = engine; }
    Rendering::RenderEngine* getRenderEngine() const { return m_renderEngine; }
    
    // Event callbacks
    using MouseCallback = std::function<void(const MouseEvent&)>;
    using KeyCallback = std::function<void(const KeyEvent&)>;
    using ResizeCallback = std::function<void(int width, int height)>;
    
    void setMouseCallback(MouseCallback callback) { m_mouseCallback = callback; }
    void setKeyCallback(KeyCallback callback) { m_keyCallback = callback; }
    void setResizeCallback(ResizeCallback callback) { m_resizeCallback = callback; }
    
private:
    Application* m_app;
    GLFWwindow* m_window = nullptr;
    Rendering::RenderEngine* m_renderEngine = nullptr;
    int m_width = 1280;
    int m_height = 720;
    
    // Current mouse state
    double m_mouseX = 0.0;
    double m_mouseY = 0.0;
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
    
    // Event callbacks
    MouseCallback m_mouseCallback;
    KeyCallback m_keyCallback;
    ResizeCallback m_resizeCallback;
    
    // GLFW callbacks (static)
    static void onMouseButton(GLFWwindow* window, int button, int action, int mods);
    static void onMouseMove(GLFWwindow* window, double x, double y);
    static void onMouseScroll(GLFWwindow* window, double xoffset, double yoffset);
    static void onKey(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void onResize(GLFWwindow* window, int width, int height);
    static void onError(int error, const char* description);
    static void onWindowFocus(GLFWwindow* window, int focused);
    static void onCursorEnter(GLFWwindow* window, int entered);
    
    // Initialize GLFW (called once)
    static bool initializeGLFW();
    static bool s_glfwInitialized;
};

} // namespace VoxelEditor