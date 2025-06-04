#include <gtest/gtest.h>
#include "../ShaderManager.h"
#include "../../../foundation/logging/Logger.h"

namespace VoxelEditor {
namespace Rendering {

// Test the issue in isolation
TEST(ShaderManagerSimpleTest, TestLoggingIssue) {
    // First test that logging works
    EXPECT_NO_THROW({
        auto& logger = Logging::Logger::getInstance();
        logger.info("Test message from simple test");
    });
    
    // Test the exact code that was crashing
    std::string name = "test_shader";
    std::string vertexSource = "#version 330 core\nvoid main() {}\n";
    std::string fragmentSource = "#version 330 core\nvoid main() {}\n";
    
    EXPECT_NO_THROW({
        auto& logger = Logging::Logger::getInstance();
        logger.info("Compiling shader program: " + name);
        
        auto vertexLines = std::count(vertexSource.begin(), vertexSource.end(), '\n');
        auto fragmentLines = std::count(fragmentSource.begin(), fragmentSource.end(), '\n');
        
        logger.debug("Vertex shader source lines: " + std::to_string(static_cast<int>(vertexLines)));
        logger.debug("Fragment shader source lines: " + std::to_string(static_cast<int>(fragmentLines)));
    });
}

// Test with a mock renderer that doesn't inherit from OpenGLRenderer
class SimpleMockRenderer {
public:
    ShaderId createShader(ShaderType type, const std::string& source) {
        (void)type;
        (void)source;
        return 1;
    }
    
    ShaderId createProgram(const std::vector<ShaderId>& shaders) {
        (void)shaders;
        return 100;
    }
    
    void deleteShader(ShaderId id) {
        (void)id;
    }
};

TEST(ShaderManagerSimpleTest, CreateShaderWithSimpleMock) {
    ShaderManager manager;
    SimpleMockRenderer renderer;
    
    // Can't use this directly since createShaderFromSource expects OpenGLRenderer*
    // This test just verifies the ShaderManager can be created
    EXPECT_NO_THROW({
        auto& logger = Logging::Logger::getInstance();
        logger.info("ShaderManager created successfully");
    });
}

// Test with safer mock renderer approach
TEST(ShaderManagerSimpleTest, CreateShaderWithSafeMockRenderer) {
    // Create a test-safe mock that doesn't trigger OpenGL calls
    class TestSafeMockRenderer : public OpenGLRenderer {
    public:
        TestSafeMockRenderer() : OpenGLRenderer() {
            // Don't call any initialization - keep context invalid by default
        }
        
        // Override OpenGL functions to avoid crashes
        ShaderId createShader(ShaderType type, const std::string& source) override {
            (void)type;
            (void)source;
            static ShaderId nextId = 1;
            return nextId++;
        }
        
        ShaderId createProgram(const std::vector<ShaderId>& shaders) override {
            (void)shaders;
            static ShaderId nextId = 100;
            return nextId++;
        }
        
        void deleteShader(ShaderId shaderId) override {
            (void)shaderId;
            // No-op for test
        }
        
        void deleteProgram(ShaderId programId) override {
            (void)programId;
            // No-op for test
        }
        
        // Override other potentially problematic methods
        bool initializeContext(const RenderConfig& config) override {
            (void)config;
            return false; // Don't actually initialize in tests
        }
        
        void destroyContext() override {
            // No-op for test
        }
        
    private:
        // Make m_contextValid accessible
        friend class TestSafeMockRenderer;
    };
    
    // Create instances
    auto shaderManager = std::make_unique<ShaderManager>();
    auto renderer = std::make_unique<TestSafeMockRenderer>();
    
    const std::string vertexSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        void main() {
            gl_Position = vec4(position, 1.0);
        }
    )";
    
    const std::string fragmentSource = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    )";
    
    // This should work without crashing
    ShaderId shader = shaderManager->createShaderFromSource(
        "test_shader", 
        vertexSource, 
        fragmentSource,
        renderer.get()
    );
    
    EXPECT_NE(shader, InvalidId);
}

} // namespace Rendering
} // namespace VoxelEditor