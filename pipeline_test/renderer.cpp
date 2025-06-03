#include "renderer.h"
#include <iostream>
#include <cstring>

Renderer::Renderer() {
}

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::setupSimpleTriangle() {
    std::cout << "Setting up simple triangle..." << std::endl;
    
    // Create triangle vertices (same format as working test)
    m_triangleVertices = {
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}}, // Red
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}}, // Green
        {{ 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}  // Blue
    };
    
    glGenBuffers(1, &m_simpleVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_simpleVBO);
    glBufferData(GL_ARRAY_BUFFER, 
                 sizeof(SimpleVertex) * m_triangleVertices.size(),
                 m_triangleVertices.data(), 
                 GL_STATIC_DRAW);
    
    std::cout << "Triangle VBO created: " << m_simpleVBO << std::endl;
    std::cout << "Triangle vertex size: " << sizeof(SimpleVertex) << " bytes" << std::endl;
    std::cout << "Triangle vertex count: " << m_triangleVertices.size() << std::endl;
    
    return true;
}

void Renderer::renderSimpleTriangle() {
    glBindBuffer(GL_ARRAY_BUFFER, m_simpleVBO);
    
    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)offsetof(SimpleVertex, position));
    glEnableVertexAttribArray(0);
    
    // Normal attribute (location 1) 
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)offsetof(SimpleVertex, normal));
    glEnableVertexAttribArray(1);
    
    // Color attribute (location 2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)offsetof(SimpleVertex, color));
    glEnableVertexAttribArray(2);
    
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    // Cleanup
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}

bool Renderer::setupSimpleQuad() {
    std::cout << "Setting up simple quad..." << std::endl;
    
    // Create quad vertices
    m_quadVertices = {
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}}, // Bottom-left, red
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}}, // Bottom-right, green
        {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}, // Top-right, blue
        {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f}}  // Top-left, yellow
    };
    
    m_quadIndices = {
        0, 1, 2,  // First triangle
        0, 2, 3   // Second triangle
    };
    
    glGenBuffers(1, &m_quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, 
                 sizeof(SimpleVertex) * m_quadVertices.size(),
                 m_quadVertices.data(), 
                 GL_STATIC_DRAW);
    
    glGenBuffers(1, &m_quadEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(uint32_t) * m_quadIndices.size(),
                 m_quadIndices.data(),
                 GL_STATIC_DRAW);
    
    std::cout << "Quad VBO created: " << m_quadVBO << std::endl;
    std::cout << "Quad EBO created: " << m_quadEBO << std::endl;
    std::cout << "Quad indices: " << m_quadIndices.size() << std::endl;
    
    return true;
}

void Renderer::renderSimpleQuad() {
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_quadEBO);
    
    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)offsetof(SimpleVertex, position));
    glEnableVertexAttribArray(0);
    
    // Normal attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)offsetof(SimpleVertex, normal));
    glEnableVertexAttribArray(1);
    
    // Color attribute (location 2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)offsetof(SimpleVertex, color));
    glEnableVertexAttribArray(2);
    
    glDrawElements(GL_TRIANGLES, m_quadIndices.size(), GL_UNSIGNED_INT, 0);
    
    // Cleanup
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}

bool Renderer::setupComplexCube() {
    std::cout << "Setting up complex cube (voxel-like)..." << std::endl;
    
    // Create a simple cube centered at origin
    float size = 0.5f;
    
    m_cubeVertices = {
        // Front face (z = size)
        {{-size, -size,  size}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // 0 - red
        {{ size, -size,  size}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // 1 - red
        {{ size,  size,  size}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // 2 - red
        {{-size,  size,  size}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // 3 - red
        
        // Back face (z = -size)
        {{-size, -size, -size}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}}, // 4 - green
        {{ size, -size, -size}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}}, // 5 - green
        {{ size,  size, -size}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}}, // 6 - green
        {{-size,  size, -size}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}}  // 7 - green
    };
    
    m_cubeIndices = {
        // Front face
        0, 1, 2,  0, 2, 3,
        // Back face  
        4, 6, 5,  4, 7, 6,
        // Left face
        4, 0, 3,  4, 3, 7,
        // Right face
        1, 5, 6,  1, 6, 2,
        // Bottom face
        4, 5, 1,  4, 1, 0,
        // Top face
        3, 2, 6,  3, 6, 7
    };
    
    glGenBuffers(1, &m_cubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, 
                 sizeof(ComplexVertex) * m_cubeVertices.size(),
                 m_cubeVertices.data(), 
                 GL_STATIC_DRAW);
    
    glGenBuffers(1, &m_cubeEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(uint32_t) * m_cubeIndices.size(),
                 m_cubeIndices.data(),
                 GL_STATIC_DRAW);
    
    std::cout << "Cube VBO created: " << m_cubeVBO << std::endl;
    std::cout << "Cube EBO created: " << m_cubeEBO << std::endl;
    std::cout << "Complex vertex size: " << sizeof(ComplexVertex) << " bytes" << std::endl;
    std::cout << "Cube vertices: " << m_cubeVertices.size() << std::endl;
    std::cout << "Cube indices: " << m_cubeIndices.size() << std::endl;
    
    return true;
}

void Renderer::renderComplexCube() {
    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeEBO);
    
    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ComplexVertex), (void*)offsetof(ComplexVertex, position));
    glEnableVertexAttribArray(0);
    
    // Normal attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ComplexVertex), (void*)offsetof(ComplexVertex, normal));
    glEnableVertexAttribArray(1);
    
    // Color attribute (location 2) - READ ONLY RGB (3 components) from RGBA data
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ComplexVertex), (void*)offsetof(ComplexVertex, color));
    glEnableVertexAttribArray(2);
    
    glDrawElements(GL_TRIANGLES, m_cubeIndices.size(), GL_UNSIGNED_INT, 0);
    
    // Cleanup
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}

bool Renderer::setupMainAppVoxel() {
    std::cout << "Setting up main app voxel (exact replica)..." << std::endl;
    
    // Main app voxel specs:
    // - World position: (0.64, 0.64, 0.64)  
    // - Voxel size: 1.28m (128cm resolution)
    // - Vertex range: 0.032 to 1.248
    // - Color: bright red (1.0, 0.0, 0.0, 1.0)
    
    float voxelSize = 1.28f;
    float halfSize = voxelSize * 0.5f;  // 0.64
    float offset = 0.64f;  // World position offset
    
    // Create vertices exactly like main app VoxelMeshGenerator
    m_voxelVertices = {
        // Front face (z = offset + halfSize = 1.28)
        {{offset - halfSize, offset - halfSize, offset + halfSize}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{offset + halfSize, offset - halfSize, offset + halfSize}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{offset + halfSize, offset + halfSize, offset + halfSize}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{offset - halfSize, offset + halfSize, offset + halfSize}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        
        // Back face (z = offset - halfSize = 0.0)
        {{offset - halfSize, offset - halfSize, offset - halfSize}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{offset + halfSize, offset - halfSize, offset - halfSize}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{offset + halfSize, offset + halfSize, offset - halfSize}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{offset - halfSize, offset + halfSize, offset - halfSize}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}
    };
    
    // Same indices as main app (standard cube)
    m_voxelIndices = {
        // Front face
        0, 1, 2,  0, 2, 3,
        // Back face  
        4, 6, 5,  4, 7, 6,
        // Left face
        4, 0, 3,  4, 3, 7,
        // Right face
        1, 5, 6,  1, 6, 2,
        // Bottom face
        4, 5, 1,  4, 1, 0,
        // Top face
        3, 2, 6,  3, 6, 7
    };
    
    glGenBuffers(1, &m_voxelVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_voxelVBO);
    glBufferData(GL_ARRAY_BUFFER, 
                 sizeof(ComplexVertex) * m_voxelVertices.size(),
                 m_voxelVertices.data(), 
                 GL_STATIC_DRAW);
    
    glGenBuffers(1, &m_voxelEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_voxelEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(uint32_t) * m_voxelIndices.size(),
                 m_voxelIndices.data(),
                 GL_STATIC_DRAW);
    
    std::cout << "Main app voxel VBO: " << m_voxelVBO << std::endl;
    std::cout << "Main app voxel EBO: " << m_voxelEBO << std::endl;
    std::cout << "Voxel world position: (" << offset << ", " << offset << ", " << offset << ")" << std::endl;
    std::cout << "Voxel size: " << voxelSize << "m" << std::endl;
    std::cout << "Vertex range: " << (offset - halfSize) << " to " << (offset + halfSize) << std::endl;
    std::cout << "Voxel vertices: " << m_voxelVertices.size() << std::endl;
    std::cout << "Voxel indices: " << m_voxelIndices.size() << std::endl;
    
    return true;
}

void Renderer::renderMainAppVoxel() {
    glBindBuffer(GL_ARRAY_BUFFER, m_voxelVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_voxelEBO);
    
    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ComplexVertex), (void*)offsetof(ComplexVertex, position));
    glEnableVertexAttribArray(0);
    
    // Normal attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ComplexVertex), (void*)offsetof(ComplexVertex, normal));
    glEnableVertexAttribArray(1);
    
    // Color attribute (location 2) - READ ONLY RGB (3 components) from RGBA data
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ComplexVertex), (void*)offsetof(ComplexVertex, color));
    glEnableVertexAttribArray(2);
    
    glDrawElements(GL_TRIANGLES, m_voxelIndices.size(), GL_UNSIGNED_INT, 0);
    
    // Cleanup
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}

bool Renderer::setupCenterVoxel() {
    std::cout << "Setting up center voxel (at origin)..." << std::endl;
    
    // Simple cube centered at origin
    float size = 0.5f;
    
    m_centerVertices = {
        // Front face (z = size)
        {{-size, -size,  size}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // red
        {{ size, -size,  size}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // red
        {{ size,  size,  size}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // red
        {{-size,  size,  size}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // red
        
        // Back face (z = -size)
        {{-size, -size, -size}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // red
        {{ size, -size, -size}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // red
        {{ size,  size, -size}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // red
        {{-size,  size, -size}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}  // red
    };
    
    m_centerIndices = {
        // Front face
        0, 1, 2,  0, 2, 3,
        // Back face  
        4, 6, 5,  4, 7, 6,
        // Left face
        4, 0, 3,  4, 3, 7,
        // Right face
        1, 5, 6,  1, 6, 2,
        // Bottom face
        4, 5, 1,  4, 1, 0,
        // Top face
        3, 2, 6,  3, 6, 7
    };
    
    glGenBuffers(1, &m_centerVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_centerVBO);
    glBufferData(GL_ARRAY_BUFFER, 
                 sizeof(ComplexVertex) * m_centerVertices.size(),
                 m_centerVertices.data(), 
                 GL_STATIC_DRAW);
    
    glGenBuffers(1, &m_centerEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_centerEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(uint32_t) * m_centerIndices.size(),
                 m_centerIndices.data(),
                 GL_STATIC_DRAW);
    
    std::cout << "Center voxel VBO: " << m_centerVBO << std::endl;
    std::cout << "Center voxel EBO: " << m_centerEBO << std::endl;
    std::cout << "Center voxel size: " << (size * 2) << std::endl;
    std::cout << "Center vertices: " << m_centerVertices.size() << std::endl;
    std::cout << "Center indices: " << m_centerIndices.size() << std::endl;
    
    return true;
}

void Renderer::renderCenterVoxel() {
    glBindBuffer(GL_ARRAY_BUFFER, m_centerVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_centerEBO);
    
    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ComplexVertex), (void*)offsetof(ComplexVertex, position));
    glEnableVertexAttribArray(0);
    
    // Normal attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ComplexVertex), (void*)offsetof(ComplexVertex, normal));
    glEnableVertexAttribArray(1);
    
    // Color attribute (location 2) - READ ONLY RGB (3 components) from RGBA data
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ComplexVertex), (void*)offsetof(ComplexVertex, color));
    glEnableVertexAttribArray(2);
    
    glDrawElements(GL_TRIANGLES, m_centerIndices.size(), GL_UNSIGNED_INT, 0);
    
    // Cleanup
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}

void Renderer::cleanup() {
    if (m_simpleVBO) { glDeleteBuffers(1, &m_simpleVBO); m_simpleVBO = 0; }
    if (m_quadVBO) { glDeleteBuffers(1, &m_quadVBO); m_quadVBO = 0; }
    if (m_quadEBO) { glDeleteBuffers(1, &m_quadEBO); m_quadEBO = 0; }
    if (m_cubeVBO) { glDeleteBuffers(1, &m_cubeVBO); m_cubeVBO = 0; }
    if (m_cubeEBO) { glDeleteBuffers(1, &m_cubeEBO); m_cubeEBO = 0; }
    if (m_voxelVBO) { glDeleteBuffers(1, &m_voxelVBO); m_voxelVBO = 0; }
    if (m_voxelEBO) { glDeleteBuffers(1, &m_voxelEBO); m_voxelEBO = 0; }
    if (m_centerVBO) { glDeleteBuffers(1, &m_centerVBO); m_centerVBO = 0; }
    if (m_centerEBO) { glDeleteBuffers(1, &m_centerEBO); m_centerEBO = 0; }
}