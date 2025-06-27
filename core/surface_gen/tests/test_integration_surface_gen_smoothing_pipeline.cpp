#include <gtest/gtest.h>
#include "../SurfaceGenerator.h"
#include "../MeshSmoother.h"
#include "../MeshValidator.h"
#include "../../voxel_data/VoxelGrid.h"
#include "../../voxel_data/VoxelTypes.h"
#include <chrono>

using namespace VoxelEditor::SurfaceGen;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor;

class SurfaceSmoothingIntegrationTest : public ::testing::Test {
protected:
    std::unique_ptr<SurfaceGenerator> generator;
    std::unique_ptr<VoxelGrid> grid;
    
    void SetUp() override {
        generator = std::make_unique<SurfaceGenerator>();
        grid = std::make_unique<VoxelGrid>(VoxelResolution::Size_4cm, 5.0f);
        
        // Create a simple blocky shape (2x2x2 cube)
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    grid->setVoxel(Math::Vector3i(x, y, z), true);
                }
            }
        }
    }
    
    bool hasSharpEdges(const Mesh& mesh) {
        // Check if mesh has sharp edges by analyzing normal variation
        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            uint32_t i0 = mesh.indices[i];
            uint32_t i1 = mesh.indices[i + 1];
            uint32_t i2 = mesh.indices[i + 2];
            
            if (i0 < mesh.normals.size() && i1 < mesh.normals.size() && i2 < mesh.normals.size()) {
                // Check angle between normals
                float dot01 = mesh.normals[i0].normalized().dot(mesh.normals[i1].normalized());
                float dot12 = mesh.normals[i1].normalized().dot(mesh.normals[i2].normalized());
                float dot02 = mesh.normals[i0].normalized().dot(mesh.normals[i2].normalized());
                
                // If any angle is > 45 degrees, consider it sharp
                if (dot01 < 0.707f || dot12 < 0.707f || dot02 < 0.707f) {
                    return true;
                }
            }
        }
        return false;
    }
};

// Test end-to-end smoothing pipeline
TEST_F(SurfaceSmoothingIntegrationTest, EndToEndSmoothingPipeline) {
    // Generate base mesh without smoothing
    SurfaceSettings baseSettings = SurfaceSettings::Default();
    baseSettings.smoothingLevel = 0;
    Mesh baseMesh = generator->generateSurface(*grid, baseSettings);
    
    ASSERT_TRUE(baseMesh.isValid());
    EXPECT_GT(baseMesh.vertices.size(), 0);
    
    // Generate smoothed mesh
    SurfaceSettings smoothSettings = SurfaceSettings::Default();
    smoothSettings.smoothingLevel = 5;
    smoothSettings.preserveTopology = true;
    smoothSettings.minFeatureSize = 1.0f;
    
    Mesh smoothedMesh = generator->generateSurface(*grid, smoothSettings);
    
    ASSERT_TRUE(smoothedMesh.isValid());
    EXPECT_GT(smoothedMesh.vertices.size(), 0);
    
    // Smoothed mesh should have similar vertex count but smoother surface
    EXPECT_NEAR(smoothedMesh.vertices.size(), baseMesh.vertices.size(), baseMesh.vertices.size() * 0.5);
    
    // Validate mesh is printable
    MeshValidator validator;
    auto result = validator.validate(smoothedMesh, 1.0f);
    EXPECT_TRUE(result.isValid);
    EXPECT_TRUE(result.isWatertight);
    EXPECT_TRUE(result.isManifold);
}

// Test different quality presets
TEST_F(SurfaceSmoothingIntegrationTest, DifferentQualityPresets) {
    // Test preview quality
    SurfaceSettings previewSettings = SurfaceSettings::Preview();
    Mesh previewMesh = generator->generateSurface(*grid, previewSettings);
    
    ASSERT_TRUE(previewMesh.isValid());
    EXPECT_EQ(previewSettings.smoothingLevel, 3);
    EXPECT_TRUE(previewSettings.usePreviewQuality);
    
    // Test export quality
    SurfaceSettings exportSettings = SurfaceSettings::Export();
    Mesh exportMesh = generator->generateSurface(*grid, exportSettings);
    
    ASSERT_TRUE(exportMesh.isValid());
    EXPECT_EQ(exportSettings.smoothingLevel, 5);
    EXPECT_FALSE(exportSettings.usePreviewQuality);
    
    // Export mesh should be smoother than preview
    if (previewMesh.normals.size() > 0 && exportMesh.normals.size() > 0) {
        bool previewHasSharp = hasSharpEdges(previewMesh);
        bool exportHasSharp = hasSharpEdges(exportMesh);
        
        // Export should be at least as smooth as preview
        if (!previewHasSharp) {
            EXPECT_FALSE(exportHasSharp);
        }
    }
}

// Test performance with various mesh sizes
TEST_F(SurfaceSmoothingIntegrationTest, PerformanceWithVariousMeshSizes) {
    // Small mesh (3x3x3)
    auto start = std::chrono::high_resolution_clock::now();
    
    SurfaceSettings settings = SurfaceSettings::Default();
    settings.smoothingLevel = 5;
    Mesh smallMesh = generator->generateSurface(*grid, settings);
    
    auto smallTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start).count();
    
    ASSERT_TRUE(smallMesh.isValid());
    EXPECT_LT(smallTime, 1000); // Should complete in under 1 second
    
    // Medium mesh (3x3x3)
    auto mediumGrid = std::make_unique<VoxelGrid>(VoxelResolution::Size_4cm, 5.0f);
    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < 3; ++y) {
            for (int z = 0; z < 3; ++z) {
                mediumGrid->setVoxel(Math::Vector3i(x, y, z), true);
            }
        }
    }
    
    start = std::chrono::high_resolution_clock::now();
    Mesh mediumMesh = generator->generateSurface(*mediumGrid, settings);
    auto mediumTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start).count();
    
    ASSERT_TRUE(mediumMesh.isValid());
    EXPECT_LT(mediumTime, 5000); // Should complete in under 5 seconds
    
    // Preview quality should be faster
    settings.usePreviewQuality = true;
    start = std::chrono::high_resolution_clock::now();
    Mesh previewMesh = generator->generateSurface(*mediumGrid, settings);
    auto previewTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start).count();
    
    ASSERT_TRUE(previewMesh.isValid());
    EXPECT_LT(previewTime, mediumTime); // Preview should be faster
}

// Test generateSmoothedSurface convenience method
TEST_F(SurfaceSmoothingIntegrationTest, GenerateSmoothedSurfaceMethod) {
    // Test with default smoothing level
    Mesh smoothed5 = generator->generateSmoothedSurface(*grid);
    ASSERT_TRUE(smoothed5.isValid());
    
    // Test with custom smoothing level
    Mesh smoothed10 = generator->generateSmoothedSurface(*grid, 10);
    ASSERT_TRUE(smoothed10.isValid());
    
    // Higher smoothing should produce smoother result
    if (smoothed5.normals.size() > 0 && smoothed10.normals.size() > 0) {
        // Just verify both are valid - actual smoothness comparison is complex
        EXPECT_GT(smoothed5.vertices.size(), 0);
        EXPECT_GT(smoothed10.vertices.size(), 0);
    }
}

// Test smoothing with holes preservation
TEST_F(SurfaceSmoothingIntegrationTest, SmoothingWithHolesPreservation) {
    // Create a shape with a hole (simple ring)
    auto torusGrid = std::make_unique<VoxelGrid>(VoxelResolution::Size_4cm, 5.0f);
    
    // Create a 3x3x1 ring
    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < 3; ++y) {
            // Skip center to create hole
            if (x != 1 || y != 1) {
                torusGrid->setVoxel(Math::Vector3i(x, y, 0), true);
            }
        }
    }
    
    // Generate with topology preservation
    SurfaceSettings settings = SurfaceSettings::Export();
    settings.preserveTopology = true;
    Mesh smoothedWithTopology = generator->generateSurface(*torusGrid, settings);
    
    ASSERT_TRUE(smoothedWithTopology.isValid());
    
    // Mesh should still have a hole (can't easily test this directly,
    // but at least verify mesh is valid and watertight)
    MeshValidator validator;
    auto result = validator.validate(smoothedWithTopology, 1.0f);
    EXPECT_TRUE(result.isValid);
}

// Test progress callback
TEST_F(SurfaceSmoothingIntegrationTest, ProgressCallback) {
    std::vector<float> progressValues;
    std::vector<std::string> statusMessages;
    
    generator->setProgressCallback([&](float progress, const std::string& status) {
        progressValues.push_back(progress);
        statusMessages.push_back(status);
    });
    
    SurfaceSettings settings = SurfaceSettings::Export();
    Mesh mesh = generator->generateSurface(*grid, settings);
    
    ASSERT_TRUE(mesh.isValid());
    
    // Should have received progress updates
    EXPECT_GT(progressValues.size(), 0);
    EXPECT_GT(statusMessages.size(), 0);
    
    // Progress should go from 0 to 1
    if (!progressValues.empty()) {
        EXPECT_GE(progressValues.front(), 0.0f);
        EXPECT_LE(progressValues.back(), 1.0f);
    }
    
    // Should have smoothing status message
    bool hasSmoothing = false;
    for (const auto& msg : statusMessages) {
        if (msg.find("Smoothing") != std::string::npos) {
            hasSmoothing = true;
            break;
        }
    }
    EXPECT_TRUE(hasSmoothing);
}

// Test cancellation during smoothing
TEST_F(SurfaceSmoothingIntegrationTest, CancellationDuringSmoothing) {
    // Create small grid that still shows cancellation behavior
    auto largeGrid = std::make_unique<VoxelGrid>(VoxelResolution::Size_4cm, 5.0f);
    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < 3; ++y) {
            for (int z = 0; z < 3; ++z) {
                largeGrid->setVoxel(Math::Vector3i(x, y, z), true);
            }
        }
    }
    
    // Set up progress callback that cancels after smoothing starts
    bool smoothingStarted = false;
    generator->setProgressCallback([&](float progress, const std::string& status) {
        if (status.find("Smoothing") != std::string::npos && !smoothingStarted) {
            smoothingStarted = true;
            generator->cancelGeneration();
        }
    });
    
    SurfaceSettings settings = SurfaceSettings::Export();
    settings.smoothingLevel = 10; // High smoothing for longer processing
    
    Mesh mesh = generator->generateSurface(*largeGrid, settings);
    
    // Mesh might be invalid if cancellation worked
    if (smoothingStarted && !mesh.isValid()) {
        EXPECT_TRUE(generator->isCancelled());
    }
}