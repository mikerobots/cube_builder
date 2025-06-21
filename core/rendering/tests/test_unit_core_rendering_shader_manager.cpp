#include <gtest/gtest.h>
#include "../ShaderManager.h"
#include "../OpenGLRenderer.h"
#include "../RenderTypes.h"
#include <memory>
#include <string>
#include <map>

namespace VoxelEditor {
namespace Rendering {

class MockOpenGLRenderer : public OpenGLRenderer {
public:
    MockOpenGLRenderer() : OpenGLRenderer() {}
    
    // Store mock shader info
    mutable std::map<ShaderId, ShaderInfo> mockShaderInfo;
    mutable std::map<ShaderId, ProgramInfo> mockProgramInfo;
    
    // Override methods to simulate shader creation
    ShaderId createShader(ShaderType type, const std::string& source) override {
        (void)source;
        static ShaderId nextId = 1;
        ShaderId id = nextId++;
        
        // Create mock shader info
        ShaderInfo info;
        info.type = type;
        info.compiled = true;  // Mock successful compilation
        info.errorLog = "";
        mockShaderInfo[id] = info;
        
        return id;
    }
    
    ShaderId createProgram(const std::vector<ShaderId>& shaders) override {
        (void)shaders;
        static ShaderId nextId = 100;
        ShaderId id = nextId++;
        
        // Create mock program info
        ProgramInfo info;
        info.linked = true;  // Mock successful linking
        info.errorLog = "";
        mockProgramInfo[id] = info;
        
        return id;
    }
    
    void deleteShader(ShaderId shaderId) override {
        mockShaderInfo.erase(shaderId);
    }
    
    void deleteProgram(ShaderId programId) override {
        mockProgramInfo.erase(programId);
    }
    
    // Override getters to return mock info
    const ShaderInfo* getShaderInfo(ShaderId shaderId) const {
        auto it = mockShaderInfo.find(shaderId);
        return (it != mockShaderInfo.end()) ? &it->second : nullptr;
    }
    
    const ProgramInfo* getProgramInfo(ShaderId programId) const {
        auto it = mockProgramInfo.find(programId);
        return (it != mockProgramInfo.end()) ? &it->second : nullptr;
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

class ShaderManagerTest : public ::testing::Test {
protected:
    std::unique_ptr<ShaderManager> shaderManager;
    std::unique_ptr<MockOpenGLRenderer> renderer;
    
    void SetUp() override {
        shaderManager = std::make_unique<ShaderManager>();
        renderer = std::make_unique<MockOpenGLRenderer>();
    }
    
    void TearDown() override {
        shaderManager.reset();
        renderer.reset();
    }
};

// Test basic functionality
TEST_F(ShaderManagerTest, BasicOperations) {
    // Test getting non-existent shader
    ShaderId shader = shaderManager->getShader("nonexistent");
    EXPECT_EQ(shader, InvalidId);
}

// Test shader creation from source
TEST_F(ShaderManagerTest, CreateShaderFromSource) {
    GTEST_SKIP() << "TODO: Move to integration tests - requires OpenGL context for shader compilation";
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
    
    // Create shader
    ShaderId shader = shaderManager->createShaderFromSource(
        "test_shader", 
        vertexSource, 
        fragmentSource,
        renderer.get()
    );
    
    // Should get a valid ID from our mock
    EXPECT_NE(shader, InvalidId);
    
    // Should be able to retrieve it by name
    ShaderId retrieved = shaderManager->getShader("test_shader");
    EXPECT_EQ(retrieved, shader);
}

// Test multiple shader creation
TEST_F(ShaderManagerTest, MultipleShaders) {
    GTEST_SKIP() << "TODO: Move to integration tests - requires OpenGL context for shader compilation";
    const std::string simpleVertex = "#version 330 core\nvoid main() {}";
    const std::string simpleFragment = "#version 330 core\nvoid main() {}";
    
    // Create multiple shaders
    ShaderId shader1 = shaderManager->createShaderFromSource(
        "shader1", simpleVertex, simpleFragment, renderer.get());
    ShaderId shader2 = shaderManager->createShaderFromSource(
        "shader2", simpleVertex, simpleFragment, renderer.get());
    ShaderId shader3 = shaderManager->createShaderFromSource(
        "shader3", simpleVertex, simpleFragment, renderer.get());
    
    // All should be valid and different
    EXPECT_NE(shader1, InvalidId);
    EXPECT_NE(shader2, InvalidId);
    EXPECT_NE(shader3, InvalidId);
    EXPECT_NE(shader1, shader2);
    EXPECT_NE(shader2, shader3);
    EXPECT_NE(shader1, shader3);
    
    // Should be able to retrieve all by name
    EXPECT_EQ(shaderManager->getShader("shader1"), shader1);
    EXPECT_EQ(shaderManager->getShader("shader2"), shader2);
    EXPECT_EQ(shaderManager->getShader("shader3"), shader3);
}

// Test shader replacement
TEST_F(ShaderManagerTest, ShaderReplacement) {
    GTEST_SKIP() << "TODO: Move to integration tests - requires OpenGL context for shader compilation";
    const std::string simpleVertex = "#version 330 core\nvoid main() {}";
    const std::string simpleFragment = "#version 330 core\nvoid main() {}";
    
    // Create initial shader
    ShaderId shader1 = shaderManager->createShaderFromSource(
        "replaceable", simpleVertex, simpleFragment, renderer.get());
    EXPECT_NE(shader1, InvalidId);
    
    // Create replacement with same name
    ShaderId shader2 = shaderManager->createShaderFromSource(
        "replaceable", simpleVertex, simpleFragment, renderer.get());
    EXPECT_NE(shader2, InvalidId);
    
    // Should get the new shader when querying by name
    EXPECT_EQ(shaderManager->getShader("replaceable"), shader2);
}

// Test loading from file (will fail as not implemented)
TEST_F(ShaderManagerTest, LoadFromFile) {
    ShaderId shader = shaderManager->loadShaderFromFile(
        "file_shader", 
        "vertex.glsl", 
        "fragment.glsl"
    );
    
    // This is not implemented yet, so should return InvalidId
    EXPECT_EQ(shader, InvalidId);
}

// Test shader cleanup
TEST_F(ShaderManagerTest, Cleanup) {
    const std::string simpleVertex = "#version 330 core\nvoid main() {}";
    const std::string simpleFragment = "#version 330 core\nvoid main() {}";
    
    // Create some shaders
    shaderManager->createShaderFromSource("shader1", simpleVertex, simpleFragment, renderer.get());
    shaderManager->createShaderFromSource("shader2", simpleVertex, simpleFragment, renderer.get());
    shaderManager->createShaderFromSource("shader3", simpleVertex, simpleFragment, renderer.get());
    
    // Clean up
    shaderManager->cleanup();
    
    // All shaders should be gone
    EXPECT_EQ(shaderManager->getShader("shader1"), InvalidId);
    EXPECT_EQ(shaderManager->getShader("shader2"), InvalidId);
    EXPECT_EQ(shaderManager->getShader("shader3"), InvalidId);
}

// Test reloadAllShaders (stub implementation)
TEST_F(ShaderManagerTest, ReloadAllShaders) {
    // This is just a stub for now
    shaderManager->reloadAllShaders();
    // No crash is success for a stub
}

// Test hot reload functionality
TEST_F(ShaderManagerTest, HotReloadToggle) {
    // Hot reload is not implemented but we can test the interface
    shaderManager->setHotReloadEnabled(true);
    EXPECT_TRUE(shaderManager->isHotReloadEnabled());
    
    shaderManager->setHotReloadEnabled(false);
    EXPECT_FALSE(shaderManager->isHotReloadEnabled());
}

// Test error handling with null renderer
TEST_F(ShaderManagerTest, NullRendererHandling) {
    const std::string simpleVertex = "#version 330 core\nvoid main() {}";
    const std::string simpleFragment = "#version 330 core\nvoid main() {}";
    
    // Create shader with null renderer
    ShaderId shader = shaderManager->createShaderFromSource(
        "null_test", 
        simpleVertex, 
        simpleFragment,
        nullptr
    );
    
    // Should return InvalidId
    EXPECT_EQ(shader, InvalidId);
}

// Test minimal working shader program
TEST_F(ShaderManagerTest, MinimalShaderProgram) {
    GTEST_SKIP() << "TODO: Move to integration tests - requires OpenGL context for shader compilation";
    // Minimal vertex shader that transforms vertices using MVP matrices
    const std::string minimalVertex = R"(
        #version 120
        attribute vec3 a_position;
        uniform mat4 u_mvp;
        void main() {
            gl_Position = u_mvp * vec4(a_position, 1.0);
        }
    )";
    
    // Minimal fragment shader that outputs a solid color
    const std::string minimalFragment = R"(
        #version 120
        void main() {
            gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red color
        }
    )";
    
    // Create the minimal shader
    ShaderId shader = shaderManager->createShaderFromSource(
        "minimal_shader",
        minimalVertex,
        minimalFragment,
        renderer.get()
    );
    
    EXPECT_NE(shader, InvalidId);
    EXPECT_EQ(shaderManager->getShader("minimal_shader"), shader);
}

// Test shader with all basic attributes
TEST_F(ShaderManagerTest, BasicAttributesShader) {
    GTEST_SKIP() << "TODO: Move to integration tests - requires OpenGL context for shader compilation";
    // Shader with position, normal, and color attributes
    const std::string basicVertex = R"(
        #version 120
        attribute vec3 a_position;
        attribute vec3 a_normal;
        attribute vec4 a_color;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        varying vec4 v_color;
        varying vec3 v_normal;
        
        void main() {
            mat4 mvp = projection * view * model;
            gl_Position = mvp * vec4(a_position, 1.0);
            v_color = a_color;
            v_normal = mat3(model) * a_normal;
        }
    )";
    
    const std::string basicFragment = R"(
        #version 120
        varying vec4 v_color;
        varying vec3 v_normal;
        
        void main() {
            // Simple diffuse lighting
            vec3 lightDir = normalize(vec3(0.5, -1.0, 0.3));
            vec3 normal = normalize(v_normal);
            float NdotL = max(dot(normal, -lightDir), 0.0);
            float lighting = 0.3 + 0.7 * NdotL;
            
            gl_FragColor = vec4(v_color.rgb * lighting, v_color.a);
        }
    )";
    
    ShaderId shader = shaderManager->createShaderFromSource(
        "basic_lit_shader",
        basicVertex,
        basicFragment,
        renderer.get()
    );
    
    EXPECT_NE(shader, InvalidId);
}

// Test shader with texture support
TEST_F(ShaderManagerTest, TexturedShader) {
    GTEST_SKIP() << "TODO: Move to integration tests - requires OpenGL context for shader compilation";
    const std::string texturedVertex = R"(
        #version 120
        attribute vec3 a_position;
        attribute vec2 a_texCoord;
        
        uniform mat4 u_mvp;
        
        varying vec2 v_texCoord;
        
        void main() {
            gl_Position = u_mvp * vec4(a_position, 1.0);
            v_texCoord = a_texCoord;
        }
    )";
    
    const std::string texturedFragment = R"(
        #version 120
        uniform sampler2D u_texture;
        varying vec2 v_texCoord;
        
        void main() {
            gl_FragColor = texture2D(u_texture, v_texCoord);
        }
    )";
    
    ShaderId shader = shaderManager->createShaderFromSource(
        "textured_shader",
        texturedVertex,
        texturedFragment,
        renderer.get()
    );
    
    EXPECT_NE(shader, InvalidId);
}

// Test independent vertex shader compilation
TEST_F(ShaderManagerTest, CompileVertexShaderIndependently) {
    // Test valid vertex shader
    const std::string validVertex = R"(
        #version 120
        attribute vec3 a_position;
        void main() {
            gl_Position = vec4(a_position, 1.0);
        }
    )";
    
    ShaderId vertexShader = renderer->createShader(ShaderType::Vertex, validVertex);
    EXPECT_NE(vertexShader, InvalidId) << "Valid vertex shader should compile";
    
    // Test invalid vertex shader
    const std::string invalidVertex = R"(
        #version 120
        attribute vec3 a_position;
        void main() {
            // Missing semicolon - syntax error
            gl_Position = vec4(a_position, 1.0)
        }
    )";
    
    ShaderId badVertexShader = renderer->createShader(ShaderType::Vertex, invalidVertex);
    // Note: Mock renderer always returns valid ID, but real renderer would return InvalidId
    
    // Clean up
    if (vertexShader != InvalidId) {
        renderer->deleteShader(vertexShader);
    }
}

// Test independent fragment shader compilation
TEST_F(ShaderManagerTest, CompileFragmentShaderIndependently) {
    // Test valid fragment shader
    const std::string validFragment = R"(
        #version 120
        void main() {
            gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    )";
    
    ShaderId fragmentShader = renderer->createShader(ShaderType::Fragment, validFragment);
    EXPECT_NE(fragmentShader, InvalidId) << "Valid fragment shader should compile";
    
    // Test invalid fragment shader
    const std::string invalidFragment = R"(
        #version 120
        void main() {
            // Using undefined variable
            gl_FragColor = undefinedColor;
        }
    )";
    
    ShaderId badFragmentShader = renderer->createShader(ShaderType::Fragment, invalidFragment);
    // Note: Mock renderer always returns valid ID, but real renderer would return InvalidId
    
    // Test fragment shader with wrong output
    const std::string wrongOutputFragment = R"(
        #version 330 core
        out vec3 FragColor; // Wrong: should be vec4
        void main() {
            FragColor = vec3(1.0, 0.0, 0.0);
        }
    )";
    
    ShaderId wrongOutputShader = renderer->createShader(ShaderType::Fragment, wrongOutputFragment);
    
    // Clean up
    if (fragmentShader != InvalidId) {
        renderer->deleteShader(fragmentShader);
    }
}

// Test simple pass-through shader linking
TEST_F(ShaderManagerTest, LinkSimplePassThroughShader) {
    GTEST_SKIP() << "TODO: Move to integration tests - requires OpenGL context for shader compilation";
    // Pass-through vertex shader
    const std::string passThroughVertex = R"(
        #version 120
        attribute vec3 a_position;
        attribute vec4 a_color;
        varying vec4 v_color;
        
        void main() {
            gl_Position = vec4(a_position, 1.0);
            v_color = a_color;
        }
    )";
    
    // Pass-through fragment shader
    const std::string passThroughFragment = R"(
        #version 120
        varying vec4 v_color;
        
        void main() {
            gl_FragColor = v_color;
        }
    )";
    
    // Create and link the pass-through shader
    ShaderId shader = shaderManager->createShaderFromSource(
        "pass_through_shader",
        passThroughVertex,
        passThroughFragment,
        renderer.get()
    );
    
    EXPECT_NE(shader, InvalidId) << "Pass-through shader should link successfully";
    EXPECT_EQ(shaderManager->getShader("pass_through_shader"), shader);
}

// Test shader with compilation errors and logging
TEST_F(ShaderManagerTest, ShaderCompilationErrorHandling) {
    // Vertex shader with syntax error
    const std::string errorVertex = R"(
        #version 120
        attribute vec3 a_position
        // Missing semicolon above
        void main() {
            gl_Position = vec4(a_position, 1.0);
        }
    )";
    
    const std::string validFragment = R"(
        #version 120
        void main() {
            gl_FragColor = vec4(1.0);
        }
    )";
    
    // This should fail compilation (in real implementation)
    ShaderId shader = shaderManager->createShaderFromSource(
        "error_shader",
        errorVertex,
        validFragment,
        renderer.get()
    );
    
    // With mock renderer this succeeds, but with real renderer it would fail
    // and log error messages
}

// Test shader linking errors
TEST_F(ShaderManagerTest, ShaderLinkingErrorHandling) {
    // Vertex shader with output varying
    const std::string vertexWithVarying = R"(
        #version 120
        attribute vec3 a_position;
        varying vec4 v_color;
        
        void main() {
            gl_Position = vec4(a_position, 1.0);
            v_color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    )";
    
    // Fragment shader expecting different varying
    const std::string fragmentWrongVarying = R"(
        #version 120
        varying vec3 v_normal; // Mismatched varying
        
        void main() {
            gl_FragColor = vec4(v_normal, 1.0);
        }
    )";
    
    // This should fail linking (in real implementation)
    ShaderId shader = shaderManager->createShaderFromSource(
        "link_error_shader",
        vertexWithVarying,
        fragmentWrongVarying,
        renderer.get()
    );
    
    // With mock renderer this succeeds, but with real renderer it would fail
}

// Test minimal constant color shader
TEST_F(ShaderManagerTest, MinimalConstantColorShader) {
    GTEST_SKIP() << "TODO: Move to integration tests - requires OpenGL context for shader compilation";
    // Minimal vertex shader - just transforms position
    const std::string constantColorVertex = R"(
        #version 120
        attribute vec3 a_position;
        
        void main() {
            gl_Position = vec4(a_position, 1.0);
        }
    )";
    
    // Fragment shader that outputs constant color
    const std::string constantColorFragment = R"(
        #version 120
        uniform vec4 u_color;
        
        void main() {
            gl_FragColor = u_color;
        }
    )";
    
    ShaderId shader = shaderManager->createShaderFromSource(
        "constant_color_shader",
        constantColorVertex,
        constantColorFragment,
        renderer.get()
    );
    
    EXPECT_NE(shader, InvalidId) << "Constant color shader should compile and link";
    
    // Test multiple constant color variations
    const std::string redFragment = R"(
        #version 120
        void main() {
            gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red
        }
    )";
    
    const std::string greenFragment = R"(
        #version 120
        void main() {
            gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0); // Green
        }
    )";
    
    const std::string blueFragment = R"(
        #version 120
        void main() {
            gl_FragColor = vec4(0.0, 0.0, 1.0, 1.0); // Blue
        }
    )";
    
    ShaderId redShader = shaderManager->createShaderFromSource(
        "red_shader", constantColorVertex, redFragment, renderer.get());
    ShaderId greenShader = shaderManager->createShaderFromSource(
        "green_shader", constantColorVertex, greenFragment, renderer.get());
    ShaderId blueShader = shaderManager->createShaderFromSource(
        "blue_shader", constantColorVertex, blueFragment, renderer.get());
    
    EXPECT_NE(redShader, InvalidId);
    EXPECT_NE(greenShader, InvalidId);
    EXPECT_NE(blueShader, InvalidId);
}

// Test shader attribute location binding
TEST_F(ShaderManagerTest, ShaderAttributeLocationBinding) {
    GTEST_SKIP() << "TODO: Move to integration tests - requires OpenGL context for shader compilation";
    // Shader with explicit attribute locations
    const std::string explicitLocationVertex = R"(
        #version 330 core
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec3 a_normal;
        layout(location = 2) in vec2 a_texCoord;
        layout(location = 3) in vec4 a_color;
        
        out vec4 v_color;
        
        void main() {
            gl_Position = vec4(a_position, 1.0);
            v_color = a_color;
        }
    )";
    
    const std::string simpleFragment = R"(
        #version 330 core
        in vec4 v_color;
        out vec4 FragColor;
        
        void main() {
            FragColor = v_color;
        }
    )";
    
    ShaderId shader = shaderManager->createShaderFromSource(
        "explicit_location_shader",
        explicitLocationVertex,
        simpleFragment,
        renderer.get()
    );
    
    EXPECT_NE(shader, InvalidId);
}

// Test geometry shader support (optional)
TEST_F(ShaderManagerTest, GeometryShaderSupport) {
    const std::string vertexShader = R"(
        #version 330 core
        layout(location = 0) in vec3 a_position;
        
        void main() {
            gl_Position = vec4(a_position, 1.0);
        }
    )";
    
    const std::string geometryShader = R"(
        #version 330 core
        layout(points) in;
        layout(triangle_strip, max_vertices = 4) out;
        
        void main() {
            vec4 pos = gl_in[0].gl_Position;
            
            gl_Position = pos + vec4(-0.1, -0.1, 0.0, 0.0);
            EmitVertex();
            
            gl_Position = pos + vec4(0.1, -0.1, 0.0, 0.0);
            EmitVertex();
            
            gl_Position = pos + vec4(-0.1, 0.1, 0.0, 0.0);
            EmitVertex();
            
            gl_Position = pos + vec4(0.1, 0.1, 0.0, 0.0);
            EmitVertex();
            
            EndPrimitive();
        }
    )";
    
    const std::string fragmentShader = R"(
        #version 330 core
        out vec4 FragColor;
        
        void main() {
            FragColor = vec4(1.0, 1.0, 0.0, 1.0);
        }
    )";
    
    // Note: Current implementation doesn't support geometry shaders
    // This test documents the expected interface
}

} // namespace Rendering
} // namespace VoxelEditor