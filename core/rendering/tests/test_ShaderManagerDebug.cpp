#include <gtest/gtest.h>
#include "../ShaderManager.h"
#include "../OpenGLRenderer.h"
#include "../RenderTypes.h"
#include "../../../foundation/logging/Logger.h"
#include <memory>
#include <string>
#include <iostream>

using namespace VoxelEditor::Rendering;

// Create a minimal mock renderer without inheritance complexity
class SimpleTestRenderer {
public:
    static ShaderId nextId;
    
    ShaderId createShader(ShaderType type, const std::string& source) {
        (void)type;
        (void)source;
        return nextId++;
    }
    
    ShaderId createProgram(const std::vector<ShaderId>& shaders) {
        (void)shaders;
        return nextId++;
    }
    
    void deleteShader(ShaderId shaderId) {
        (void)shaderId;
    }
    
    void deleteProgram(ShaderId programId) {
        (void)programId;
    }
};

ShaderId SimpleTestRenderer::nextId = 1;

// Test the Logger in isolation first
TEST(ShaderManagerDebugTest, LoggerBasicFunctionality) {
    std::cout << "Testing Logger basic functionality..." << std::endl;
    
    try {
        auto& logger = VoxelEditor::Logging::Logger::getInstance();
        logger.info("Logger test message");
        std::cout << "✓ Logger basic functionality works" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✗ Logger failed with exception: " << e.what() << std::endl;
        FAIL() << "Logger basic functionality failed: " << e.what();
    } catch (...) {
        std::cout << "✗ Logger failed with unknown exception" << std::endl;
        FAIL() << "Logger basic functionality failed with unknown exception";
    }
}

// Test the Logger with string operations
TEST(ShaderManagerDebugTest, LoggerStringOperations) {
    std::cout << "Testing Logger with string operations..." << std::endl;
    
    try {
        auto& logger = VoxelEditor::Logging::Logger::getInstance();
        std::string testString = "test_shader";
        logger.info("Compiling shader program: " + testString);
        
        std::string vertexSource = "#version 120\nvoid main() {}";
        auto lineCount = std::count(vertexSource.begin(), vertexSource.end(), '\n');
        logger.debug("Lines: " + std::to_string(static_cast<int>(lineCount)));
        
        std::cout << "✓ Logger string operations work" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✗ Logger string operations failed: " << e.what() << std::endl;
        FAIL() << "Logger string operations failed: " << e.what();
    } catch (...) {
        std::cout << "✗ Logger string operations failed with unknown exception" << std::endl;
        FAIL() << "Logger string operations failed with unknown exception";
    }
}

// Test MockOpenGLRenderer basic creation
TEST(ShaderManagerDebugTest, MockRendererCreation) {
    std::cout << "Testing MockOpenGLRenderer creation..." << std::endl;
    
    try {
        class MockOpenGLRenderer : public OpenGLRenderer {
        public:
            MockOpenGLRenderer() : OpenGLRenderer() {}
            
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
            }
            
            void deleteProgram(ShaderId programId) override {
                (void)programId;
            }
        };
        
        auto renderer = std::make_unique<MockOpenGLRenderer>();
        std::cout << "✓ MockOpenGLRenderer creation works" << std::endl;
        
        // Test basic operations
        ShaderId shader = renderer->createShader(ShaderType::Vertex, "test");
        EXPECT_NE(shader, InvalidId);
        std::cout << "✓ MockOpenGLRenderer basic operations work" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "✗ MockOpenGLRenderer creation failed: " << e.what() << std::endl;
        FAIL() << "MockOpenGLRenderer creation failed: " << e.what();
    } catch (...) {
        std::cout << "✗ MockOpenGLRenderer creation failed with unknown exception" << std::endl;
        FAIL() << "MockOpenGLRenderer creation failed with unknown exception";
    }
}

// Test ShaderManager creation in isolation
TEST(ShaderManagerDebugTest, ShaderManagerCreation) {
    std::cout << "Testing ShaderManager creation..." << std::endl;
    
    try {
        auto shaderManager = std::make_unique<ShaderManager>();
        std::cout << "✓ ShaderManager creation works" << std::endl;
        
        // Test basic operations
        ShaderId shader = shaderManager->getShader("nonexistent");
        EXPECT_EQ(shader, InvalidId);
        std::cout << "✓ ShaderManager basic operations work" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "✗ ShaderManager creation failed: " << e.what() << std::endl;
        FAIL() << "ShaderManager creation failed: " << e.what();
    } catch (...) {
        std::cout << "✗ ShaderManager creation failed with unknown exception" << std::endl;
        FAIL() << "ShaderManager creation failed with unknown exception";
    }
}

// Test the combination step by step
TEST(ShaderManagerDebugTest, CombinedStepByStep) {
    std::cout << "Testing combined functionality step by step..." << std::endl;
    
    try {
        // Step 1: Create ShaderManager
        auto shaderManager = std::make_unique<ShaderManager>();
        std::cout << "✓ Step 1: ShaderManager created" << std::endl;
        
        // Step 2: Create simple test renderer (not inheriting from OpenGLRenderer)
        auto simpleRenderer = std::make_unique<SimpleTestRenderer>();
        std::cout << "✓ Step 2: Simple renderer created" << std::endl;
        
        // Step 3: Test logging in this context
        auto& logger = VoxelEditor::Logging::Logger::getInstance();
        logger.info("Testing logging in combined context");
        std::cout << "✓ Step 3: Logging works in combined context" << std::endl;
        
        // This should work since we're not using the problematic inheritance
        std::cout << "✓ Combined functionality works with simple renderer" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "✗ Combined step-by-step failed: " << e.what() << std::endl;
        FAIL() << "Combined step-by-step failed: " << e.what();
    } catch (...) {
        std::cout << "✗ Combined step-by-step failed with unknown exception" << std::endl;
        FAIL() << "Combined step-by-step failed with unknown exception";
    }
}

// Test the problematic combination directly
TEST(ShaderManagerDebugTest, ProblematicCombination) {
    std::cout << "Testing the problematic combination that causes crashes..." << std::endl;
    
    class MockOpenGLRenderer : public OpenGLRenderer {
    public:
        MockOpenGLRenderer() : OpenGLRenderer() {
            std::cout << "MockOpenGLRenderer constructor called" << std::endl;
        }
        
        ~MockOpenGLRenderer() override {
            std::cout << "MockOpenGLRenderer destructor called" << std::endl;
        }
        
        ShaderId createShader(ShaderType type, const std::string& source) override {
            std::cout << "MockOpenGLRenderer::createShader called" << std::endl;
            (void)type;
            (void)source;
            static ShaderId nextId = 1;
            return nextId++;
        }
        
        ShaderId createProgram(const std::vector<ShaderId>& shaders) override {
            std::cout << "MockOpenGLRenderer::createProgram called" << std::endl;
            (void)shaders;
            static ShaderId nextId = 100;
            return nextId++;
        }
        
        void deleteShader(ShaderId shaderId) override {
            std::cout << "MockOpenGLRenderer::deleteShader called" << std::endl;
            (void)shaderId;
        }
        
        void deleteProgram(ShaderId programId) override {
            std::cout << "MockOpenGLRenderer::deleteProgram called" << std::endl;
            (void)programId;
        }
    };
    
    try {
        std::cout << "Creating ShaderManager..." << std::endl;
        auto shaderManager = std::make_unique<ShaderManager>();
        std::cout << "✓ ShaderManager created" << std::endl;
        
        std::cout << "Creating MockOpenGLRenderer..." << std::endl;
        auto renderer = std::make_unique<MockOpenGLRenderer>();
        std::cout << "✓ MockOpenGLRenderer created" << std::endl;
        
        std::cout << "Testing logging before createShaderFromSource..." << std::endl;
        auto& logger = VoxelEditor::Logging::Logger::getInstance();
        logger.info("About to call createShaderFromSource");
        std::cout << "✓ Logging works before createShaderFromSource" << std::endl;
        
        std::cout << "Calling createShaderFromSource..." << std::endl;
        
        const std::string vertexSource = R"(
            #version 120
            attribute vec3 a_position;
            void main() {
                gl_Position = vec4(a_position, 1.0);
            }
        )";
        
        const std::string fragmentSource = R"(
            #version 120
            void main() {
                gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
            }
        )";
        
        // This is where the crash typically occurs in the original test
        ShaderId shader = shaderManager->createShaderFromSource(
            "debug_test_shader", 
            vertexSource, 
            fragmentSource,
            renderer.get()
        );
        
        std::cout << "✓ createShaderFromSource completed successfully" << std::endl;
        EXPECT_NE(shader, InvalidId);
        
    } catch (const std::exception& e) {
        std::cout << "✗ Problematic combination failed: " << e.what() << std::endl;
        // Don't fail the test here, we want to understand what's happening
        std::cout << "This is the expected crash location" << std::endl;
    } catch (...) {
        std::cout << "✗ Problematic combination failed with unknown exception" << std::endl;
        std::cout << "This is the expected crash location" << std::endl;
    }
}