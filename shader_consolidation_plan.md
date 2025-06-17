# Simple Shader Consolidation Plan

## Goal
Create a single, self-contained directory with all shaders and minimal validation code that an LLM can easily work with to add, modify, and validate shaders without needing to understand the entire codebase.

## Proposed Structure

```
shaders/
├── README.md                    # Quick reference for LLMs
├── catalog.json                 # Simple shader inventory
├── sources/                     # All shader source files
│   ├── voxel/
│   │   ├── basic_voxel.vert
│   │   ├── basic_voxel.frag
│   │   ├── enhanced_voxel.frag
│   │   ├── flat_voxel.frag
│   │   └── ...
│   ├── ui/
│   │   ├── overlay.vert
│   │   ├── overlay.frag
│   │   ├── highlight.vert
│   │   ├── highlight.frag
│   │   └── ...
│   ├── ground/
│   │   ├── ground_plane.vert
│   │   ├── ground_plane.frag
│   │   └── grid_lines.frag
│   └── debug/
│       ├── wireframe.vert
│       ├── wireframe.frag
│       └── test_color.frag
├── validate_shader.cpp          # Simple validation tool
├── shader_test_runner.cpp       # Standalone test runner
└── CMakeLists.txt              # Build just this subsystem
```

## 1. Shader Catalog (catalog.json)

Simple JSON file that lists all shaders with minimal metadata:

```json
{
  "shaders": [
    {
      "id": "basic_voxel",
      "vertex": "sources/voxel/basic_voxel.vert",
      "fragment": "sources/voxel/basic_voxel.frag",
      "uniforms": ["model", "view", "projection", "lightPos", "lightColor", "viewPos"],
      "attributes": ["aPos", "aNormal", "aColor"],
      "description": "Basic voxel rendering with Phong lighting"
    },
    {
      "id": "ground_plane", 
      "vertex": "sources/ground/ground_plane.vert",
      "fragment": "sources/ground/ground_plane.frag",
      "uniforms": ["mvpMatrix", "gridScale", "gridOpacity"],
      "attributes": ["position"],
      "description": "Ground plane grid rendering"
    }
    // ... more shaders
  ]
}
```

## 2. Shader Source Management with Inline Fallbacks

All shaders will be organized into a centralized header file that maintains both file paths and inline fallback sources:

### ShaderSources.h
```cpp
// shaders/ShaderSources.h
#pragma once

namespace VoxelEditor {
namespace Shaders {

// Structure to hold shader source information
struct ShaderSource {
    const char* name;
    const char* vertexPath;
    const char* fragmentPath;
    const char* vertexSource;    // Inline fallback
    const char* fragmentSource;  // Inline fallback
};

// Basic Voxel Shader
namespace BasicVoxel {
    constexpr const char* VERTEX_SOURCE = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aColor;

out vec3 FragPos;
out vec3 Normal;
out vec4 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Color = aColor;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

    constexpr const char* FRAGMENT_SOURCE = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec4 Color;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main() {
    // Phong lighting implementation
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    vec3 result = (ambient + diffuse) * Color.rgb;
    FragColor = vec4(result, Color.a);
}
)";
}

// Highlight Shader (extracted from HighlightRenderer.cpp)
namespace Highlight {
    constexpr const char* VERTEX_SOURCE = R"(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

out vec3 v_normal;
out vec3 v_worldPos;

void main() {
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    v_worldPos = worldPos.xyz;
    v_normal = mat3(u_model) * a_normal;
    gl_Position = u_projection * u_view * worldPos;
}
)";

    constexpr const char* FRAGMENT_SOURCE = R"(
#version 330 core
in vec3 v_normal;
in vec3 v_worldPos;

uniform vec4 u_color;
uniform float u_time;

out vec4 FragColor;

void main() {
    // Simple highlight with optional animation
    FragColor = u_color;
}
)";
}

// All shader definitions
constexpr ShaderSource SHADER_SOURCES[] = {
    {
        "basic_voxel",
        "shaders/sources/voxel/basic_voxel.vert",
        "shaders/sources/voxel/basic_voxel.frag",
        BasicVoxel::VERTEX_SOURCE,
        BasicVoxel::FRAGMENT_SOURCE
    },
    {
        "highlight",
        "shaders/sources/ui/highlight.vert",
        "shaders/sources/ui/highlight.frag",
        Highlight::VERTEX_SOURCE,
        Highlight::FRAGMENT_SOURCE
    },
    // ... more shaders
};

} // namespace Shaders
} // namespace VoxelEditor
```

### Updated ShaderManager Loading
```cpp
// In ShaderManager.cpp or a new ShaderLoader.cpp
#include "shaders/ShaderSources.h"

ShaderId ShaderManager::loadShaderWithFallback(const std::string& name) {
    // Find shader definition
    const ShaderSource* source = nullptr;
    for (const auto& shader : Shaders::SHADER_SOURCES) {
        if (shader.name == name) {
            source = &shader;
            break;
        }
    }
    
    if (!source) {
        Logger::error("Unknown shader: " + name);
        return InvalidId;
    }
    
    // Try loading from file first
    ShaderId shader = loadShaderFromFile(name, source->vertexPath, source->fragmentPath);
    
    // Fall back to inline source if file loading fails
    if (shader == InvalidId) {
        Logger::warning("Failed to load shader from file, using inline fallback: " + name);
        shader = createShaderFromSource(name, 
                                      source->vertexSource, 
                                      source->fragmentSource,
                                      m_renderer);
    }
    
    return shader;
}
```

### Migration Process for Classes with Direct GL Compilation

For classes like HighlightRenderer that compile shaders directly:

```cpp
// OLD: Direct OpenGL compilation
void HighlightRenderer::createHighlightShader() {
    const char* vertexShaderSrc = R"(...)";
    const char* fragmentShaderSrc = R"(...)";
    
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);
    // ... direct GL calls ...
}

// NEW: Use ShaderManager with automatic fallback
void HighlightRenderer::createHighlightShader() {
    m_highlightShader = m_shaderManager->loadShaderWithFallback("highlight");
    
    if (m_highlightShader == InvalidId) {
        Logger::error("Failed to create highlight shader");
    }
}
```

### Benefits of This Approach

1. **Robustness**: Always have working shaders even if files are missing
2. **Single Source of Truth**: All shader code in one organized location
3. **Easy Testing**: Can test both file and inline versions
4. **Gradual Migration**: Can update classes one at a time
5. **Development Flexibility**: Can edit files for iteration, inline for distribution

## 3. Simple Validation Tool

```cpp
// shaders/validate_shader.cpp
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <json/json.h>

class ShaderValidator {
    GLFWwindow* window;
    
public:
    bool initialize() {
        if (!glfwInit()) return false;
        
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        
        window = glfwCreateWindow(1, 1, "Shader Validator", NULL, NULL);
        if (!window) return false;
        
        glfwMakeContextCurrent(window);
        return glewInit() == GLEW_OK;
    }
    
    bool validateShader(const std::string& vertexPath, const std::string& fragmentPath) {
        std::string vertexCode = readFile(vertexPath);
        std::string fragmentCode = readFile(fragmentPath);
        
        // Compile vertex shader
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        const char* vCode = vertexCode.c_str();
        glShaderSource(vertexShader, 1, &vCode, NULL);
        glCompileShader(vertexShader);
        
        if (!checkCompileErrors(vertexShader, "VERTEX", vertexPath)) {
            return false;
        }
        
        // Compile fragment shader
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        const char* fCode = fragmentCode.c_str();
        glShaderSource(fragmentShader, 1, &fCode, NULL);
        glCompileShader(fragmentShader);
        
        if (!checkCompileErrors(fragmentShader, "FRAGMENT", fragmentPath)) {
            glDeleteShader(vertexShader);
            return false;
        }
        
        // Link program
        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        
        bool success = checkLinkErrors(program, vertexPath + " + " + fragmentPath);
        
        // Cleanup
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        
        return success;
    }
    
    void validateAll() {
        std::cout << "Validating all shaders...\n\n";
        
        int filePassed = 0, fileFailed = 0;
        int inlinePassed = 0, inlineFailed = 0;
        
        // Include the shader sources header
        #include "ShaderSources.h"
        
        for (const auto& shader : Shaders::SHADER_SOURCES) {
            std::cout << "Testing " << shader.name << ":\n";
            
            // Test file version
            std::cout << "  File version: ";
            if (std::filesystem::exists(shader.vertexPath) && 
                std::filesystem::exists(shader.fragmentPath)) {
                if (validateShader(shader.vertexPath, shader.fragmentPath)) {
                    std::cout << "✓ PASSED\n";
                    filePassed++;
                } else {
                    std::cout << "✗ FAILED\n";
                    fileFailed++;
                }
            } else {
                std::cout << "- NOT FOUND\n";
            }
            
            // Test inline version
            std::cout << "  Inline version: ";
            if (validateShaderSource(shader.vertexSource, shader.fragmentSource, shader.name)) {
                std::cout << "✓ PASSED\n";
                inlinePassed++;
            } else {
                std::cout << "✗ FAILED\n";
                inlineFailed++;
            }
            
            std::cout << "\n";
        }
        
        std::cout << "\nSummary:\n";
        std::cout << "  File shaders: " << filePassed << " passed, " << fileFailed << " failed\n";
        std::cout << "  Inline shaders: " << inlinePassed << " passed, " << inlineFailed << " failed\n";
    }
    
    bool validateShaderSource(const std::string& vertexSource, 
                            const std::string& fragmentSource,
                            const std::string& name) {
        // Compile vertex shader from source
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        const char* vCode = vertexSource.c_str();
        glShaderSource(vertexShader, 1, &vCode, NULL);
        glCompileShader(vertexShader);
        
        if (!checkCompileErrors(vertexShader, "VERTEX", name + " (inline)")) {
            return false;
        }
        
        // Compile fragment shader from source
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        const char* fCode = fragmentSource.c_str();
        glShaderSource(fragmentShader, 1, &fCode, NULL);
        glCompileShader(fragmentShader);
        
        if (!checkCompileErrors(fragmentShader, "FRAGMENT", name + " (inline)")) {
            glDeleteShader(vertexShader);
            return false;
        }
        
        // Link program
        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        
        bool success = checkLinkErrors(program, name + " (inline)");
        
        // Cleanup
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        
        return success;
    }
    
    ~ShaderValidator() {
        if (window) glfwDestroyWindow(window);
        glfwTerminate();
    }
    
private:
    std::string readFile(const std::string& path) {
        std::ifstream file(path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    bool checkCompileErrors(GLuint shader, const std::string& type, const std::string& path) {
        GLint success;
        GLchar infoLog[1024];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "\nERROR: " << type << " shader compilation failed (" << path << ")\n"
                     << infoLog << "\n";
            return false;
        }
        return true;
    }
    
    bool checkLinkErrors(GLuint program, const std::string& name) {
        GLint success;
        GLchar infoLog[1024];
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        
        if (!success) {
            glGetProgramInfoLog(program, 1024, NULL, infoLog);
            std::cerr << "\nERROR: Shader program linking failed (" << name << ")\n"
                     << infoLog << "\n";
            return false;
        }
        return true;
    }
};

int main(int argc, char* argv[]) {
    ShaderValidator validator;
    
    if (!validator.initialize()) {
        std::cerr << "Failed to initialize OpenGL context\n";
        return 1;
    }
    
    if (argc > 2) {
        // Validate specific shader pair
        return validator.validateShader(argv[1], argv[2]) ? 0 : 1;
    } else {
        // Validate all shaders
        validator.validateAll();
        return 0;
    }
}
```

## 4. Test Mesh Generation

To properly validate shaders, we need appropriate test geometry for each shader type:

```cpp
// shaders/TestMeshGenerator.h
#pragma once
#include "math/Vector3f.h"
#include <vector>

namespace VoxelEditor {
namespace ShaderTest {

// Vertex formats for different shader types
struct BasicVertex {
    Math::Vector3f position;
    Math::Vector3f normal;
    Math::Vector4f color;
};

struct GridVertex {
    Math::Vector3f position;
    float isMajorLine;
};

struct UIVertex {
    Math::Vector3f position;
    Math::Vector2f texCoord;
};

class TestMeshGenerator {
public:
    // Voxel shader test meshes
    static std::vector<BasicVertex> createCube(float size = 1.0f) {
        std::vector<BasicVertex> vertices;
        
        // Front face (6 vertices per face for independent normals)
        vertices.push_back({{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}});
        vertices.push_back({{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}});
        vertices.push_back({{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}});
        vertices.push_back({{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}});
        vertices.push_back({{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}});
        vertices.push_back({{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}});
        
        // Scale by size
        for (auto& vertex : vertices) {
            vertex.position = vertex.position * size;
        }
        
        return vertices;
    }
    
    // Ground plane shader test mesh
    static std::vector<GridVertex> createGrid(float size = 10.0f, int divisions = 10) {
        std::vector<GridVertex> vertices;
        float step = size / divisions;
        float halfSize = size * 0.5f;
        
        // Lines parallel to X axis
        for (int i = 0; i <= divisions; ++i) {
            float z = -halfSize + i * step;
            bool isMajor = (i % 5) == 0;
            
            vertices.push_back({{-halfSize, 0.0f, z}, isMajor ? 1.0f : 0.0f});
            vertices.push_back({{ halfSize, 0.0f, z}, isMajor ? 1.0f : 0.0f});
        }
        
        // Lines parallel to Z axis  
        for (int i = 0; i <= divisions; ++i) {
            float x = -halfSize + i * step;
            bool isMajor = (i % 5) == 0;
            
            vertices.push_back({{x, 0.0f, -halfSize}, isMajor ? 1.0f : 0.0f});
            vertices.push_back({{x, 0.0f,  halfSize}, isMajor ? 1.0f : 0.0f});
        }
        
        return vertices;
    }
    
    // UI/Overlay shader test mesh
    static std::vector<UIVertex> createQuad(float width = 2.0f, float height = 2.0f) {
        std::vector<UIVertex> vertices = {
            {{-width/2, -height/2, 0.0f}, {0.0f, 0.0f}},
            {{ width/2, -height/2, 0.0f}, {1.0f, 0.0f}},
            {{ width/2,  height/2, 0.0f}, {1.0f, 1.0f}},
            {{-width/2, -height/2, 0.0f}, {0.0f, 0.0f}},
            {{ width/2,  height/2, 0.0f}, {1.0f, 1.0f}},
            {{-width/2,  height/2, 0.0f}, {0.0f, 1.0f}}
        };
        return vertices;
    }
    
    // Generate appropriate mesh based on shader type
    static void* generateTestMesh(const std::string& shaderName, size_t& vertexCount, size_t& vertexSize) {
        if (shaderName.find("voxel") != std::string::npos) {
            auto vertices = createCube();
            vertexCount = vertices.size();
            vertexSize = sizeof(BasicVertex);
            void* data = malloc(vertexCount * vertexSize);
            memcpy(data, vertices.data(), vertexCount * vertexSize);
            return data;
        }
        else if (shaderName.find("ground") != std::string::npos || 
                 shaderName.find("grid") != std::string::npos) {
            auto vertices = createGrid();
            vertexCount = vertices.size();
            vertexSize = sizeof(GridVertex);
            void* data = malloc(vertexCount * vertexSize);
            memcpy(data, vertices.data(), vertexCount * vertexSize);
            return data;
        }
        else if (shaderName.find("overlay") != std::string::npos ||
                 shaderName.find("ui") != std::string::npos) {
            auto vertices = createQuad();
            vertexCount = vertices.size();
            vertexSize = sizeof(UIVertex);
            void* data = malloc(vertexCount * vertexSize);
            memcpy(data, vertices.data(), vertexCount * vertexSize);
            return data;
        }
        
        // Default: simple triangle
        struct SimpleVertex { Math::Vector3f pos; };
        SimpleVertex triangle[] = {
            {{-0.5f, -0.5f, 0.0f}},
            {{ 0.5f, -0.5f, 0.0f}},
            {{ 0.0f,  0.5f, 0.0f}}
        };
        vertexCount = 3;
        vertexSize = sizeof(SimpleVertex);
        void* data = malloc(vertexCount * vertexSize);
        memcpy(data, triangle, vertexCount * vertexSize);
        return data;
    }
};

} // namespace ShaderTest
} // namespace VoxelEditor
```

## 5. Enhanced Shader Test Runner

```cpp
// shaders/shader_test_runner.cpp
#include "validate_shader.cpp"
#include "TestMeshGenerator.h"
#include "ShaderSources.h"
#include <fstream>

class ShaderTestRunner : public ShaderValidator {
public:
    struct TestResult {
        std::string shaderName;
        bool compiled;
        bool linked;
        bool rendered;
        std::string outputPath;
        std::string errorLog;
    };
    
    TestResult testShaderWithGeometry(const ShaderSource& shader) {
        TestResult result;
        result.shaderName = shader.name;
        
        // Step 1: Compile and link shader
        GLuint program = compileAndLinkShader(shader);
        if (program == 0) {
            result.compiled = false;
            result.linked = false;
            return result;
        }
        
        result.compiled = true;
        result.linked = true;
        
        // Step 2: Generate appropriate test mesh
        size_t vertexCount, vertexSize;
        void* meshData = TestMeshGenerator::generateTestMesh(
            shader.name, vertexCount, vertexSize);
        
        // Step 3: Create VAO/VBO
        GLuint vao, vbo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * vertexSize, meshData, GL_STATIC_DRAW);
        
        // Step 4: Setup vertex attributes based on shader type
        setupVertexAttributes(shader.name);
        
        // Step 5: Setup uniforms
        glUseProgram(program);
        setupUniforms(program, shader.name);
        
        // Step 6: Render to framebuffer
        GLuint fbo, texture;
        createFramebuffer(fbo, texture, 512, 512);
        
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, 512, 512);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Choose primitive type based on shader
        GLenum primitive = GL_TRIANGLES;
        if (shader.name.find("grid") != std::string::npos) {
            primitive = GL_LINES;
        }
        
        glDrawArrays(primitive, 0, vertexCount);
        
        // Step 7: Capture output
        result.outputPath = "output/" + shader.name + "_test.ppm";
        result.rendered = captureFramebufferToPPM(result.outputPath, 512, 512);
        
        // Cleanup
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &texture);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        glDeleteProgram(program);
        free(meshData);
        
        return result;
    }
    
private:
    void setupVertexAttributes(const std::string& shaderName) {
        if (shaderName.find("voxel") != std::string::npos) {
            // BasicVertex layout
            glEnableVertexAttribArray(0); // position
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BasicVertex), (void*)0);
            
            glEnableVertexAttribArray(1); // normal
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(BasicVertex), 
                                (void*)offsetof(BasicVertex, normal));
            
            glEnableVertexAttribArray(2); // color
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(BasicVertex),
                                (void*)offsetof(BasicVertex, color));
        }
        else if (shaderName.find("grid") != std::string::npos) {
            // GridVertex layout
            glEnableVertexAttribArray(0); // position
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GridVertex), (void*)0);
            
            glEnableVertexAttribArray(1); // isMajorLine
            glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(GridVertex),
                                (void*)offsetof(GridVertex, isMajorLine));
        }
        // Add more shader-specific layouts as needed
    }
    
    void setupUniforms(GLuint program, const std::string& shaderName) {
        // Common matrices
        Math::Matrix4f model = Math::Matrix4f::identity();
        Math::Matrix4f view = Math::Matrix4f::lookAt(
            {3.0f, 3.0f, 3.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
        Math::Matrix4f projection = Math::Matrix4f::perspective(45.0f, 1.0f, 0.1f, 100.0f);
        
        // Set common uniforms
        setUniformMatrix(program, "model", model);
        setUniformMatrix(program, "view", view);
        setUniformMatrix(program, "projection", projection);
        setUniformMatrix(program, "mvpMatrix", projection * view * model);
        
        // Shader-specific uniforms
        if (shaderName.find("voxel") != std::string::npos) {
            setUniformVec3(program, "lightPos", {5.0f, 5.0f, 5.0f});
            setUniformVec3(program, "lightColor", {1.0f, 1.0f, 1.0f});
            setUniformVec3(program, "viewPos", {3.0f, 3.0f, 3.0f});
        }
        else if (shaderName.find("grid") != std::string::npos) {
            setUniformVec3(program, "minorLineColor", {0.3f, 0.3f, 0.3f});
            setUniformVec3(program, "majorLineColor", {0.5f, 0.5f, 0.5f});
            setUniformFloat(program, "opacity", 0.8f);
        }
    }
    
    void runAllVisualTests() {
        std::vector<TestResult> results;
        
        for (const auto& shader : Shaders::SHADER_SOURCES) {
            std::cout << "Visual test for " << shader.name << "... ";
            TestResult result = testShaderWithGeometry(shader);
            
            if (result.rendered) {
                std::cout << "✓ PASSED (output: " << result.outputPath << ")\n";
            } else {
                std::cout << "✗ FAILED\n";
                if (!result.errorLog.empty()) {
                    std::cout << "  Error: " << result.errorLog << "\n";
                }
            }
            
            results.push_back(result);
        }
        
        // Generate summary report
        generateHTMLReport(results);
    }
};
```

## 6. README for LLMs

```markdown
# Shader Subsystem Guide

## Quick Start
To work with shaders in this codebase:
1. All shader sources are in `shaders/ShaderSources.h` (inline fallbacks)
2. Shader files are in `shaders/sources/` organized by category
3. Run `./validate_shader` to check all shaders compile (both file and inline)
4. Run `./shader_test_runner` to visually test shaders with appropriate geometry

## Adding a New Shader
1. Add shader definition to `ShaderSources.h`:
   ```cpp
   namespace MyShader {
       constexpr const char* VERTEX_SOURCE = R"(
           #version 330 core
           // ... vertex shader code
       )";
       constexpr const char* FRAGMENT_SOURCE = R"(
           #version 330 core
           // ... fragment shader code
       )";
   }
   ```

2. Add entry to SHADER_SOURCES array:
   ```cpp
   {
       "my_shader",
       "shaders/sources/category/my_shader.vert",
       "shaders/sources/category/my_shader.frag",
       MyShader::VERTEX_SOURCE,
       MyShader::FRAGMENT_SOURCE
   }
   ```

3. Create corresponding .vert/.frag files in appropriate subfolder
4. If shader needs custom geometry, update TestMeshGenerator
5. Run validation to ensure both versions compile
6. Update renderer to use: `loadShaderWithFallback("my_shader")`

## Shader Categories and Test Meshes
- **voxel/** - Cube meshes with position, normal, color attributes
- **ground/** - Grid line meshes with position, isMajorLine attributes  
- **ui/** - Quad meshes with position, texCoord attributes
- **debug/** - Simple triangle meshes

## Common Patterns
- All shaders use #version 330 core
- Standard uniforms: model, view, projection, mvpMatrix
- Standard voxel attributes: position (vec3), normal (vec3), color (vec4)
- Grid attributes: position (vec3), isMajorLine (float)

## Validation Tools
1. **validate_shader** - Tests compilation and linking
   - Tests both file and inline versions
   - Reports which version works

2. **shader_test_runner** - Visual validation
   - Generates appropriate test mesh for each shader type
   - Renders to framebuffer and saves PPM images
   - Validates pixel output isn't all black

## Common Issues and Solutions
- **Vertex/Fragment mismatch**: Ensure `out` variables in vertex match `in` variables in fragment
- **vec3 vs vec4**: Color is often vec4 in vertex but vec3 in fragment
- **Missing uniforms**: Check that all declared uniforms are used (optimizer removes unused)
- **Attribute locations**: Ensure layout(location=X) matches mesh setup
- **Grid shaders**: Use GL_LINES primitive, not GL_TRIANGLES

## Test Mesh Reference
```cpp
// Voxel shader expects:
struct BasicVertex {
    vec3 position;   // layout(location = 0)
    vec3 normal;     // layout(location = 1)  
    vec4 color;      // layout(location = 2)
};

// Grid shader expects:
struct GridVertex {
    vec3 position;   // layout(location = 0)
    float isMajor;   // layout(location = 1)
};

// UI shader expects:
struct UIVertex {
    vec3 position;   // layout(location = 0)
    vec2 texCoord;   // layout(location = 1)
};
```
```

## 7. Static Library Architecture

### Library Structure
```
shaders/
├── CMakeLists.txt              # Builds static library
├── include/
│   └── shaders/
│       ├── ShaderLibrary.h     # Public API
│       ├── ShaderTypes.h       # Common types
│       └── ShaderVersion.h     # Version info
├── src/
│   ├── ShaderSources.cpp       # All shader sources
│   ├── ShaderLoader.cpp        # Loading implementation
│   ├── ShaderValidator.cpp     # Validation implementation
│   └── TestMeshGenerator.cpp   # Test mesh generation
├── data/                       # Shader files (optional)
│   └── sources/
│       ├── voxel/
│       ├── ground/
│       └── ui/
└── tools/                      # Standalone validation tools
    ├── validate_shader.cpp
    └── shader_test_runner.cpp
```

### CMakeLists.txt for Static Library
```cmake
cmake_minimum_required(VERSION 3.16)
project(VoxelShaders VERSION 1.0.0)

# Create static library
add_library(voxel_shaders STATIC
    src/ShaderSources.cpp
    src/ShaderLoader.cpp
    src/ShaderValidator.cpp
    src/TestMeshGenerator.cpp
)

# Set include directories
target_include_directories(voxel_shaders
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies (minimal)
find_package(OpenGL REQUIRED)
target_link_libraries(voxel_shaders
    PRIVATE
        OpenGL::GL
)

# Set properties
set_target_properties(voxel_shaders PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# Install rules
install(TARGETS voxel_shaders
    EXPORT VoxelShadersTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
)

# Optional: Build validation tools
option(BUILD_SHADER_TOOLS "Build shader validation tools" ON)
if(BUILD_SHADER_TOOLS)
    add_subdirectory(tools)
endif()

# Generate version header
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/shaders/ShaderVersion.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/shaders/ShaderVersion.h"
)
```

### Public API Header
```cpp
// include/shaders/ShaderLibrary.h
#pragma once
#include "ShaderTypes.h"
#include <string>
#include <memory>
#include <functional>

namespace VoxelEditor {
namespace Shaders {

// Forward declarations
class ShaderLibraryImpl;
class VertexArrayBuilder;

/**
 * Vertex layout definition with error checking
 */
struct VertexAttribute {
    int location;
    int componentCount;  // 1, 2, 3, or 4
    GLenum type;        // GL_FLOAT, GL_INT, etc.
    bool normalized;
    size_t offset;
    const char* name;
};

struct VertexLayout {
    std::vector<VertexAttribute> attributes;
    size_t stride;
    GLenum primitiveType;  // GL_TRIANGLES, GL_LINES, etc.
    
    // Validation helper
    std::string validate() const;
};

/**
 * VAO management with automatic error checking
 */
class ManagedVAO {
public:
    ManagedVAO(GLuint vao, GLuint vbo, GLuint ibo, const VertexLayout& layout)
        : m_vao(vao), m_vbo(vbo), m_ibo(ibo), m_layout(layout) {}
    
    ~ManagedVAO();
    
    // Safe binding with error checking
    void bind() const;
    void unbind() const;
    
    // Update vertex data with validation
    bool updateVertexData(const void* data, size_t sizeBytes);
    bool updateIndexData(const uint32_t* indices, size_t count);
    
    // Get info
    GLuint getVAO() const { return m_vao; }
    GLuint getVBO() const { return m_vbo; }
    const VertexLayout& getLayout() const { return m_layout; }
    
    // Validate state
    std::string validateState() const;
    
private:
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ibo;
    VertexLayout m_layout;
    size_t m_vertexCount = 0;
    size_t m_indexCount = 0;
};

/**
 * Static shader library interface with VAO support
 */
class ShaderLibrary {
public:
    // Get singleton instance
    static ShaderLibrary& getInstance();
    
    // Get shader source by name
    const ShaderSource* getShader(const std::string& name) const;
    
    // Get vertex layout for shader
    const VertexLayout* getVertexLayout(const std::string& shaderName) const;
    
    // Create VAO configured for specific shader
    std::unique_ptr<ManagedVAO> createVAO(const std::string& shaderName) const;
    
    // Create and populate VAO with data
    std::unique_ptr<ManagedVAO> createVAO(const std::string& shaderName,
                                          const void* vertexData,
                                          size_t vertexSizeBytes,
                                          const uint32_t* indices = nullptr,
                                          size_t indexCount = 0) const;
    
    // Validate VAO is compatible with shader
    std::string validateVAOWithShader(const ManagedVAO& vao, 
                                     GLuint shaderProgram) const;
    
    // List all available shaders
    std::vector<std::string> listShaders() const;
    
    // Get version information
    static const char* getVersion();
    static int getShaderCount();
    
private:
    ShaderLibrary();
    ~ShaderLibrary();
    std::unique_ptr<ShaderLibraryImpl> m_impl;
};

/**
 * Fluent builder for complex VAO setups
 */
class VertexArrayBuilder {
public:
    VertexArrayBuilder(const std::string& shaderName);
    
    // Configure vertex data
    VertexArrayBuilder& withVertexData(const void* data, size_t sizeBytes);
    VertexArrayBuilder& withIndexData(const uint32_t* indices, size_t count);
    
    // Override default layout
    VertexArrayBuilder& withCustomAttribute(int location, int components,
                                          GLenum type, size_t offset,
                                          bool normalized = false);
    
    // Dynamic data hints
    VertexArrayBuilder& withUsageHint(GLenum usage); // GL_STATIC_DRAW, etc.
    
    // Build with validation
    std::unique_ptr<ManagedVAO> build();
    
private:
    std::string m_shaderName;
    const void* m_vertexData = nullptr;
    size_t m_vertexSizeBytes = 0;
    const uint32_t* m_indices = nullptr;
    size_t m_indexCount = 0;
    GLenum m_usage = GL_STATIC_DRAW;
    std::vector<VertexAttribute> m_customAttributes;
};

// Helper function for shader loading with fallback
template<typename ShaderManager>
ShaderId loadShaderWithFallback(ShaderManager* manager, const std::string& name) {
    auto& lib = ShaderLibrary::getInstance();
    const ShaderSource* source = lib.getShader(name);
    
    if (!source) {
        return InvalidId;
    }
    
    // Try file first
    ShaderId shader = manager->loadShaderFromFile(
        name, source->vertexPath, source->fragmentPath);
    
    // Fall back to inline
    if (shader == InvalidId) {
        shader = manager->createShaderFromSource(
            name, source->vertexSource, source->fragmentSource);
    }
    
    return shader;
}

} // namespace Shaders
} // namespace VoxelEditor
```

### Implementation Files

#### Vertex Layouts with Shader Sources
```cpp
// src/ShaderSources.cpp
#include "shaders/ShaderLibrary.h"
#include "ShaderLibraryImpl.h"

namespace VoxelEditor {
namespace Shaders {

// Vertex layouts for each shader type
namespace Layouts {
    const VertexLayout BASIC_VOXEL = {
        {
            {0, 3, GL_FLOAT, false, offsetof(BasicVertex, position), "aPos"},
            {1, 3, GL_FLOAT, false, offsetof(BasicVertex, normal), "aNormal"},
            {2, 4, GL_FLOAT, false, offsetof(BasicVertex, color), "aColor"}
        },
        sizeof(BasicVertex),
        GL_TRIANGLES
    };
    
    const VertexLayout GROUND_PLANE = {
        {
            {0, 3, GL_FLOAT, false, offsetof(GridVertex, position), "position"},
            {1, 1, GL_FLOAT, false, offsetof(GridVertex, isMajorLine), "isMajorLine"}
        },
        sizeof(GridVertex),
        GL_LINES
    };
    
    const VertexLayout UI_OVERLAY = {
        {
            {0, 3, GL_FLOAT, false, offsetof(UIVertex, position), "position"},
            {1, 2, GL_FLOAT, false, offsetof(UIVertex, texCoord), "texCoord"}
        },
        sizeof(UIVertex),
        GL_TRIANGLES
    };
}

// Shader definitions with associated layouts
struct ShaderDefinition {
    ShaderSource source;
    VertexLayout layout;
};

namespace {
    const ShaderDefinition g_shaderDefs[] = {
        // Basic Voxel Shader
        {
            {
                "basic_voxel",
                "data/sources/voxel/basic_voxel.vert",
                "data/sources/voxel/basic_voxel.frag",
                R"(#version 330 core
                layout (location = 0) in vec3 aPos;
                layout (location = 1) in vec3 aNormal;
                layout (location = 2) in vec4 aColor;
                // ... rest of shader ...
                )",
                R"(#version 330 core
                // ... fragment shader ...
                )"
            },
            Layouts::BASIC_VOXEL
        },
        // Ground Plane Shader
        {
            {
                "ground_plane",
                "data/sources/ground/ground_plane.vert",
                "data/sources/ground/ground_plane.frag",
                R"(#version 330 core
                layout(location = 0) in vec3 position;
                layout(location = 1) in float isMajorLine;
                // ... rest of shader ...
                )",
                R"(#version 330 core
                // ... fragment shader ...
                )"
            },
            Layouts::GROUND_PLANE
        },
        // ... more shaders ...
    };
}

// ManagedVAO Implementation
void ManagedVAO::bind() const {
    // Clear any pending errors
    while (glGetError() != GL_NO_ERROR) {}
    
    glBindVertexArray(m_vao);
    
    // Verify binding succeeded
    GLint currentVAO = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
    if (currentVAO != m_vao) {
        throw std::runtime_error("Failed to bind VAO " + std::to_string(m_vao));
    }
}

void ManagedVAO::unbind() const {
    glBindVertexArray(0);
}

bool ManagedVAO::updateVertexData(const void* data, size_t sizeBytes) {
    if (!data || sizeBytes == 0) {
        return false;
    }
    
    // Validate size is multiple of stride
    if (sizeBytes % m_layout.stride != 0) {
        Logger::error("Vertex data size not multiple of stride");
        return false;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeBytes, data, GL_DYNAMIC_DRAW);
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        Logger::error("Failed to update vertex data: GL error " + std::to_string(error));
        return false;
    }
    
    m_vertexCount = sizeBytes / m_layout.stride;
    return true;
}

std::string ManagedVAO::validateState() const {
    bind();
    
    std::stringstream errors;
    
    // Check each attribute is enabled and properly configured
    for (const auto& attr : m_layout.attributes) {
        GLint enabled;
        glGetVertexAttribiv(attr.location, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
        if (!enabled) {
            errors << "Attribute " << attr.name << " at location " 
                   << attr.location << " is not enabled\n";
        }
        
        // Check attribute properties
        GLint size;
        glGetVertexAttribiv(attr.location, GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);
        if (size != attr.componentCount) {
            errors << "Attribute " << attr.name << " has wrong component count: "
                   << size << " vs expected " << attr.componentCount << "\n";
        }
    }
    
    unbind();
    return errors.str();
}

// ShaderLibrary VAO creation
std::unique_ptr<ManagedVAO> ShaderLibrary::createVAO(const std::string& shaderName) const {
    const VertexLayout* layout = getVertexLayout(shaderName);
    if (!layout) {
        throw std::runtime_error("Unknown shader: " + shaderName);
    }
    
    // Validate layout before creating VAO
    std::string layoutErrors = layout->validate();
    if (!layoutErrors.empty()) {
        throw std::runtime_error("Invalid vertex layout: " + layoutErrors);
    }
    
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    
    // Setup attributes based on layout
    for (const auto& attr : layout->attributes) {
        glEnableVertexAttribArray(attr.location);
        glVertexAttribPointer(
            attr.location,
            attr.componentCount,
            attr.type,
            attr.normalized ? GL_TRUE : GL_FALSE,
            layout->stride,
            reinterpret_cast<const void*>(attr.offset)
        );
    }
    
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // Check for errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        throw std::runtime_error("Failed to create VAO: GL error " + std::to_string(error));
    }
    
    return std::make_unique<ManagedVAO>(vao, vbo, 0, *layout);
}

std::string ShaderLibrary::validateVAOWithShader(const ManagedVAO& vao, 
                                                GLuint shaderProgram) const {
    std::stringstream errors;
    
    // Check that shader expects the attributes we're providing
    GLint maxAttribs;
    glGetProgramiv(shaderProgram, GL_ACTIVE_ATTRIBUTES, &maxAttribs);
    
    for (int i = 0; i < maxAttribs; ++i) {
        char name[256];
        GLsizei length;
        GLint size;
        GLenum type;
        glGetActiveAttrib(shaderProgram, i, sizeof(name), &length, &size, &type, name);
        
        // Find matching attribute in layout
        bool found = false;
        for (const auto& attr : vao.getLayout().attributes) {
            if (strcmp(attr.name, name) == 0) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            errors << "Shader expects attribute '" << name 
                   << "' which is not provided by VAO\n";
        }
    }
    
    return errors.str();
}

} // namespace Shaders
} // namespace VoxelEditor
```

#### Usage Example for Renderers
```cpp
// In GroundPlaneGrid.cpp - Simplified with error checking
bool GroundPlaneGrid::initialize() {
    // Load shader with fallback
    m_shader = Shaders::loadShaderWithFallback(m_shaderManager, "ground_plane");
    if (m_shader == InvalidId) {
        Logger::error("Failed to load ground plane shader");
        return false;
    }
    
    // Create VAO configured for ground plane shader
    try {
        m_managedVAO = ShaderLibrary::getInstance().createVAO("ground_plane");
        
        // Validate VAO matches shader expectations
        std::string errors = ShaderLibrary::getInstance().validateVAOWithShader(
            *m_managedVAO, m_shader);
        
        if (!errors.empty()) {
            Logger::error("VAO/Shader mismatch: " + errors);
            return false;
        }
    } catch (const std::exception& e) {
        Logger::error("Failed to create VAO: " + std::string(e.what()));
        return false;
    }
    
    return true;
}

void GroundPlaneGrid::generateGridMesh(const Vector3f& workspaceSize) {
    std::vector<GridVertex> vertices;
    // ... generate vertices ...
    
    // Update VAO with automatic validation
    if (!m_managedVAO->updateVertexData(vertices.data(), 
                                       vertices.size() * sizeof(GridVertex))) {
        Logger::error("Failed to update grid mesh");
        return;
    }
    
    m_lineCount = vertices.size() / 2;
}

void GroundPlaneGrid::render(const Matrix4f& viewMatrix, const Matrix4f& projMatrix) {
    if (!m_managedVAO || m_lineCount == 0) return;
    
    // Use shader
    m_glRenderer->useProgram(m_shader);
    
    // Bind VAO - automatically validates state
    m_managedVAO->bind();
    
    // Set uniforms...
    
    // Draw with correct primitive type (GL_LINES for grid)
    glDrawArrays(m_managedVAO->getLayout().primitiveType, 0, m_lineCount * 2);
    
    // Check for errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        // ManagedVAO can help diagnose the issue
        std::string vaoState = m_managedVAO->validateState();
        Logger::error("Draw error: " + std::to_string(error) + "\nVAO state:\n" + vaoState);
    }
    
    m_managedVAO->unbind();
}
```

### Using the Static Library

In the main application's CMakeLists.txt:
```cmake
# Find or add the shader library
add_subdirectory(shaders)

# Link to your renderer
target_link_libraries(VoxelEditor_Rendering
    PRIVATE
        voxel_shaders
)
```

In renderer code:
```cpp
#include <shaders/ShaderLibrary.h>

void SomeRenderer::loadShaders() {
    // Use the static library's helper function
    m_shader = Shaders::loadShaderWithFallback(m_shaderManager, "basic_voxel");
}
```

### Benefits of Static Library Approach

1. **Isolation**: Shader code is completely separate from main application
2. **Stability**: Once built, shader library won't change unless explicitly rebuilt
3. **Version Control**: Can version shaders independently
4. **Distribution**: Can ship as prebuilt library
5. **Testing**: Can test shaders in isolation with their own test suite
6. **Clear API**: Only exposes necessary functionality, hides implementation

### Build Options

```bash
# Build shader library only
cd shaders
mkdir build && cd build
cmake ..
make

# Install for use by other projects
make install

# Build with validation tools
cmake -DBUILD_SHADER_TOOLS=ON ..
make
./tools/validate_shader
```

## 8. Migration Plan

### Phase 1: Create Static Library (1-2 days)
1. Set up shaders/ directory with CMake structure
2. Create ShaderLibrary API and implementation
3. Move all shader sources into ShaderSources.cpp
4. Build and test static library independently

### Phase 2: Integrate with Application (1 day)
1. Add shader library to main CMakeLists.txt
2. Update ShaderManager to use ShaderLibrary API
3. Remove inline shaders from renderer classes
4. Test integration

### Phase 3: Lock Down Library (1 day)
1. Tag stable version of shader library
2. Document shader API and versioning
3. Set up separate CI for shader validation
4. Create release process for shader updates

## How This Addresses Common VAO/VBO Errors

### 1. **Attribute Location Mismatches**
```cpp
// BEFORE: Easy to get wrong
glEnableVertexAttribArray(0);  // Is this the right location?
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GridVertex), 0);
glEnableVertexAttribArray(1);  // What if shader expects location 2?

// AFTER: Automatically correct from shader definition
m_managedVAO = ShaderLibrary::getInstance().createVAO("ground_plane");
// Attributes automatically configured with correct locations
```

### 2. **Primitive Type Errors**
```cpp
// BEFORE: Common error - using wrong primitive
glDrawArrays(GL_TRIANGLES, 0, lineCount * 2);  // Oops, should be GL_LINES!

// AFTER: Primitive type is part of vertex layout
glDrawArrays(m_managedVAO->getLayout().primitiveType, 0, count);
// GL_LINES for grid, GL_TRIANGLES for voxels - always correct
```

### 3. **Stride and Offset Calculation Errors**
```cpp
// BEFORE: Manual calculations prone to errors
glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 
                     sizeof(GridVertex),  // Did I calculate this right?
                     (void*)(3 * sizeof(float)));  // Is this offset correct?

// AFTER: Offsets calculated at compile time
{1, 1, GL_FLOAT, false, offsetof(GridVertex, isMajorLine), "isMajorLine"}
// offsetof() ensures correct offset calculation
```

### 4. **VAO State Validation**
```cpp
// BEFORE: No way to know if VAO is set up correctly until draw fails
glDrawArrays(GL_LINES, 0, count);
// GL_INVALID_OPERATION - but why?

// AFTER: Can validate VAO state
std::string errors = m_managedVAO->validateState();
// "Attribute 'isMajorLine' at location 1 is not enabled"
// "Attribute 'position' has wrong component count: 4 vs expected 3"
```

### 5. **Shader/VAO Compatibility**
```cpp
// BEFORE: VAO might not match what shader expects
// Shader expects vec4 color, but VAO provides vec3

// AFTER: Validate compatibility
std::string errors = ShaderLibrary::getInstance().validateVAOWithShader(
    *m_managedVAO, m_shader);
// "Shader expects attribute 'aColor' which is not provided by VAO"
```

### 6. **Resource Cleanup**
```cpp
// BEFORE: Manual cleanup, easy to leak
if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);

// AFTER: RAII with ManagedVAO
// Destructor automatically cleans up resources
```

### 7. **Dynamic Updates with Validation**
```cpp
// BEFORE: Update buffer, hope size is right
glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GridVertex), 
             vertices.data(), GL_DYNAMIC_DRAW);

// AFTER: Validated updates
if (!m_managedVAO->updateVertexData(vertices.data(), dataSize)) {
    // "Vertex data size not multiple of stride"
    // Catches size calculation errors
}
```

## Benefits of This Approach

1. **Error Prevention**: Most VAO/VBO errors caught at setup time, not draw time
2. **Clear Diagnostics**: When errors occur, get specific error messages
3. **Type Safety**: Vertex formats defined once, used everywhere
4. **Consistency**: All shaders follow same patterns
5. **Testing**: Can validate VAO setup without rendering
6. **LLM-Friendly**: All shader and vertex layout info in one place

## What This Doesn't Do
- Doesn't prevent all OpenGL errors (still need context)
- Doesn't handle multi-threaded VAO usage
- Doesn't optimize VAO sharing between similar meshes
- Keeps dynamic mesh generation in renderers (appropriate)

This approach significantly reduces VAO/VBO errors while keeping the system simple and maintainable.