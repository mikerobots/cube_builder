// Test with proper buffer handling
#include <iostream>
#include <thread>
#include <chrono>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

int main() {
    if (!glfwInit()) {
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);  // Explicitly request double buffering
    
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Buffer Test", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    
    // Set viewport to window size (not framebuffer size)
    glViewport(0, 0, 1280, 720);
    
    std::cout << "=== Buffer Test ===" << std::endl;
    
    // Test 1: Clear and immediate read (before swap)
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    unsigned char pixel[3];
    glReadPixels(640, 360, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    std::cout << "Before swap: R=" << (int)pixel[0] << " G=" << (int)pixel[1] << " B=" << (int)pixel[2] << std::endl;
    
    // Actually swap buffers
    glfwSwapBuffers(window);
    
    // Test 2: Read after swap
    glReadPixels(640, 360, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    std::cout << "After swap: R=" << (int)pixel[0] << " G=" << (int)pixel[1] << " B=" << (int)pixel[2] << std::endl;
    
    // Test 3: Clear green, read from back buffer
    glReadBuffer(GL_BACK);
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glReadPixels(640, 360, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    std::cout << "Back buffer after green clear: R=" << (int)pixel[0] << " G=" << (int)pixel[1] << " B=" << (int)pixel[2] << std::endl;
    
    // Test 4: Try front buffer
    glReadBuffer(GL_FRONT);
    glReadPixels(640, 360, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    std::cout << "Front buffer: R=" << (int)pixel[0] << " G=" << (int)pixel[1] << " B=" << (int)pixel[2] << std::endl;
    
    // Test 5: Draw something visible
    glReadBuffer(GL_BACK);
    glDrawBuffer(GL_BACK);
    
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Draw immediate mode quad
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glColor3f(1.0f, 1.0f, 0.0f);  // Yellow
    glBegin(GL_QUADS);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f( 0.5f, -0.5f);
    glVertex2f( 0.5f,  0.5f);
    glVertex2f(-0.5f,  0.5f);
    glEnd();
    
    glFlush();  // Force commands to execute
    
    glReadPixels(640, 360, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    std::cout << "After drawing yellow quad: R=" << (int)pixel[0] << " G=" << (int)pixel[1] << " B=" << (int)pixel[2] << std::endl;
    
    // Visual test - keep window open
    std::cout << "\nYou should see a yellow square on gray background" << std::endl;
    glfwSwapBuffers(window);
    
    // Keep window open for 3 seconds
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(3)) {
        glfwPollEvents();
    }
    
    // Take screenshot after displaying
    glReadBuffer(GL_FRONT);  // Read from front after swap
    std::vector<unsigned char> screenshot(1280 * 720 * 3);
    glReadPixels(0, 0, 1280, 720, GL_RGB, GL_UNSIGNED_BYTE, screenshot.data());
    
    // Count yellow pixels
    int yellow_count = 0;
    for (int i = 0; i < 1280 * 720; i++) {
        int r = screenshot[i * 3];
        int g = screenshot[i * 3 + 1];
        int b = screenshot[i * 3 + 2];
        if (r > 250 && g > 250 && b < 50) {
            yellow_count++;
        }
    }
    
    std::cout << "Yellow pixels in screenshot: " << yellow_count << " / " << (1280 * 720) << std::endl;
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}