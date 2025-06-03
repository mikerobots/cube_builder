#pragma once

extern "C" {
    #include <glad/glad.h>
}
#include <GLFW/glfw3.h>
#include <string>

class Window {
public:
    Window();
    ~Window();
    
    bool create(int width, int height, const std::string& title);
    void destroy();
    
    void makeContextCurrent();
    void swapBuffers();
    void pollEvents();
    bool shouldClose();
    
    bool saveScreenshot(const std::string& filename);
    
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    
private:
    GLFWwindow* m_window = nullptr;
    int m_width = 0;
    int m_height = 0;
    
    static bool s_glfwInitialized;
    static void errorCallback(int error, const char* description);
};