#pragma once

extern "C" {
    #include <glad/glad.h>
}
#include <string>

class Shader {
public:
    Shader();
    ~Shader();
    
    bool createProgram(const std::string& vertexSource, const std::string& fragmentSource);
    void destroy();
    
    void use();
    void unuse();
    
    GLint getUniformLocation(const std::string& name);
    GLint getAttributeLocation(const std::string& name);
    
    GLuint getProgram() const { return m_program; }
    
private:
    GLuint m_program = 0;
    
    GLuint compileShader(GLenum type, const std::string& source);
};