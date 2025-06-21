#include <gtest/gtest.h>
extern "C" {
#include <glad/glad.h>
}
#include <GLFW/glfw3.h>
#include <string>
#include <memory>
#include <vector>

// Test framework for validating inline shaders
class InlineShaderTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    
    void SetUp() override {
        if (!glfwInit()) {
            FAIL() << "Failed to initialize GLFW";
        }
        
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        
        window = glfwCreateWindow(1, 1, "Test", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            FAIL() << "Failed to create GLFW window";
        }
        
        glfwMakeContextCurrent(window);
        
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            FAIL() << "Failed to initialize GLAD";
        }
    }
    
    void TearDown() override {
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
    
    bool compileShader(GLuint shader, const char* source) {
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            ADD_FAILURE() << "Shader compilation failed: " << infoLog;
        }
        
        return success == GL_TRUE;
    }
    
    bool linkProgram(GLuint program) {
        glLinkProgram(program);
        
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            ADD_FAILURE() << "Program linking failed: " << infoLog;
        }
        
        return success == GL_TRUE;
    }
    
    struct ShaderValidationResult {
        bool compilationSuccess;
        bool linkingSuccess;
        bool hasRequiredUniforms;
        bool hasRequiredAttributes;
        std::string errorLog;
    };
    
    ShaderValidationResult validateShaderPair(
        const char* vertexSource,
        const char* fragmentSource,
        const std::vector<std::string>& requiredUniforms = {},
        const std::vector<std::string>& requiredAttributes = {}
    ) {
        ShaderValidationResult result{true, true, true, true, ""};
        
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        GLuint program = glCreateProgram();
        
        // Compile shaders
        result.compilationSuccess = compileShader(vertexShader, vertexSource) &&
                                   compileShader(fragmentShader, fragmentSource);
        
        if (result.compilationSuccess) {
            glAttachShader(program, vertexShader);
            glAttachShader(program, fragmentShader);
            result.linkingSuccess = linkProgram(program);
            
            if (result.linkingSuccess) {
                // Check uniforms
                for (const auto& uniform : requiredUniforms) {
                    GLint location = glGetUniformLocation(program, uniform.c_str());
                    if (location == -1) {
                        result.hasRequiredUniforms = false;
                        result.errorLog += "Missing uniform: " + uniform + "\n";
                    }
                }
                
                // Check attributes
                for (const auto& attribute : requiredAttributes) {
                    GLint location = glGetAttribLocation(program, attribute.c_str());
                    if (location == -1) {
                        result.hasRequiredAttributes = false;
                        result.errorLog += "Missing attribute: " + attribute + "\n";
                    }
                }
            }
        }
        
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);
        
        return result;
    }
};

// Test OutlineRenderer inline shaders
TEST_F(InlineShaderTest, OutlineRendererShaders) {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec4 color;
        layout(location = 2) in float patternCoord;
        
        uniform mat4 mvpMatrix;
        
        out vec4 fragColor;
        out float fragPatternCoord;
        
        void main() {
            gl_Position = mvpMatrix * vec4(position, 1.0);
            fragColor = color;
            fragPatternCoord = patternCoord;
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec4 fragColor;
        in float fragPatternCoord;
        
        uniform float patternScale;
        uniform float patternOffset;
        uniform int linePattern; // 0=solid, 1=dashed, 2=dotted, 3=dashdot
        uniform float animationTime;
        
        out vec4 color;
        
        void main() {
            // Calculate pattern value based on pattern type
            float coord = (fragPatternCoord + patternOffset) * patternScale;
            float alpha = 1.0;
            
            if (linePattern == 1) { // Dashed
                alpha = step(0.5, fract(coord));
            } else if (linePattern == 2) { // Dotted
                alpha = step(0.7, fract(coord * 3.0));
            } else if (linePattern == 3) { // DashDot
                float phase = fract(coord * 0.5);
                alpha = (phase < 0.4) ? 1.0 : (phase < 0.5 || phase > 0.8) ? 0.0 : 1.0;
            }
            
            color = vec4(fragColor.rgb, fragColor.a * alpha);
        }
    )";
    
    std::vector<std::string> requiredUniforms = {
        "mvpMatrix", "patternScale", "patternOffset", "linePattern"
    };
    
    std::vector<std::string> requiredAttributes = {
        "position", "color", "patternCoord"
    };
    
    auto result = validateShaderPair(vertexShaderSource, fragmentShaderSource,
                                    requiredUniforms, requiredAttributes);
    
    EXPECT_TRUE(result.compilationSuccess) << "Shader compilation failed";
    EXPECT_TRUE(result.linkingSuccess) << "Shader linking failed";
    EXPECT_TRUE(result.hasRequiredUniforms) << result.errorLog;
    EXPECT_TRUE(result.hasRequiredAttributes) << result.errorLog;
}

// Test OverlayRenderer inline shaders
TEST_F(InlineShaderTest, OverlayRendererShaders) {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec2 position;
        layout(location = 1) in vec2 texCoord;
        layout(location = 2) in vec4 color;
        
        uniform vec2 screenSize;
        
        out vec2 fragTexCoord;
        out vec4 fragColor;
        
        void main() {
            // Convert from screen coordinates to NDC
            vec2 ndc = (position / screenSize) * 2.0 - 1.0;
            ndc.y = -ndc.y; // Flip Y axis
            
            gl_Position = vec4(ndc, 0.0, 1.0);
            fragTexCoord = texCoord;
            fragColor = color;
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 fragTexCoord;
        in vec4 fragColor;
        
        uniform sampler2D fontTexture;
        
        out vec4 color;
        
        void main() {
            float alpha = texture(fontTexture, fragTexCoord).a;
            color = vec4(fragColor.rgb, fragColor.a * alpha);
        }
    )";
    
    std::vector<std::string> requiredUniforms = {
        "screenSize", "fontTexture"
    };
    
    std::vector<std::string> requiredAttributes = {
        "position", "texCoord", "color"
    };
    
    auto result = validateShaderPair(vertexShaderSource, fragmentShaderSource,
                                    requiredUniforms, requiredAttributes);
    
    EXPECT_TRUE(result.compilationSuccess) << "Shader compilation failed";
    EXPECT_TRUE(result.linkingSuccess) << "Shader linking failed";
    EXPECT_TRUE(result.hasRequiredUniforms) << result.errorLog;
    EXPECT_TRUE(result.hasRequiredAttributes) << result.errorLog;
}

// Test GroundPlaneGrid inline shaders
TEST_F(InlineShaderTest, GroundPlaneGridShaders) {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in float isMajor;
        
        uniform mat4 mvpMatrix;
        
        out float fragIsMajor;
        
        void main() {
            gl_Position = mvpMatrix * vec4(position, 1.0);
            fragIsMajor = isMajor;
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in float fragIsMajor;
        
        uniform vec3 gridColor;
        uniform vec3 majorGridColor;
        uniform float gridOpacity;
        uniform float fadeStart;
        uniform float fadeEnd;
        
        out vec4 color;
        
        void main() {
            // Use major or minor grid color
            vec3 lineColor = fragIsMajor > 0.5 ? majorGridColor : gridColor;
            
            // Calculate distance fade
            float distance = length(gl_FragCoord.xy);
            float fadeFactor = smoothstep(fadeStart, fadeEnd, distance);
            
            // Combine opacity with fade
            float alpha = gridOpacity * (1.0 - fadeFactor);
            
            color = vec4(lineColor, alpha);
        }
    )";
    
    std::vector<std::string> requiredUniforms = {
        "mvpMatrix", "gridColor", "majorGridColor", "gridOpacity", "fadeStart", "fadeEnd"
    };
    
    std::vector<std::string> requiredAttributes = {
        "position", "isMajor"
    };
    
    auto result = validateShaderPair(vertexShaderSource, fragmentShaderSource,
                                    requiredUniforms, requiredAttributes);
    
    EXPECT_TRUE(result.compilationSuccess) << "Shader compilation failed";
    EXPECT_TRUE(result.linkingSuccess) << "Shader linking failed";
    EXPECT_TRUE(result.hasRequiredUniforms) << result.errorLog;
    EXPECT_TRUE(result.hasRequiredAttributes) << result.errorLog;
}

// Test HighlightRenderer inline shaders
TEST_F(InlineShaderTest, HighlightRendererShaders) {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec3 normal;
        
        uniform mat4 mvpMatrix;
        uniform mat4 modelMatrix;
        uniform mat4 viewMatrix;
        uniform mat3 normalMatrix;
        
        out vec3 fragNormal;
        out vec3 fragViewDir;
        
        void main() {
            gl_Position = mvpMatrix * vec4(position, 1.0);
            fragNormal = normalize(normalMatrix * normal);
            vec4 worldPos = modelMatrix * vec4(position, 1.0);
            vec4 viewPos = viewMatrix * worldPos;
            fragViewDir = normalize(-viewPos.xyz);
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 fragNormal;
        in vec3 fragViewDir;
        
        uniform vec4 highlightColor;
        uniform float pulseTime;
        uniform float edgeThreshold;
        
        out vec4 color;
        
        void main() {
            // Fresnel effect for edge highlighting
            float fresnel = 1.0 - abs(dot(fragNormal, fragViewDir));
            fresnel = pow(fresnel, edgeThreshold);
            
            // Animated pulse
            float pulse = sin(pulseTime * 3.14159) * 0.5 + 0.5;
            
            // Combine effects
            float alpha = highlightColor.a * fresnel * (0.5 + pulse * 0.5);
            color = vec4(highlightColor.rgb, alpha);
        }
    )";
    
    std::vector<std::string> requiredUniforms = {
        "mvpMatrix", "modelMatrix", "viewMatrix", "normalMatrix",
        "highlightColor", "pulseTime", "edgeThreshold"
    };
    
    std::vector<std::string> requiredAttributes = {
        "position", "normal"
    };
    
    auto result = validateShaderPair(vertexShaderSource, fragmentShaderSource,
                                    requiredUniforms, requiredAttributes);
    
    EXPECT_TRUE(result.compilationSuccess) << "Shader compilation failed";
    EXPECT_TRUE(result.linkingSuccess) << "Shader linking failed";
    EXPECT_TRUE(result.hasRequiredUniforms) << result.errorLog;
    EXPECT_TRUE(result.hasRequiredAttributes) << result.errorLog;
}