// Test rendering with our actual shader
#include <iostream>
extern "C" {
#include <glad/glad.h>
}
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstring>

const char* vertexShaderSource = R"(
#version 120
attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_texCoord;
attribute vec4 a_color;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat4 u_normalMatrix;

varying vec3 v_worldPos;
varying vec3 v_normal;
varying vec2 v_texCoord;
varying vec4 v_color;

void main() {
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    v_worldPos = worldPos.xyz;
    v_normal = mat3(u_normalMatrix) * a_normal;
    v_texCoord = a_texCoord;
    v_color = a_color;
    
    gl_Position = u_projection * u_view * worldPos;
}
)";

const char* fragmentShaderSource = R"(
#version 120
varying vec3 v_worldPos;
varying vec3 v_normal;
varying vec2 v_texCoord;
varying vec4 v_color;

uniform vec4 u_albedo;
uniform float u_metallic;
uniform float u_roughness;
uniform float u_emission;

uniform vec4 u_ambientLight;
uniform vec3 u_lightDirection;
uniform vec4 u_lightColor;
uniform int u_enableLighting;

void main() {
    vec4 albedo = u_albedo * v_color;
    
    if (u_enableLighting > 0) {
        vec3 normal = normalize(v_normal);
        float NdotL = max(dot(normal, -u_lightDirection), 0.0);
        
        vec3 diffuse = albedo.rgb * u_lightColor.rgb * NdotL;
        vec3 ambient = albedo.rgb * u_ambientLight.rgb;
        vec3 emission = albedo.rgb * u_emission;
        
        gl_FragColor = vec4(ambient + diffuse + emission, albedo.a);
    } else {
        gl_FragColor = albedo;
    }
}
)";

// Identity matrix
float identity[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
};

// Simple perspective matrix
void setPerspective(float* m, float fov, float aspect, float near, float far) {
    float f = 1.0f / tanf(fov * 0.5f * 3.14159f / 180.0f);
    memset(m, 0, sizeof(float) * 16);
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (far + near) / (near - far);
    m[11] = -1.0f;
    m[14] = (2.0f * far * near) / (near - far);
}

// Simple view matrix (look at origin from z=5)
void setLookAt(float* m, float eyeX, float eyeY, float eyeZ) {
    memset(m, 0, sizeof(float) * 16);
    m[0] = 1;
    m[5] = 1;
    m[10] = 1;
    m[15] = 1;
    m[12] = -eyeX;
    m[13] = -eyeY;
    m[14] = -eyeZ;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Simple Cube Test", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    
    // Create shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    GLint success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex shader error: " << infoLog << std::endl;
    }
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment shader error: " << infoLog << std::endl;
    }
    
    // Link shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    
    // Bind attribute locations BEFORE linking
    glBindAttribLocation(shaderProgram, 0, "a_position");
    glBindAttribLocation(shaderProgram, 1, "a_normal");
    glBindAttribLocation(shaderProgram, 2, "a_texCoord");
    glBindAttribLocation(shaderProgram, 3, "a_color");
    
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader link error: " << infoLog << std::endl;
    }
    
    // Create a simple cube (just front face for now)
    struct Vertex {
        float pos[3];
        float normal[3];
        float texCoord[2];
        float color[4];
    };
    
    Vertex vertices[] = {
        // Front face
        {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {0, 0}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1, 0}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1, 1}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {0, 1}, {1.0f, 0.0f, 0.0f, 1.0f}},
    };
    
    unsigned int indices[] = {
        0, 1, 2,  2, 3, 0
    };
    
    GLuint VBO, EBO;
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Setup matrices
    float projection[16], view[16];
    setPerspective(projection, 45.0f, 800.0f/600.0f, 0.1f, 100.0f);
    setLookAt(view, 0, 0, 5);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Render loop with timeout
    double startTime = glfwGetTime();
    double timeout = 3.0;
    int frameCount = 0;
    
    while (!glfwWindowShouldClose(window) && glfwGetTime() - startTime < timeout) {
        frameCount++;
        
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(shaderProgram);
        
        // Set uniforms
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_model"), 1, GL_FALSE, identity);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_view"), 1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_projection"), 1, GL_FALSE, projection);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "u_normalMatrix"), 1, GL_FALSE, identity);
        
        // Material uniforms
        float albedo[4] = {0.8f, 0.8f, 0.8f, 1.0f};
        glUniform4fv(glGetUniformLocation(shaderProgram, "u_albedo"), 1, albedo);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_metallic"), 0.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_roughness"), 0.5f);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_emission"), 0.0f);
        
        // Lighting uniforms - disable lighting for now
        glUniform1i(glGetUniformLocation(shaderProgram, "u_enableLighting"), 0);
        
        // Bind buffers and set attributes
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
        
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
        
        // Draw
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        // Check for errors
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cout << "OpenGL error: " << err << std::endl;
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    std::cout << "\nTest completed!" << std::endl;
    std::cout << "Rendered " << frameCount << " frames" << std::endl;
    
    // Check uniform locations
    std::cout << "\nUniform locations:" << std::endl;
    std::cout << "u_model: " << glGetUniformLocation(shaderProgram, "u_model") << std::endl;
    std::cout << "u_view: " << glGetUniformLocation(shaderProgram, "u_view") << std::endl;
    std::cout << "u_projection: " << glGetUniformLocation(shaderProgram, "u_projection") << std::endl;
    std::cout << "u_albedo: " << glGetUniformLocation(shaderProgram, "u_albedo") << std::endl;
    
    // Cleanup
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}