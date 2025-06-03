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

// Test context initialization
TEST_F(OpenGLRendererTest, InitializeContext) {
    RenderConfig config;
    config.windowWidth = 800;
    config.windowHeight = 600;
    config.vsync = true;
    config.enableDebugOutput = false;
    
    // Note: This test would fail without a proper GL context
    // In real testing, you'd need to set up a headless context
    EXPECT_FALSE(renderer->isContextValid());
    
    // Test double initialization protection
    EXPECT_FALSE(renderer->initializeContext(config));
}

// Test buffer creation and management
TEST_F(OpenGLRendererTest, BufferManagement) {
    // Test vertex buffer creation
    std::vector<float> vertices = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };
    
    // Without context, buffer creation should still assign IDs
    BufferId vb = renderer->createVertexBuffer(vertices.data(), 
                                               vertices.size() * sizeof(float), 
                                               BufferUsage::Static);
    EXPECT_NE(vb, InvalidId);
    
    // Test index buffer creation
    std::vector<uint32_t> indices = {0, 1, 2};
    BufferId ib = renderer->createIndexBuffer(indices.data(), 
                                              indices.size(), 
                                              BufferUsage::Static);
    EXPECT_NE(ib, InvalidId);
    
    // Test buffer info retrieval
    const BufferInfo* vbInfo = renderer->getBufferInfo(vb);
    EXPECT_NE(vbInfo, nullptr);
    if (vbInfo) {
        EXPECT_EQ(vbInfo->id, vb);
        EXPECT_EQ(vbInfo->size, vertices.size() * sizeof(float));
        EXPECT_FALSE(vbInfo->isIndexBuffer);
    }
    
    const BufferInfo* ibInfo = renderer->getBufferInfo(ib);
    EXPECT_NE(ibInfo, nullptr);
    if (ibInfo) {
        EXPECT_EQ(ibInfo->id, ib);
        EXPECT_EQ(ibInfo->size, indices.size() * sizeof(uint32_t));
        EXPECT_TRUE(ibInfo->isIndexBuffer);
    }
    
    // Test buffer deletion
    renderer->deleteBuffer(vb);
    EXPECT_EQ(renderer->getBufferInfo(vb), nullptr);
    
    // Test memory statistics
    size_t totalMem = renderer->getTotalBufferMemory();
    EXPECT_EQ(totalMem, indices.size() * sizeof(uint32_t));
}

// Test shader creation and compilation
TEST_F(OpenGLRendererTest, ShaderManagement) {
    // Test vertex shader creation
    const std::string vertexSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )";
    
    ShaderId vs = renderer->createShader(ShaderType::Vertex, vertexSource);
    EXPECT_NE(vs, InvalidId);
    
    // Test fragment shader creation
    const std::string fragmentSource = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    )";
    
    ShaderId fs = renderer->createShader(ShaderType::Fragment, fragmentSource);
    EXPECT_NE(fs, InvalidId);
    
    // Test shader info retrieval
    const ShaderInfo* vsInfo = renderer->getShaderInfo(vs);
    EXPECT_NE(vsInfo, nullptr);
    if (vsInfo) {
        EXPECT_EQ(vsInfo->id, vs);
        EXPECT_EQ(vsInfo->type, ShaderType::Vertex);
        EXPECT_EQ(vsInfo->source, vertexSource);
    }
    
    // Test program creation
    std::vector<ShaderId> shaders = {vs, fs};
    ShaderId program = renderer->createProgram(shaders);
    EXPECT_NE(program, InvalidId);
    
    // Test program info retrieval
    const ProgramInfo* programInfo = renderer->getProgramInfo(program);
    EXPECT_NE(programInfo, nullptr);
    if (programInfo) {
        EXPECT_EQ(programInfo->id, program);
        EXPECT_EQ(programInfo->shaders.size(), 2);
    }
    
    // Test shader deletion
    renderer->deleteShader(vs);
    EXPECT_EQ(renderer->getShaderInfo(vs), nullptr);
    
    renderer->deleteProgram(program);
    EXPECT_EQ(renderer->getProgramInfo(program), nullptr);
}

// Test texture creation and management
TEST_F(OpenGLRendererTest, TextureManagement) {
    // Test 2D texture creation
    const int width = 256;
    const int height = 256;
    std::vector<uint8_t> data(width * height * 4, 255); // White texture
    
    TextureId tex2D = renderer->createTexture2D(width, height, 
                                                 TextureFormat::RGBA8, 
                                                 data.data());
    EXPECT_NE(tex2D, InvalidId);
    
    // Test texture info retrieval
    const TextureInfo* texInfo = renderer->getTextureInfo(tex2D);
    EXPECT_NE(texInfo, nullptr);
    if (texInfo) {
        EXPECT_EQ(texInfo->id, tex2D);
        EXPECT_EQ(texInfo->width, width);
        EXPECT_EQ(texInfo->height, height);
        EXPECT_EQ(texInfo->format, TextureFormat::RGBA8);
        EXPECT_GT(texInfo->memorySize, 0);
    }
    
    // Test cube texture creation
    const int cubeSize = 128;
    std::vector<uint8_t> cubeData(cubeSize * cubeSize * 4, 128);
    const void* cubeFaces[6] = {
        cubeData.data(), cubeData.data(), cubeData.data(),
        cubeData.data(), cubeData.data(), cubeData.data()
    };
    
    TextureId texCube = renderer->createTextureCube(cubeSize, 
                                                     TextureFormat::RGBA8, 
                                                     cubeFaces);
    EXPECT_NE(texCube, InvalidId);
    
    // Test texture deletion
    renderer->deleteTexture(tex2D);
    EXPECT_EQ(renderer->getTextureInfo(tex2D), nullptr);
    
    // Test memory statistics
    size_t totalTexMem = renderer->getTotalTextureMemory();
    EXPECT_GT(totalTexMem, 0);
}

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

// Test render state management
TEST_F(OpenGLRendererTest, RenderStateManagement) {
    // Test depth test
    EXPECT_NO_THROW(renderer->setDepthTest(true));
    EXPECT_NO_THROW(renderer->setDepthTest(false));
    
    // Test depth write
    EXPECT_NO_THROW(renderer->setDepthWrite(true));
    EXPECT_NO_THROW(renderer->setDepthWrite(false));
    
    // Test blending
    EXPECT_NO_THROW(renderer->setBlending(true, BlendMode::Alpha));
    EXPECT_NO_THROW(renderer->setBlending(false));
    
    // Test culling
    EXPECT_NO_THROW(renderer->setCulling(true, CullMode::Back));
    EXPECT_NO_THROW(renderer->setCulling(true, CullMode::Front));
    EXPECT_NO_THROW(renderer->setCulling(false));
    
    // Test polygon mode
    EXPECT_NO_THROW(renderer->setPolygonMode(true));  // Wireframe
    EXPECT_NO_THROW(renderer->setPolygonMode(false)); // Solid
    
    // Test line width
    EXPECT_NO_THROW(renderer->setLineWidth(2.0f));
    
    // Test point size
    EXPECT_NO_THROW(renderer->setPointSize(5.0f));
}

// Test viewport and clear operations
TEST_F(OpenGLRendererTest, ViewportAndClear) {
    // Test viewport setting
    EXPECT_NO_THROW(renderer->setViewport(0, 0, 800, 600));
    
    // Test clear color
    Color clearColor(0.2f, 0.3f, 0.4f, 1.0f);
    EXPECT_NO_THROW(renderer->setClearColor(clearColor));
    
    // Test clear operations
    EXPECT_NO_THROW(renderer->clear(ClearFlags::All, clearColor, 1.0f, 0));
    EXPECT_NO_THROW(renderer->clear(ClearFlags::COLOR | ClearFlags::DEPTH));
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

// Test resource counting
TEST_F(OpenGLRendererTest, ResourceCounting) {
    // Create some resources
    BufferId vb1 = renderer->createVertexBuffer(nullptr, 100, BufferUsage::Static);
    BufferId vb2 = renderer->createVertexBuffer(nullptr, 200, BufferUsage::Dynamic);
    TextureId tex1 = renderer->createTexture2D(64, 64, TextureFormat::RGBA8);
    TextureId tex2 = renderer->createTexture2D(128, 128, TextureFormat::RGB8);
    
    EXPECT_EQ(renderer->getActiveBufferCount(), 2);
    EXPECT_EQ(renderer->getActiveTextureCount(), 2);
    
    // Delete some resources
    renderer->deleteBuffer(vb1);
    renderer->deleteTexture(tex1);
    
    EXPECT_EQ(renderer->getActiveBufferCount(), 1);
    EXPECT_EQ(renderer->getActiveTextureCount(), 1);
    
    // Clean up remaining resources
    renderer->deleteBuffer(vb2);
    renderer->deleteTexture(tex2);
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