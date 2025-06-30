// Simplified test to capture overlay rendering screenshots
#include <iostream>
#include <fstream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

void saveScreenshot(const std::string& filename, int width, int height) {
    std::vector<unsigned char> pixels(width * height * 3);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    
    std::ofstream file(filename);
    file << "P3\n" << width << " " << height << "\n255\n";
    
    // Flip vertically while writing
    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;
            file << (int)pixels[idx] << " " 
                 << (int)pixels[idx + 1] << " " 
                 << (int)pixels[idx + 2] << " ";
        }
        file << "\n";
    }
}

void drawTestGrid() {
    // Simple immediate mode rendering for testing
    glBegin(GL_LINES);
    
    // Grid lines in gray
    glColor3f(0.5f, 0.5f, 0.5f);
    
    // Horizontal lines
    for (float z = -2.0f; z <= 2.0f; z += 0.32f) {
        glVertex3f(-2.0f, 0.0f, z);
        glVertex3f(2.0f, 0.0f, z);
    }
    
    // Vertical lines
    for (float x = -2.0f; x <= 2.0f; x += 0.32f) {
        glVertex3f(x, 0.0f, -2.0f);
        glVertex3f(x, 0.0f, 2.0f);
    }
    
    glEnd();
}

void drawTestOutline(float x, float z, float size) {
    // Green outline
    glColor3f(0.0f, 1.0f, 0.0f);
    glLineWidth(2.0f);
    
    glBegin(GL_LINE_LOOP);
    glVertex3f(x, 0.0f, z);
    glVertex3f(x + size, 0.0f, z);
    glVertex3f(x + size, 0.0f, z + size);
    glVertex3f(x, 0.0f, z + size);
    glEnd();
    
    // Top face
    glBegin(GL_LINE_LOOP);
    glVertex3f(x, size, z);
    glVertex3f(x + size, size, z);
    glVertex3f(x + size, size, z + size);
    glVertex3f(x, size, z + size);
    glEnd();
    
    // Vertical edges
    glBegin(GL_LINES);
    glVertex3f(x, 0.0f, z);
    glVertex3f(x, size, z);
    glVertex3f(x + size, 0.0f, z);
    glVertex3f(x + size, size, z);
    glVertex3f(x + size, 0.0f, z + size);
    glVertex3f(x + size, size, z + size);
    glVertex3f(x, 0.0f, z + size);
    glVertex3f(x, size, z + size);
    glEnd();
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    
    GLFWwindow* window = glfwCreateWindow(800, 600, "Overlay Test", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }
    
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Test different views and positions
    struct TestCase {
        float camX, camY, camZ;
        float outlineX, outlineZ;
        std::string name;
    };
    
    TestCase tests[] = {
        {0, 5, 0.01f, 0, 0, "top_view_center"},
        {0, 5, 0.01f, 1.0f, 0, "top_view_right"},
        {0, 5, 0.01f, -1.0f, 0, "top_view_left"},
        {0, 5, 0.01f, 0, 1.0f, "top_view_forward"},
        {0, 5, 0.01f, 0, -1.0f, "top_view_back"},
    };
    
    for (const auto& test : tests) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Set up view
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-3.33, 3.33, -2.5, 2.5, 0.1, 100);
        
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(test.camX, test.camY, test.camZ,  // eye
                  0, 0, 0,                           // center
                  0, 0, -1);                         // up (for top view)
        
        // Draw grid
        drawTestGrid();
        
        // Draw outline at position
        drawTestOutline(test.outlineX, test.outlineZ, 0.32f);
        
        // Capture screenshot
        std::string filename = "overlay_test_" + test.name + ".ppm";
        saveScreenshot(filename, 800, 600);
        
        std::cout << "Saved: " << filename 
                  << " (outline at " << test.outlineX << ", " << test.outlineZ << ")\n";
    }
    
    // Test mouse position correspondence
    std::cout << "\nMouse to world mapping (800x600 screen, ortho -3.33 to 3.33 horizontal):\n";
    float screenToWorld = 6.66f / 800.0f;
    std::cout << "Center (400,300) -> World (0,0)\n";
    std::cout << "Right 100px (500,300) -> World (" << (100 * screenToWorld) << ",0)\n";
    std::cout << "Expected: 100 pixels * " << screenToWorld << " = 0.833 world units\n";
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}