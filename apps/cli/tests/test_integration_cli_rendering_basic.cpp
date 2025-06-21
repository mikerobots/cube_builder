#include <gtest/gtest.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <chrono>

#include "cli/Application.h"
#include "cli/RenderWindow.h"
#include "foundation/logging/Logger.h"

namespace VoxelEditor {
namespace Tests {

// CLI Basic Rendering Integration Tests
class CLIBasicRenderingTest : public ::testing::Test {
protected:
    std::unique_ptr<Application> app;
    
    void SetUp() override {
        // Initialize logging
        Logging::Logger::getInstance().setLevel(Logging::Logger::Level::Warning);
        
        // Create application with OpenGL context
        app = std::make_unique<Application>();
        
        int argc = 1;
        char arg0[] = "test";
        char* argv[] = {arg0, nullptr};
        
        ASSERT_TRUE(app->initialize(argc, argv)) << "Application should initialize";
    }
    
    void TearDown() override {
        app.reset();
    }
};

TEST_F(CLIBasicRenderingTest, WindowCreationTest) {
    // Test window creation
    auto* window = app->getRenderWindow();
    ASSERT_NE(window, nullptr) << "Render window should be created";
    
    // Check window properties
    EXPECT_TRUE(window->isOpen()) << "Window should be open";
    EXPECT_GT(window->getWidth(), 0) << "Window width should be positive";
    EXPECT_GT(window->getHeight(), 0) << "Window height should be positive";
    EXPECT_GT(window->getAspectRatio(), 0) << "Aspect ratio should be positive";
}

TEST_F(CLIBasicRenderingTest, BasicRenderLoopTest) {
    // Test basic render loop functionality
    auto* window = app->getRenderWindow();
    ASSERT_NE(window, nullptr);
    
    // Run a few frames of the render loop
    for (int i = 0; i < 5; ++i) {
        // Poll events
        window->pollEvents();
        
        // Render
        app->render();
        
        // Swap buffers
        window->swapBuffers();
        
        // Check for OpenGL errors
        GLenum error = glGetError();
        EXPECT_EQ(error, GL_NO_ERROR) << "OpenGL error in frame " << i << ": " << error;
        
        // Small delay
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
    
    // Window should still be open
    EXPECT_TRUE(window->isOpen()) << "Window should remain open after render loop";
}

TEST_F(CLIBasicRenderingTest, FramebufferTest) {
    // Test framebuffer functionality
    auto* window = app->getRenderWindow();
    ASSERT_NE(window, nullptr);
    
    // Clear framebuffer with a specific color
    glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Read back pixels
    int width = window->getWidth();
    int height = window->getHeight();
    std::vector<unsigned char> pixels(width * height * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    
    // Check that pixels match the clear color (with some tolerance)
    bool colorMatches = true;
    for (size_t i = 0; i < pixels.size(); i += 4) {
        if (abs(pixels[i] - 51) > 5 ||      // R: 0.2 * 255 = 51
            abs(pixels[i+1] - 76) > 5 ||    // G: 0.3 * 255 = 76
            abs(pixels[i+2] - 102) > 5) {   // B: 0.4 * 255 = 102
            colorMatches = false;
            break;
        }
    }
    
    EXPECT_TRUE(colorMatches) << "Framebuffer should contain the clear color";
    
    // Test depth buffer
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    // No OpenGL errors
    EXPECT_EQ(glGetError(), GL_NO_ERROR) << "No OpenGL errors in framebuffer operations";
}

} // namespace Tests
} // namespace VoxelEditor