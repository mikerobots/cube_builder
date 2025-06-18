#include <gtest/gtest.h>
#include "../SurfaceTypes.h"
#include <thread>
#include <chrono>

using namespace VoxelEditor::SurfaceGen;
using namespace VoxelEditor::Math;

TEST(MeshTest, DefaultConstruction) {
    Mesh mesh;
    
    EXPECT_EQ(mesh.vertices.size(), 0);
    EXPECT_EQ(mesh.normals.size(), 0);
    EXPECT_EQ(mesh.uvCoords.size(), 0);
    EXPECT_EQ(mesh.indices.size(), 0);
    EXPECT_EQ(mesh.materialId, 0);
    EXPECT_TRUE(mesh.isValid()); // Empty mesh is valid
}

TEST(MeshTest, IsValid) {
    Mesh mesh;
    
    // Empty mesh is valid
    EXPECT_TRUE(mesh.isValid());
    
    // Add vertices only - still valid
    mesh.vertices.push_back(WorldCoordinates(Vector3f(0, 0, 0)));
    mesh.vertices.push_back(WorldCoordinates(Vector3f(1, 0, 0)));
    mesh.vertices.push_back(WorldCoordinates(Vector3f(0, 1, 0)));
    EXPECT_TRUE(mesh.isValid());
    
    // Add indices
    mesh.indices = {0, 1, 2};
    EXPECT_TRUE(mesh.isValid());
    
    // Invalid index
    mesh.indices.push_back(5); // Out of bounds
    EXPECT_FALSE(mesh.isValid());
    
    // Fix by removing invalid indices
    mesh.indices = {0, 1, 2};
    EXPECT_TRUE(mesh.isValid());
    
    // Non-multiple of 3 indices
    mesh.indices = {0, 1};
    EXPECT_FALSE(mesh.isValid());
}

TEST(MeshTest, Clear) {
    Mesh mesh;
    
    // Add data
    mesh.vertices = {WorldCoordinates(Vector3f(0, 0, 0)), WorldCoordinates(Vector3f(1, 0, 0))};
    mesh.normals = {Vector3f(0, 0, 1), Vector3f(0, 0, 1)};
    mesh.uvCoords = {Vector2f(0, 0), Vector2f(1, 0)};
    mesh.indices = {0, 1, 0};
    mesh.materialId = 5;
    
    // Clear
    mesh.clear();
    
    // Check everything is cleared
    EXPECT_EQ(mesh.vertices.size(), 0);
    EXPECT_EQ(mesh.normals.size(), 0);
    EXPECT_EQ(mesh.uvCoords.size(), 0);
    EXPECT_EQ(mesh.indices.size(), 0);
    EXPECT_EQ(mesh.materialId, 0);
    EXPECT_TRUE(mesh.isValid());
}

TEST(MeshTest, GetMemoryUsage) {
    Mesh mesh;
    
    // Empty mesh
    size_t emptySize = mesh.getMemoryUsage();
    EXPECT_EQ(emptySize, sizeof(Mesh));
    
    // Add vertices
    mesh.vertices.resize(100);
    size_t withVertices = mesh.getMemoryUsage();
    EXPECT_GT(withVertices, emptySize);
    EXPECT_GE(withVertices, sizeof(Mesh) + 100 * sizeof(Vector3f));
    
    // Add more data
    mesh.normals.resize(100);
    mesh.uvCoords.resize(100);
    mesh.indices.resize(300);
    
    size_t fullSize = mesh.getMemoryUsage();
    EXPECT_GT(fullSize, withVertices);
}

TEST(MeshTest, Transform) {
    Mesh mesh;
    
    // Create simple triangle
    mesh.vertices = {
        WorldCoordinates(Vector3f(0, 0, 0)),
        WorldCoordinates(Vector3f(1, 0, 0)),
        WorldCoordinates(Vector3f(0, 1, 0))
    };
    mesh.normals = {
        Vector3f(0, 0, 1),
        Vector3f(0, 0, 1),
        Vector3f(0, 0, 1)
    };
    mesh.indices = {0, 1, 2};
    
    // Apply translation
    Matrix4f translation = Matrix4f::translation(Vector3f(2, 3, 4));
    mesh.transform(translation);
    
    // Check vertices are translated
    EXPECT_EQ(mesh.vertices[0], WorldCoordinates(Vector3f(2, 3, 4)));
    EXPECT_EQ(mesh.vertices[1], WorldCoordinates(Vector3f(3, 3, 4)));
    EXPECT_EQ(mesh.vertices[2], WorldCoordinates(Vector3f(2, 4, 4)));
    
    // Normals should not be translated
    EXPECT_EQ(mesh.normals[0], Vector3f(0, 0, 1));
}

TEST(SurfaceSettingsTest, DefaultSettings) {
    SurfaceSettings settings = SurfaceSettings::Default();
    
    EXPECT_EQ(settings.adaptiveError, 0.01f);
    EXPECT_TRUE(settings.generateNormals);
    EXPECT_FALSE(settings.generateUVs);
    EXPECT_EQ(settings.smoothingIterations, 0);
    EXPECT_EQ(settings.simplificationRatio, 1.0f);
    EXPECT_TRUE(settings.preserveSharpFeatures);
    EXPECT_EQ(settings.sharpFeatureAngle, 30.0f);
}

TEST(SurfaceSettingsTest, PreviewSettings) {
    SurfaceSettings settings = SurfaceSettings::Preview();
    
    EXPECT_EQ(settings.adaptiveError, 0.05f);
    EXPECT_FALSE(settings.generateNormals);
    EXPECT_FALSE(settings.generateUVs);
    EXPECT_EQ(settings.smoothingIterations, 0);
    EXPECT_EQ(settings.simplificationRatio, 0.5f);
    EXPECT_FALSE(settings.preserveSharpFeatures);
}

TEST(SurfaceSettingsTest, ExportSettings) {
    SurfaceSettings settings = SurfaceSettings::Export();
    
    EXPECT_EQ(settings.adaptiveError, 0.001f);
    EXPECT_TRUE(settings.generateNormals);
    EXPECT_TRUE(settings.generateUVs);
    EXPECT_EQ(settings.smoothingIterations, 2);
    EXPECT_EQ(settings.simplificationRatio, 0.95f);
    EXPECT_TRUE(settings.preserveSharpFeatures);
    EXPECT_EQ(settings.sharpFeatureAngle, 45.0f);
}

TEST(SurfaceSettingsTest, EqualityOperator) {
    SurfaceSettings s1 = SurfaceSettings::Default();
    SurfaceSettings s2 = SurfaceSettings::Default();
    SurfaceSettings s3 = SurfaceSettings::Preview();
    
    EXPECT_TRUE(s1 == s2);
    EXPECT_FALSE(s1 == s3);
    EXPECT_TRUE(s1 != s3);
    
    // Modify one field
    s2.adaptiveError = 0.02f;
    EXPECT_FALSE(s1 == s2);
}

TEST(SurfaceSettingsTest, HashFunction) {
    SurfaceSettings s1 = SurfaceSettings::Default();
    SurfaceSettings s2 = SurfaceSettings::Default();
    SurfaceSettings s3 = SurfaceSettings::Preview();
    
    // Same settings should have same hash
    EXPECT_EQ(s1.hash(), s2.hash());
    
    // Different settings should (likely) have different hash
    EXPECT_NE(s1.hash(), s3.hash());
    
    // Modified settings should have different hash
    s2.adaptiveError = 0.02f;
    EXPECT_NE(s1.hash(), s2.hash());
}

TEST(MeshGenerationEventTest, Construction) {
    MeshGenerationEvent event(MeshGenerationEventType::Started);
    
    EXPECT_EQ(event.type, MeshGenerationEventType::Started);
    EXPECT_EQ(event.progress, 0.0f);
    EXPECT_TRUE(event.message.empty());
    EXPECT_EQ(event.lodLevel, LODLevel::LOD0);
}

TEST(SimplificationSettingsTest, Presets) {
    SimplificationSettings aggressive = SimplificationSettings::Aggressive();
    EXPECT_EQ(aggressive.targetRatio, 0.25f); // Updated value from implementation
    EXPECT_FALSE(aggressive.preserveTopology);
    EXPECT_FALSE(aggressive.preserveBoundary);
    
    SimplificationSettings balanced = SimplificationSettings::Balanced();
    EXPECT_EQ(balanced.targetRatio, 0.5f);
    EXPECT_TRUE(balanced.preserveTopology);
    EXPECT_TRUE(balanced.preserveBoundary);
    
    SimplificationSettings conservative = SimplificationSettings::Conservative();
    EXPECT_EQ(conservative.targetRatio, 0.75f);
    EXPECT_TRUE(conservative.preserveTopology);
    EXPECT_TRUE(conservative.preserveBoundary);
}

TEST(MeshCacheEntryTest, UpdateAccess) {
    MeshCacheEntry entry;
    
    auto time1 = entry.lastAccess;
    
    // Sleep a bit to ensure time difference
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    entry.updateAccess();
    auto time2 = entry.lastAccess;
    
    EXPECT_GT(time2, time1);
}

TEST(LODLevelTest, Conversion) {
    // Test LOD level conversions
    for (int i = 0; i <= 4; ++i) {
        LODLevel level = static_cast<LODLevel>(i);
        int value = static_cast<int>(level);
        EXPECT_EQ(value, i);
    }
}

TEST(ExportQualityTest, Values) {
    // Ensure export quality values are distinct
    int draft = static_cast<int>(ExportQuality::Draft);
    int standard = static_cast<int>(ExportQuality::Standard);
    int high = static_cast<int>(ExportQuality::High);
    int maximum = static_cast<int>(ExportQuality::Maximum);
    
    EXPECT_NE(draft, standard);
    EXPECT_NE(standard, high);
    EXPECT_NE(high, maximum);
}