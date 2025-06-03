#pragma once

extern "C" {
    #include <glad/glad.h>
}
#include <vector>

struct SimpleVertex {
    float position[3];
    float normal[3];
    float color[3];
};

struct ComplexVertex {
    float position[3];
    float normal[3];
    float texCoords[2];
    float color[4];  // RGBA like main app
};

class Renderer {
public:
    Renderer();
    ~Renderer();
    
    // Test 1: Simple triangle (proven working case)
    bool setupSimpleTriangle();
    void renderSimpleTriangle();
    
    // Test 2: Indexed quad with simple vertices
    bool setupSimpleQuad();
    void renderSimpleQuad();
    
    // Test 3: Indexed cube with complex vertices (like voxel)
    bool setupComplexCube();
    void renderComplexCube();
    
    // Test 4: Voxel at exact main app position and scale
    bool setupMainAppVoxel();
    void renderMainAppVoxel();
    
    // Test 6: Voxel at origin for simple camera
    bool setupCenterVoxel();
    void renderCenterVoxel();
    
    void cleanup();
    
private:
    // Simple triangle data
    GLuint m_simpleVBO = 0;
    
    // Simple quad data
    GLuint m_quadVBO = 0;
    GLuint m_quadEBO = 0;
    
    // Complex cube data
    GLuint m_cubeVBO = 0;
    GLuint m_cubeEBO = 0;
    
    // Main app voxel data
    GLuint m_voxelVBO = 0;
    GLuint m_voxelEBO = 0;
    
    // Center voxel data
    GLuint m_centerVBO = 0;
    GLuint m_centerEBO = 0;
    
    std::vector<SimpleVertex> m_triangleVertices;
    std::vector<SimpleVertex> m_quadVertices;
    std::vector<uint32_t> m_quadIndices;
    
    std::vector<ComplexVertex> m_cubeVertices;
    std::vector<uint32_t> m_cubeIndices;
    
    std::vector<ComplexVertex> m_voxelVertices;
    std::vector<uint32_t> m_voxelIndices;
    
    std::vector<ComplexVertex> m_centerVertices;
    std::vector<uint32_t> m_centerIndices;
};