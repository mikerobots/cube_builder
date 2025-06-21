#include <gtest/gtest.h>
#include "../OpenGLRenderer.h"
#include "../RenderConfig.h"
#include <vector>
#include <memory>

// Mock GL functions for testing without actual OpenGL context
// In real tests, you'd use a headless context or mock library

namespace VoxelEditor {
namespace Rendering {

// Test fixture for OpenGLRenderer
class OpenGLRendererTest : public ::testing::Test {
protected:
    std::unique_ptr<OpenGLRenderer> renderer;
    
    void SetUp() override {
        renderer = std::make_unique<OpenGLRenderer>();
    }
    
    void TearDown() override {
        renderer.reset();
    }
};





// Test vertex attribute setup
TEST_F(OpenGLRendererTest, VertexAttributes) {
    // Test standard vertex attributes
    std::vector<VertexAttribute> attributes = {
        VertexAttribute::Position,
        VertexAttribute::Normal,
        VertexAttribute::TexCoord0,
        VertexAttribute::Color
    };
    
    // This should not crash even without context
    EXPECT_NO_THROW(renderer->setupVertexAttributes(attributes));
}



// Test uniform value creation
TEST_F(OpenGLRendererTest, UniformValues) {
    // Test float uniform
    UniformValue floatUniform(1.5f);
    EXPECT_EQ(floatUniform.type, UniformValue::Float);
    EXPECT_FLOAT_EQ(floatUniform.data.f, 1.5f);
    
    // Test vector uniforms
    Math::Vector2f vec2(1.0f, 2.0f);
    UniformValue vec2Uniform(vec2);
    EXPECT_EQ(vec2Uniform.type, UniformValue::Vec2);
    EXPECT_FLOAT_EQ(vec2Uniform.data.vec2[0], 1.0f);
    EXPECT_FLOAT_EQ(vec2Uniform.data.vec2[1], 2.0f);
    
    Math::Vector3f vec3(1.0f, 2.0f, 3.0f);
    UniformValue vec3Uniform(vec3);
    EXPECT_EQ(vec3Uniform.type, UniformValue::Vec3);
    EXPECT_FLOAT_EQ(vec3Uniform.data.vec3[0], 1.0f);
    EXPECT_FLOAT_EQ(vec3Uniform.data.vec3[1], 2.0f);
    EXPECT_FLOAT_EQ(vec3Uniform.data.vec3[2], 3.0f);
    
    Color color(0.5f, 0.6f, 0.7f, 0.8f);
    UniformValue colorUniform(color);
    EXPECT_EQ(colorUniform.type, UniformValue::Vec4);
    EXPECT_FLOAT_EQ(colorUniform.data.vec4[0], 0.5f);
    EXPECT_FLOAT_EQ(colorUniform.data.vec4[1], 0.6f);
    EXPECT_FLOAT_EQ(colorUniform.data.vec4[2], 0.7f);
    EXPECT_FLOAT_EQ(colorUniform.data.vec4[3], 0.8f);
    
    // Test integer uniform
    UniformValue intUniform(42);
    EXPECT_EQ(intUniform.type, UniformValue::Int);
    EXPECT_EQ(intUniform.data.i, 42);
    
    // Test matrix uniform
    Math::Matrix4f mat; // Default constructor creates identity matrix
    UniformValue matUniform(mat);
    EXPECT_EQ(matUniform.type, UniformValue::Mat4);
    EXPECT_FLOAT_EQ(matUniform.data.mat4[0], 1.0f); // First diagonal element
}

// Test capability queries
TEST_F(OpenGLRendererTest, CapabilityQueries) {
    // These should return default values without context
    EXPECT_GE(renderer->getMaxTextureSize(), 2048);
    EXPECT_GE(renderer->getMaxTextureUnits(), 16);
    EXPECT_GE(renderer->getMaxAnisotropy(), 1.0f);
    
    // Debug output should be false without context
    EXPECT_FALSE(renderer->supportsDebugOutput());
    EXPECT_FALSE(renderer->supportsTimestampQueries());
}




// Test vertex array setup with different attribute configurations
TEST_F(OpenGLRendererTest, VertexAttributeConfigurations) {
    // Test position-only vertices
    {
        std::vector<VertexAttribute> posOnly = {VertexAttribute::Position};
        EXPECT_NO_THROW(renderer->setupVertexAttributes(posOnly));
    }
    
    // Test position + normal
    {
        std::vector<VertexAttribute> posNormal = {
            VertexAttribute::Position,
            VertexAttribute::Normal
        };
        EXPECT_NO_THROW(renderer->setupVertexAttributes(posNormal));
    }
    
    // Test full vertex format
    {
        std::vector<VertexAttribute> fullFormat = {
            VertexAttribute::Position,
            VertexAttribute::Normal,
            VertexAttribute::TexCoord0,
            VertexAttribute::Color,
            VertexAttribute::Tangent
        };
        EXPECT_NO_THROW(renderer->setupVertexAttributes(fullFormat));
    }
    
    // Test custom attributes
    {
        std::vector<VertexAttribute> customFormat = {
            VertexAttribute::Position,
            VertexAttribute::Color,
            VertexAttribute::TexCoord0,
            VertexAttribute::TexCoord1
        };
        EXPECT_NO_THROW(renderer->setupVertexAttributes(customFormat));
    }
}


// Test error handling
TEST_F(OpenGLRendererTest, ErrorHandling) {
    // Test invalid resource access
    EXPECT_EQ(renderer->getBufferInfo(9999), nullptr);
    EXPECT_EQ(renderer->getTextureInfo(9999), nullptr);
    EXPECT_EQ(renderer->getShaderInfo(9999), nullptr);
    EXPECT_EQ(renderer->getProgramInfo(9999), nullptr);
    
    // These operations should not crash with invalid IDs
    EXPECT_NO_THROW(renderer->bindVertexBuffer(9999));
    EXPECT_NO_THROW(renderer->bindIndexBuffer(9999));
    EXPECT_NO_THROW(renderer->bindTexture(9999));
    EXPECT_NO_THROW(renderer->useProgram(9999));
    EXPECT_NO_THROW(renderer->deleteBuffer(9999));
    EXPECT_NO_THROW(renderer->deleteTexture(9999));
    EXPECT_NO_THROW(renderer->deleteShader(9999));
    EXPECT_NO_THROW(renderer->deleteProgram(9999));
}

} // namespace Rendering
} // namespace VoxelEditor