#include <gtest/gtest.h>
#include "../ShaderManager.h"
#include "../../../foundation/logging/Logger.h"
#include <memory>

namespace VoxelEditor {
namespace Rendering {

// Test specifically for Logger usage in ShaderManager
TEST(ShaderManagerLoggingTest, BasicLogging) {
    // Test that we can use the logger directly
    auto& logger = Logging::Logger::getInstance();
    EXPECT_NO_THROW(logger.info("Direct logging test"));
    
    // Test string operations
    std::string testStr = "test";
    EXPECT_NO_THROW(logger.info("Testing with string: " + testStr));
    
    // Test with std::to_string
    int count = 42;
    EXPECT_NO_THROW(logger.info("Count: " + std::to_string(count)));
    
    // Test the specific operation that was crashing
    std::string source = "line1\nline2\nline3\n";
    auto lineCount = std::count(source.begin(), source.end(), '\n');
    EXPECT_EQ(lineCount, 3);
    EXPECT_NO_THROW(logger.info("Line count: " + std::to_string(static_cast<int>(lineCount))));
}

TEST(ShaderManagerLoggingTest, ShaderManagerConstruction) {
    // Test that ShaderManager can be constructed
    EXPECT_NO_THROW(ShaderManager manager);
}

TEST(ShaderManagerLoggingTest, ShaderManagerWithMockRenderer) {
    // Simple mock renderer for testing
    class TestRenderer : public OpenGLRenderer {
    public:
        ShaderId createShader(ShaderType type, const std::string& source) override {
            (void)type;
            (void)source;
            return 1; // Return a valid ID
        }
        
        ShaderId createProgram(const std::vector<ShaderId>& shaders) override {
            (void)shaders;
            return 100; // Return a valid program ID
        }
        
        void deleteShader(ShaderId id) override {
            (void)id;
        }
        
        void deleteProgram(ShaderId id) override {
            (void)id;
        }
        
        // Override context methods to avoid OpenGL calls in test environment
        bool initializeContext(const RenderConfig& config) override {
            (void)config;
            return false; // Don't actually initialize in tests
        }
        
        void destroyContext() override {
            // No-op for test
        }
    };
    
    ShaderManager manager;
    TestRenderer renderer;
    
    // Test with very simple shader source
    const std::string vertexSource = "vertex";
    const std::string fragmentSource = "fragment";
    
    // This should not crash
    ShaderId shader = manager.createShaderFromSource("test", vertexSource, fragmentSource, &renderer);
    EXPECT_NE(shader, InvalidId);
}

} // namespace Rendering
} // namespace VoxelEditor