#include <gtest/gtest.h>
#include "../RenderTypes.h"

using namespace VoxelEditor::Rendering;
using namespace VoxelEditor::Math;

class RenderTypesTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(RenderTypesTest, ColorConstruction) {
    // Default construction
    Color defaultColor;
    EXPECT_FLOAT_EQ(defaultColor.r, 0.0f);
    EXPECT_FLOAT_EQ(defaultColor.g, 0.0f);
    EXPECT_FLOAT_EQ(defaultColor.b, 0.0f);
    EXPECT_FLOAT_EQ(defaultColor.a, 1.0f);
    
    // Component construction
    Color color(0.5f, 0.7f, 0.2f, 0.8f);
    EXPECT_FLOAT_EQ(color.r, 0.5f);
    EXPECT_FLOAT_EQ(color.g, 0.7f);
    EXPECT_FLOAT_EQ(color.b, 0.2f);
    EXPECT_FLOAT_EQ(color.a, 0.8f);
    
    // Static color functions
    Color white = Color::White();
    EXPECT_FLOAT_EQ(white.r, 1.0f);
    EXPECT_FLOAT_EQ(white.g, 1.0f);
    EXPECT_FLOAT_EQ(white.b, 1.0f);
    EXPECT_FLOAT_EQ(white.a, 1.0f);
    
    Color black = Color::Black();
    EXPECT_FLOAT_EQ(black.r, 0.0f);
    EXPECT_FLOAT_EQ(black.g, 0.0f);
    EXPECT_FLOAT_EQ(black.b, 0.0f);
    EXPECT_FLOAT_EQ(black.a, 1.0f);
    
    Color transparent = Color::Transparent();
    EXPECT_FLOAT_EQ(transparent.r, 0.0f);
    EXPECT_FLOAT_EQ(transparent.g, 0.0f);
    EXPECT_FLOAT_EQ(transparent.b, 0.0f);
    EXPECT_FLOAT_EQ(transparent.a, 0.0f);
}

TEST_F(RenderTypesTest, VertexConstruction) {
    // Default construction
    Vertex defaultVertex;
    EXPECT_EQ(defaultVertex.position.value(), Vector3f::Zero());
    EXPECT_EQ(defaultVertex.normal, Vector3f::UnitZ());
    EXPECT_EQ(defaultVertex.texCoords, Vector2f::zero());
    EXPECT_FLOAT_EQ(defaultVertex.color.r, 1.0f);
    EXPECT_FLOAT_EQ(defaultVertex.color.g, 1.0f);
    EXPECT_FLOAT_EQ(defaultVertex.color.b, 1.0f);
    EXPECT_FLOAT_EQ(defaultVertex.color.a, 1.0f);
    
    // Custom construction
    Vector3f pos(1.0f, 2.0f, 3.0f);
    Vector3f norm(0.0f, 1.0f, 0.0f);
    Vector2f tex(0.5f, 0.7f);
    Color col(0.8f, 0.6f, 0.4f, 0.9f);
    
    Vertex vertex(pos, norm, tex, col);
    EXPECT_EQ(vertex.position.value(), pos);
    EXPECT_EQ(vertex.normal, norm);
    EXPECT_EQ(vertex.texCoords, tex);
    EXPECT_FLOAT_EQ(vertex.color.r, col.r);
    EXPECT_FLOAT_EQ(vertex.color.g, col.g);
    EXPECT_FLOAT_EQ(vertex.color.b, col.b);
    EXPECT_FLOAT_EQ(vertex.color.a, col.a);
}

TEST_F(RenderTypesTest, MeshOperations) {
    Mesh mesh;
    
    // Test empty mesh
    EXPECT_TRUE(mesh.isEmpty());
    EXPECT_EQ(mesh.getVertexCount(), 0);
    EXPECT_EQ(mesh.getIndexCount(), 0);
    EXPECT_EQ(mesh.getTriangleCount(), 0);
    EXPECT_TRUE(mesh.dirty);
    
    // Add vertices
    mesh.vertices.push_back(Vertex(Vector3f(0.0f, 0.0f, 0.0f)));
    mesh.vertices.push_back(Vertex(Vector3f(1.0f, 0.0f, 0.0f)));
    mesh.vertices.push_back(Vertex(Vector3f(0.0f, 1.0f, 0.0f)));
    
    EXPECT_FALSE(mesh.isEmpty());
    EXPECT_EQ(mesh.getVertexCount(), 3);
    
    // Add indices for a triangle
    mesh.indices.push_back(0);
    mesh.indices.push_back(1);
    mesh.indices.push_back(2);
    
    EXPECT_EQ(mesh.getIndexCount(), 3);
    EXPECT_EQ(mesh.getTriangleCount(), 1);
    
    // Test clear
    mesh.clear();
    EXPECT_TRUE(mesh.isEmpty());
    EXPECT_EQ(mesh.getVertexCount(), 0);
    EXPECT_EQ(mesh.getIndexCount(), 0);
    EXPECT_TRUE(mesh.dirty);
}

TEST_F(RenderTypesTest, TransformConstruction) {
    // Default construction
    Transform defaultTransform;
    EXPECT_EQ(defaultTransform.position.value(), Vector3f::Zero());
    EXPECT_EQ(defaultTransform.rotation, Vector3f::Zero());
    EXPECT_EQ(defaultTransform.scale, Vector3f::One());
    
    // Custom construction
    Vector3f pos(1.0f, 2.0f, 3.0f);
    Vector3f rot(45.0f, 90.0f, 0.0f);
    Vector3f scl(2.0f, 1.5f, 0.5f);
    
    Transform transform(pos, rot, scl);
    EXPECT_EQ(transform.position.value(), pos);
    EXPECT_EQ(transform.rotation, rot);
    EXPECT_EQ(transform.scale, scl);
}

TEST_F(RenderTypesTest, MaterialCreation) {
    // Default material
    Material defaultMaterial;
    EXPECT_FLOAT_EQ(defaultMaterial.albedo.r, 1.0f);
    EXPECT_FLOAT_EQ(defaultMaterial.albedo.g, 1.0f);
    EXPECT_FLOAT_EQ(defaultMaterial.albedo.b, 1.0f);
    EXPECT_FLOAT_EQ(defaultMaterial.albedo.a, 1.0f);
    EXPECT_FLOAT_EQ(defaultMaterial.metallic, 0.0f);
    EXPECT_FLOAT_EQ(defaultMaterial.roughness, 0.5f);
    EXPECT_FLOAT_EQ(defaultMaterial.emission, 0.0f);
    EXPECT_FALSE(defaultMaterial.doubleSided);
    EXPECT_EQ(defaultMaterial.blendMode, BlendMode::Opaque);
    EXPECT_EQ(defaultMaterial.cullMode, CullMode::Back);
    
    // Factory methods
    Material defaultFactory = Material::createDefault();
    EXPECT_FLOAT_EQ(defaultFactory.metallic, 0.0f);
    EXPECT_FLOAT_EQ(defaultFactory.roughness, 0.5f);
    
    Color voxelColor(0.7f, 0.3f, 0.9f, 1.0f);
    Material voxelMaterial = Material::createVoxel(voxelColor);
    EXPECT_FLOAT_EQ(voxelMaterial.albedo.r, voxelColor.r);
    EXPECT_FLOAT_EQ(voxelMaterial.albedo.g, voxelColor.g);
    EXPECT_FLOAT_EQ(voxelMaterial.albedo.b, voxelColor.b);
    EXPECT_FLOAT_EQ(voxelMaterial.albedo.a, voxelColor.a);
    EXPECT_FLOAT_EQ(voxelMaterial.metallic, 0.0f);
    EXPECT_FLOAT_EQ(voxelMaterial.roughness, 0.8f);
    
    Color wireColor(1.0f, 0.0f, 0.0f, 0.5f);
    Material wireMaterial = Material::createWireframe(wireColor);
    EXPECT_FLOAT_EQ(wireMaterial.albedo.r, wireColor.r);
    EXPECT_FLOAT_EQ(wireMaterial.albedo.g, wireColor.g);
    EXPECT_FLOAT_EQ(wireMaterial.albedo.b, wireColor.b);
    EXPECT_FLOAT_EQ(wireMaterial.albedo.a, wireColor.a);
    EXPECT_EQ(wireMaterial.blendMode, BlendMode::Alpha);
    EXPECT_FLOAT_EQ(wireMaterial.roughness, 1.0f);
}

TEST_F(RenderTypesTest, EnumValues) {
    // Test that enums have expected values
    EXPECT_EQ(static_cast<int>(RenderMode::Solid), 0);
    EXPECT_EQ(static_cast<int>(RenderMode::Wireframe), 1);
    EXPECT_EQ(static_cast<int>(RenderMode::Combined), 2);
    EXPECT_EQ(static_cast<int>(RenderMode::Points), 3);
    
    EXPECT_EQ(static_cast<int>(BlendMode::Opaque), 0);
    EXPECT_EQ(static_cast<int>(BlendMode::Alpha), 1);
    EXPECT_EQ(static_cast<int>(BlendMode::Additive), 2);
    EXPECT_EQ(static_cast<int>(BlendMode::Multiply), 3);
    
    EXPECT_EQ(static_cast<int>(CullMode::None), 0);
    EXPECT_EQ(static_cast<int>(CullMode::Front), 1);
    EXPECT_EQ(static_cast<int>(CullMode::Back), 2);
    
    EXPECT_EQ(static_cast<int>(BufferUsage::Static), 0);
    EXPECT_EQ(static_cast<int>(BufferUsage::Dynamic), 1);
    EXPECT_EQ(static_cast<int>(BufferUsage::Stream), 2);
}

TEST_F(RenderTypesTest, ClearFlagsOperations) {
    // Test bitwise operations on clear flags
    ClearFlags colorOnly = ClearFlags::COLOR;
    ClearFlags depthOnly = ClearFlags::DEPTH;
    ClearFlags stencilOnly = ClearFlags::STENCIL;
    ClearFlags all = ClearFlags::All;
    
    // Test individual flags
    EXPECT_EQ(static_cast<int>(colorOnly), 1);
    EXPECT_EQ(static_cast<int>(depthOnly), 2);
    EXPECT_EQ(static_cast<int>(stencilOnly), 4);
    
    // Test combined flags
    ClearFlags colorAndDepth = static_cast<ClearFlags>(static_cast<int>(ClearFlags::COLOR) | static_cast<int>(ClearFlags::DEPTH));
    EXPECT_EQ(static_cast<int>(colorAndDepth), 3);
    
    // Test all flags
    EXPECT_EQ(static_cast<int>(all), 7);
}