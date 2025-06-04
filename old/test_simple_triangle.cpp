#include <iostream>
#include <fstream>
#include <GLFW/glfw3.h>

void saveScreenshot(const char* filename, int width, int height) {
    unsigned char* pixels = new unsigned char[width * height * 3];
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    
    // Flip vertically
    for (int y = 0; y < height/2; y++) {
        for (int x = 0; x < width * 3; x++) {
            std::swap(pixels[y * width * 3 + x], 
                      pixels[(height - 1 - y) * width * 3 + x]);
        }
    }
    
    // Write PPM
    std::ofstream file(filename, std::ios::binary);
    file << "P6\n" << width << " " << height << "\n255\n";
    file.write(reinterpret_cast<char*>(pixels), width * height * 3);
    delete[] pixels;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(640, 480, "Triangle Test", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    // Clear to blue
    glClearColor(0.0f, 0.0f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Draw a red triangle
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-0.5f, -0.5f);
        glVertex2f( 0.5f, -0.5f);
        glVertex2f( 0.0f,  0.5f);
    glEnd();
    
    glfwSwapBuffers(window);
    
    // Save screenshot
    saveScreenshot("simple_triangle.ppm", 640, 480);
    std::cout << "Screenshot saved to simple_triangle.ppm" << std::endl;
    
    // Keep window open briefly
    for (int i = 0; i < 60; i++) {
        glfwPollEvents();
        glfwSwapBuffers(window);
        if (glfwWindowShouldClose(window)) break;
    }
    
    glfwTerminate();
    return 0;
}