#include "shader.h"
#include <iostream>
#include <vector>

Shader::Shader() {
}

Shader::~Shader() {
    destroy();
}

GLuint Shader::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* sourceCStr = source.c_str();
    glShaderSource(shader, 1, &sourceCStr, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength);
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        
        std::cerr << "Shader compilation failed (" 
                  << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") 
                  << "): " << log.data() << std::endl;
        
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

bool Shader::createProgram(const std::string& vertexSource, const std::string& fragmentSource) {
    // Compile shaders
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (!vertexShader) return false;
    
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!fragmentShader) {
        glDeleteShader(vertexShader);
        return false;
    }
    
    // Create program
    m_program = glCreateProgram();
    glAttachShader(m_program, vertexShader);
    glAttachShader(m_program, fragmentShader);
    glLinkProgram(m_program);
    
    // Check linking
    GLint success;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    
    if (!success) {
        GLint logLength;
        glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength);
        glGetProgramInfoLog(m_program, logLength, nullptr, log.data());
        
        std::cerr << "Shader linking failed: " << log.data() << std::endl;
        
        glDeleteProgram(m_program);
        m_program = 0;
    }
    
    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    if (success) {
        std::cout << "Shader program created successfully (ID: " << m_program << ")" << std::endl;
    }
    
    return success;
}

void Shader::destroy() {
    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

void Shader::use() {
    glUseProgram(m_program);
}

void Shader::unuse() {
    glUseProgram(0);
}

GLint Shader::getUniformLocation(const std::string& name) {
    return glGetUniformLocation(m_program, name.c_str());
}

GLint Shader::getAttributeLocation(const std::string& name) {
    return glGetAttribLocation(m_program, name.c_str());
}