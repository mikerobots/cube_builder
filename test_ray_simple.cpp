#include <gtest/gtest.h>
#include <GLFW/glfw3.h>
#include "cli/Application.h"
#include "cli/RenderWindow.h"

class SimpleRayTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize GLFW
        ASSERT_TRUE(glfwInit());
        
        // Create window
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        
        m_renderWindow = std::make_unique<VoxelEditor::RenderWindow>();
        m_renderWindow->initialize(1280, 720, "Test", false);
        
        // Create application
        m_app = std::make_unique<VoxelEditor::Application>();
        m_app->setRenderWindow(m_renderWindow.get());
        m_app->initializeRenderer();
        m_app->initializeSceneFromCommandLine("");
        
        // Wait for initialization
        for (int i = 0; i < 5; ++i) {
            m_app->update();
            m_app->render();
            glfwPollEvents();
        }
    }
    
    void TearDown() override {
        m_app.reset();
        m_renderWindow.reset();
        glfwTerminate();
    }
    
    std::unique_ptr<VoxelEditor::Application> m_app;
    std::unique_ptr<VoxelEditor::RenderWindow> m_renderWindow;
};

TEST_F(SimpleRayTest, RayAtEdgeWithoutVoxel) {
    auto mouseInteraction = m_app->getMouseInteraction();
    ASSERT_NE(mouseInteraction, nullptr);
    
    // Enable ray visualization
    mouseInteraction->setRayVisualizationEnabled(true);
    
    // Move mouse to edge position WITHOUT placing any voxels
    mouseInteraction->onMouseMove(100, 100);
    
    // Update and render
    m_app->update();
    m_app->render();
    
    // Just check that it doesn't crash
    SUCCEED();
}

TEST_F(SimpleRayTest, RayAtEdgeWithVoxel) {
    auto mouseInteraction = m_app->getMouseInteraction();
    auto voxelManager = m_app->getVoxelManager();
    ASSERT_NE(mouseInteraction, nullptr);
    ASSERT_NE(voxelManager, nullptr);
    
    // Enable ray visualization
    mouseInteraction->setRayVisualizationEnabled(true);
    
    // Place a voxel like the failing test does
    voxelManager->setVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_1cm, true);
    m_app->requestMeshUpdate();
    
    // Move mouse to edge position 
    mouseInteraction->onMouseMove(100, 100);
    
    // Update and render
    m_app->update();
    m_app->render();
    
    // Just check that it doesn't crash
    SUCCEED();
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}