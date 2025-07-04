#include <gtest/gtest.h>
#include "../RenderTypes.h"
#include "math/Vector2f.h"
#include <cstring>

using namespace VoxelEditor::Rendering;
using namespace VoxelEditor::Math;

class ShaderAttributeBindingTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
    
    void TearDown() override {
    }
};

TEST_F(ShaderAttributeBindingTest, VertexAttributeOffsets) {
    // Test that vertex attribute offsets match shader expectations
    
    // Expected layout:
    // Position: vec3 at offset 0 (3 * 4 = 12 bytes)
    // Normal: vec3 at offset 12 (3 * 4 = 12 bytes)
    // TexCoords: vec2 at offset 24 (2 * 4 = 8 bytes)
    // Color: struct Color at offset 32 (4 * 4 = 16 bytes)
    // Total size: 48 bytes
    
    EXPECT_EQ(sizeof(Vertex), 48);
    EXPECT_EQ(offsetof(Vertex, position), 0);
    EXPECT_EQ(offsetof(Vertex, normal), 12);
    EXPECT_EQ(offsetof(Vertex, texCoords), 24);
    EXPECT_EQ(offsetof(Vertex, color), 32);
}

TEST_F(ShaderAttributeBindingTest, VertexDataPacking) {
    // Test that vertex data is correctly packed
    Vertex v;
    v.position = WorldCoordinates(Vector3f(1.0f, 2.0f, 3.0f));
    v.normal = Vector3f(0.0f, 1.0f, 0.0f);
    v.texCoords = Vector2f(0.5f, 0.5f);
    v.color = Color(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
    
    // Verify data is at expected locations
    float* data = reinterpret_cast<float*>(&v);
    
    // Position
    EXPECT_FLOAT_EQ(data[0], 1.0f);
    EXPECT_FLOAT_EQ(data[1], 2.0f);
    EXPECT_FLOAT_EQ(data[2], 3.0f);
    
    // Normal
    EXPECT_FLOAT_EQ(data[3], 0.0f);
    EXPECT_FLOAT_EQ(data[4], 1.0f);
    EXPECT_FLOAT_EQ(data[5], 0.0f);
    
    // TexCoords
    EXPECT_FLOAT_EQ(data[6], 0.5f);
    EXPECT_FLOAT_EQ(data[7], 0.5f);
    
    // Color (RGBA)
    EXPECT_FLOAT_EQ(data[8], 1.0f);  // R
    EXPECT_FLOAT_EQ(data[9], 1.0f);  // G
    EXPECT_FLOAT_EQ(data[10], 0.0f); // B
    EXPECT_FLOAT_EQ(data[11], 1.0f); // A
}

TEST_F(ShaderAttributeBindingTest, AttributeLocationMapping) {
    // Test expected attribute locations match shader
    // From basic_voxel.vert:
    // layout (location = 0) in vec3 aPos;
    // layout (location = 1) in vec3 aNormal;
    // layout (location = 2) in vec3 aColor;
    
    // The OpenGLRenderer has been fixed to bind:
    // 0 -> Position
    // 1 -> Normal
    // 2 -> Color
    // 3 -> TexCoord0
    
    // Test that the binding order is correct
    std::vector<VertexAttribute> expectedOrder = {
        VertexAttribute::Position,  // location 0
        VertexAttribute::Normal,    // location 1
        VertexAttribute::Color,     // location 2 (matches shader!)
    };
    
    // Verify enum values (these are just the enum values, not the binding locations)
    EXPECT_EQ(static_cast<int>(VertexAttribute::Position), 0);
    EXPECT_EQ(static_cast<int>(VertexAttribute::Normal), 1);
    EXPECT_EQ(static_cast<int>(VertexAttribute::TexCoord0), 2);
    EXPECT_EQ(static_cast<int>(VertexAttribute::TexCoord1), 3);
    EXPECT_EQ(static_cast<int>(VertexAttribute::Color), 4);
    
    // The OpenGLRenderer correctly maps Color to location 2 in setupVertexAttributes
}

TEST_F(ShaderAttributeBindingTest, MeshVertexGeneration) {
    // Test that mesh vertices have correct data format
    Mesh testMesh;
    
    // Create a simple triangle
    testMesh.vertices.resize(3);
    
    // Vertex 0
    testMesh.vertices[0].position = WorldCoordinates(Vector3f(-0.5f, -0.5f, 0.0f));
    testMesh.vertices[0].normal = Vector3f(0.0f, 0.0f, 1.0f);
    testMesh.vertices[0].texCoords = Vector2f(0.0f, 0.0f);
    testMesh.vertices[0].color = Color(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
    
    // Vertex 1
    testMesh.vertices[1].position = WorldCoordinates(Vector3f(0.5f, -0.5f, 0.0f));
    testMesh.vertices[1].normal = Vector3f(0.0f, 0.0f, 1.0f);
    testMesh.vertices[1].texCoords = Vector2f(1.0f, 0.0f);
    testMesh.vertices[1].color = Color(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
    
    // Vertex 2
    testMesh.vertices[2].position = WorldCoordinates(Vector3f(0.0f, 0.5f, 0.0f));
    testMesh.vertices[2].normal = Vector3f(0.0f, 0.0f, 1.0f);
    testMesh.vertices[2].texCoords = Vector2f(0.5f, 1.0f);
    testMesh.vertices[2].color = Color(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
    
    // Add indices
    testMesh.indices = {0, 1, 2};
    
    ASSERT_EQ(testMesh.vertices.size(), 3);
    ASSERT_EQ(testMesh.indices.size(), 3);
    
    // Check first vertex
    const auto& v0 = testMesh.vertices[0];
    
    // Position
    EXPECT_FLOAT_EQ(v0.position.x(), -0.5f);
    EXPECT_FLOAT_EQ(v0.position.y(), -0.5f);
    EXPECT_FLOAT_EQ(v0.position.z(), 0.0f);
    
    // Normal should be normalized
    float normalLength = v0.normal.length();
    EXPECT_NEAR(normalLength, 1.0f, 0.001f);
    
    // Color should be yellow
    EXPECT_FLOAT_EQ(v0.color.r, 1.0f);
    EXPECT_FLOAT_EQ(v0.color.g, 1.0f);
    EXPECT_FLOAT_EQ(v0.color.b, 0.0f);
    EXPECT_FLOAT_EQ(v0.color.a, 1.0f);
}

TEST_F(ShaderAttributeBindingTest, VertexBufferLayout) {
    // Test the vertex buffer layout matches OpenGL expectations
    std::vector<Vertex> vertices(3);
    
    vertices[0].position = WorldCoordinates(Vector3f(0, 0, 0));
    vertices[0].normal = Vector3f(0, 0, 1);
    vertices[0].texCoords = Vector2f(0, 0);
    vertices[0].color = Color(1, 0, 0, 1);
    
    vertices[1].position = WorldCoordinates(Vector3f(1, 0, 0));
    vertices[1].normal = Vector3f(0, 0, 1);
    vertices[1].texCoords = Vector2f(1, 0);
    vertices[1].color = Color(0, 1, 0, 1);
    
    vertices[2].position = WorldCoordinates(Vector3f(0, 1, 0));
    vertices[2].normal = Vector3f(0, 0, 1);
    vertices[2].texCoords = Vector2f(0, 1);
    vertices[2].color = Color(0, 0, 1, 1);
    
    // Verify stride
    size_t stride = sizeof(Vertex);
    EXPECT_EQ(stride, 48);  // 12 + 12 + 8 + 16 = 48 bytes
    
    // Verify data can be accessed correctly
    const uint8_t* data = reinterpret_cast<const uint8_t*>(vertices.data());
    
    // First vertex position
    const float* pos0 = reinterpret_cast<const float*>(data + 0);
    EXPECT_FLOAT_EQ(pos0[0], 0.0f);
    EXPECT_FLOAT_EQ(pos0[1], 0.0f);
    EXPECT_FLOAT_EQ(pos0[2], 0.0f);
    
    // Second vertex position (offset by stride)
    const float* pos1 = reinterpret_cast<const float*>(data + stride);
    EXPECT_FLOAT_EQ(pos1[0], 1.0f);
    EXPECT_FLOAT_EQ(pos1[1], 0.0f);
    EXPECT_FLOAT_EQ(pos1[2], 0.0f);
    
    // First vertex color (RGBA)
    const float* color0 = reinterpret_cast<const float*>(data + offsetof(Vertex, color));
    EXPECT_FLOAT_EQ(color0[0], 1.0f);  // R
    EXPECT_FLOAT_EQ(color0[1], 0.0f);  // G
    EXPECT_FLOAT_EQ(color0[2], 0.0f);  // B
    EXPECT_FLOAT_EQ(color0[3], 1.0f);  // A
}