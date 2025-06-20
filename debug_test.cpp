#include <iostream>
#include <vector>
#include <cstring>

// Simple test to analyze the GL_INVALID_OPERATION error

int main() {
    std::cout << "GL_INVALID_OPERATION Debug Analysis" << std::endl;
    std::cout << "===================================" << std::endl;
    
    std::cout << "\nBased on the code analysis, here are the key findings:\n" << std::endl;
    
    std::cout << "1. SHADER ATTRIBUTES:" << std::endl;
    std::cout << "   - basic_voxel_gl33.vert expects:" << std::endl;
    std::cout << "     * layout(location = 0) in vec3 a_position;" << std::endl;
    std::cout << "     * layout(location = 1) in vec3 a_normal;" << std::endl;
    std::cout << "     * layout(location = 2) in vec4 a_color;  // Note: vec4!" << std::endl;
    std::cout << std::endl;
    
    std::cout << "2. VERTEX SETUP IN OpenGLRenderer::setupVertexAttributes():" << std::endl;
    std::cout << "   - Location 0: Position (3 floats)" << std::endl;
    std::cout << "   - Location 1: Normal (3 floats)" << std::endl;
    std::cout << "   - Location 2: Color (4 floats) ✓ Matches shader" << std::endl;
    std::cout << "   - Location 3: TexCoord0 (2 floats)" << std::endl;
    std::cout << std::endl;
    
    std::cout << "3. POTENTIAL ISSUES FOUND:" << std::endl;
    std::cout << "   a) The shader expects 'a_color' to be vec4, vertex setup provides 4 floats ✓ OK" << std::endl;
    std::cout << "   b) RenderEngine::setupMeshBuffers() enables ALL 4 attributes:" << std::endl;
    std::cout << "      - Position, Normal, Color, TexCoord0" << std::endl;
    std::cout << "      - But shader only uses first 3!" << std::endl;
    std::cout << "   c) TexCoord0 attribute is enabled but not used in shader" << std::endl;
    std::cout << std::endl;
    
    std::cout << "4. GL_INVALID_OPERATION CAUSES:" << std::endl;
    std::cout << "   The error occurs when:" << std::endl;
    std::cout << "   - An enabled vertex attribute has no corresponding shader input" << std::endl;
    std::cout << "   - Shader validation fails due to attribute mismatch" << std::endl;
    std::cout << "   - The shader program is not properly linked" << std::endl;
    std::cout << std::endl;
    
    std::cout << "5. SOLUTION:" << std::endl;
    std::cout << "   RenderEngine::setupMeshBuffers() should only enable attributes" << std::endl;
    std::cout << "   that the current shader actually uses. The TexCoord0 attribute" << std::endl;
    std::cout << "   at location 3 is being enabled but the shader has no input at" << std::endl;
    std::cout << "   that location." << std::endl;
    std::cout << std::endl;
    
    std::cout << "6. FIX LOCATION:" << std::endl;
    std::cout << "   File: core/rendering/RenderEngine.cpp" << std::endl;
    std::cout << "   Function: setupMeshBuffers() around line 547" << std::endl;
    std::cout << "   Change: Only enable Position, Normal, and Color attributes" << std::endl;
    std::cout << "         (remove TexCoord0 from the attributes list)" << std::endl;
    
    return 0;
}