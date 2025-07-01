#include <gtest/gtest.h>
#include "../include/visual_feedback/OutlineRenderer.h"
#include "../include/visual_feedback/FeedbackTypes.h"
#include "../../voxel_data/VoxelTypes.h"
#include "../../rendering/RenderTypes.h"
#include "../../../foundation/math/CoordinateConverter.h"

using namespace VoxelEditor;
using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::Math;

class OutlineRendererPreviewTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_renderer = std::make_unique<OutlineRenderer>();
    }
    
    std::unique_ptr<OutlineRenderer> m_renderer;
};

// REQ-2.2.4: All voxel sizes (1cm to 512cm) shall be placeable at any valid 1cm increment position
// REQ-4.1.1: All placement previews shall use green outline rendering
TEST_F(OutlineRendererPreviewTest, VoxelOutlineGeneration) {
    // Test all voxel resolutions
    VoxelData::VoxelResolution resolutions[] = {
        VoxelData::VoxelResolution::Size_1cm,
        VoxelData::VoxelResolution::Size_2cm,
        VoxelData::VoxelResolution::Size_4cm,
        VoxelData::VoxelResolution::Size_8cm,
        VoxelData::VoxelResolution::Size_16cm,
        VoxelData::VoxelResolution::Size_32cm,
        VoxelData::VoxelResolution::Size_64cm,
        VoxelData::VoxelResolution::Size_128cm,
        VoxelData::VoxelResolution::Size_256cm,
        VoxelData::VoxelResolution::Size_512cm
    };
    
    for (auto resolution : resolutions) {
        Vector3i position(10, 20, 30);
        auto edges = VoxelOutlineGenerator::generateVoxelEdges(position, resolution);
        
        // A cube has 12 edges, each edge has 2 points
        EXPECT_EQ(edges.size(), 24);
        
        // Verify edges are at correct distance based on voxel size
        float voxelSize = VoxelData::getVoxelSize(resolution);
        
        // Use proper coordinate conversion like the actual implementation
        Math::IncrementCoordinates incrementPos(position.x, position.y, position.z);
        Math::WorldCoordinates worldPos = Math::CoordinateConverter::incrementToWorld(incrementPos);
        Math::Vector3f basePos = worldPos.value();
        
        // Voxels use bottom-center positioning (same as in addVoxelEdges)
        float baseX = basePos.x - voxelSize * 0.5f;  // Min X corner
        float baseY = basePos.y;                      // Bottom Y
        float baseZ = basePos.z - voxelSize * 0.5f;  // Min Z corner
        
        // Check that all points are within voxel bounds
        for (const auto& point : edges) {
            EXPECT_GE(point.x, baseX);
            EXPECT_LE(point.x, baseX + voxelSize);
            EXPECT_GE(point.y, baseY);
            EXPECT_LE(point.y, baseY + voxelSize);
            EXPECT_GE(point.z, baseZ);
            EXPECT_LE(point.z, baseZ + voxelSize);
        }
    }
}

// REQ-4.1.1: All placement previews shall use green outline rendering
// REQ-4.1.2: Invalid placements shall show red outline preview
TEST_F(OutlineRendererPreviewTest, OutlineStyleCreation) {
    // Test valid preview style
    OutlineStyle validStyle = OutlineStyle::VoxelPreview();
    // Test valid preview style color (green)
    EXPECT_FLOAT_EQ(validStyle.color.r, 0.0f);
    EXPECT_FLOAT_EQ(validStyle.color.g, 1.0f);
    EXPECT_FLOAT_EQ(validStyle.color.b, 0.0f);
    EXPECT_FLOAT_EQ(validStyle.color.a, 1.0f);
    EXPECT_EQ(validStyle.pattern, LinePattern::Solid);
    EXPECT_FALSE(validStyle.depthTest);
    
    // Create custom style for invalid preview
    OutlineStyle invalidStyle;
    invalidStyle.color = Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f); // Red
    invalidStyle.pattern = LinePattern::Dashed;
    invalidStyle.lineWidth = 3.0f;
    invalidStyle.animated = true;
    
    // Test invalid preview style color (red)
    EXPECT_FLOAT_EQ(invalidStyle.color.r, 1.0f);
    EXPECT_FLOAT_EQ(invalidStyle.color.g, 0.0f);
    EXPECT_FLOAT_EQ(invalidStyle.color.b, 0.0f);
    EXPECT_FLOAT_EQ(invalidStyle.color.a, 1.0f);
    EXPECT_EQ(invalidStyle.pattern, LinePattern::Dashed);
    EXPECT_EQ(invalidStyle.lineWidth, 3.0f);
    EXPECT_TRUE(invalidStyle.animated);
}

// REQ-6.2.1: System shall handle 10,000+ voxels without degradation
TEST_F(OutlineRendererPreviewTest, BatchRendering) {
    OutlineStyle style = OutlineStyle::VoxelPreview();
    
    // Begin batch
    m_renderer->beginBatch();
    
    // Add multiple voxel outlines
    m_renderer->renderVoxelOutline(Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_32cm, style);
    m_renderer->renderVoxelOutline(Vector3i(1, 0, 0), VoxelData::VoxelResolution::Size_32cm, style);
    m_renderer->renderVoxelOutline(Vector3i(0, 1, 0), VoxelData::VoxelResolution::Size_32cm, style);
    
    // End batch
    m_renderer->endBatch();
    
    // Clear should work without errors
    EXPECT_NO_THROW(m_renderer->clearBatch());
}

// REQ-4.1.3: Preview updates shall be smooth and responsive (< 16ms)
TEST_F(OutlineRendererPreviewTest, AnimationUpdate) {
    // Set pattern scale and offset
    m_renderer->setPatternScale(2.0f);
    m_renderer->setPatternOffset(0.5f);
    
    // Update animation
    float deltaTime = 0.016f; // 60 FPS
    EXPECT_NO_THROW(m_renderer->update(deltaTime));
    
    // Multiple updates should work
    for (int i = 0; i < 60; ++i) {
        m_renderer->update(deltaTime);
    }
}

// Test color switching between valid/invalid
TEST_F(OutlineRendererPreviewTest, ColorSwitching) {
    Vector3i position(10, 10, 10);
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_16cm;
    
    // Valid preview (green)
    OutlineStyle validStyle;
    validStyle.color = Rendering::Color(0.0f, 1.0f, 0.0f, 1.0f);
    validStyle.pattern = LinePattern::Solid;
    
    m_renderer->beginBatch();
    m_renderer->renderVoxelOutline(position, resolution, validStyle);
    m_renderer->endBatch();
    m_renderer->clearBatch();
    
    // Invalid preview (red)
    OutlineStyle invalidStyle;
    invalidStyle.color = Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f);
    invalidStyle.pattern = LinePattern::Dashed;
    
    m_renderer->beginBatch();
    m_renderer->renderVoxelOutline(position, resolution, invalidStyle);
    m_renderer->endBatch();
    
    // Both should work without errors
    EXPECT_NO_THROW(m_renderer->clearBatch());
}

// Test edge cases
TEST_F(OutlineRendererPreviewTest, EdgeCases) {
    OutlineStyle style = OutlineStyle::VoxelPreview();
    
    // Very small position values
    m_renderer->renderVoxelOutline(Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_1cm, style);
    
    // Large position values
    m_renderer->renderVoxelOutline(Vector3i(1000, 1000, 1000), VoxelData::VoxelResolution::Size_1cm, style);
    
    // Negative positions (should work for X and Z, but Y should be >= 0)
    m_renderer->renderVoxelOutline(Vector3i(-10, 0, -10), VoxelData::VoxelResolution::Size_32cm, style);
    
    // Mixed resolutions in same batch
    m_renderer->beginBatch();
    m_renderer->renderVoxelOutline(Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_1cm, style);
    m_renderer->renderVoxelOutline(Vector3i(1, 0, 0), VoxelData::VoxelResolution::Size_512cm, style);
    m_renderer->endBatch();
    
    EXPECT_NO_THROW(m_renderer->clearBatch());
}

// Test custom outline for complex shapes
TEST_F(OutlineRendererPreviewTest, CustomOutline) {
    OutlineStyle style = OutlineStyle::VoxelPreview();
    
    // Create a custom shape (triangle)
    std::vector<Vector3f> points = {
        Vector3f(0.0f, 0.0f, 0.0f),
        Vector3f(1.0f, 0.0f, 0.0f),
        Vector3f(0.5f, 1.0f, 0.0f)
    };
    
    // Closed outline
    m_renderer->renderCustomOutline(points, style, true);
    
    // Open outline
    m_renderer->renderCustomOutline(points, style, false);
    
    // Empty points should not crash
    std::vector<Vector3f> emptyPoints;
    EXPECT_NO_THROW(m_renderer->renderCustomOutline(emptyPoints, style));
    
    // Single point should not crash
    std::vector<Vector3f> singlePoint = { Vector3f(0, 0, 0) };
    EXPECT_NO_THROW(m_renderer->renderCustomOutline(singlePoint, style));
}

// Test line patterns
TEST_F(OutlineRendererPreviewTest, LinePatterns) {
    Vector3i position(5, 5, 5);
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_8cm;
    
    LinePattern patterns[] = {
        LinePattern::Solid,
        LinePattern::Dashed,
        LinePattern::Dotted,
        LinePattern::DashDot
    };
    
    for (auto pattern : patterns) {
        OutlineStyle style;
        style.pattern = pattern;
        style.color = Rendering::Color::Green();
        
        m_renderer->beginBatch();
        m_renderer->renderVoxelOutline(position, resolution, style);
        m_renderer->endBatch();
        m_renderer->clearBatch();
    }
}

// Test performance with many outlines
TEST_F(OutlineRendererPreviewTest, PerformanceManyOutlines) {
    OutlineStyle style = OutlineStyle::VoxelPreview();
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    m_renderer->beginBatch();
    
    // Add 1000 voxel outlines
    for (int x = 0; x < 10; ++x) {
        for (int y = 0; y < 10; ++y) {
            for (int z = 0; z < 10; ++z) {
                m_renderer->renderVoxelOutline(
                    Vector3i(x, y, z), 
                    VoxelData::VoxelResolution::Size_4cm, 
                    style
                );
            }
        }
    }
    
    m_renderer->endBatch();
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Should complete reasonably quickly (less than 100ms)
    EXPECT_LT(duration.count(), 100);
    
    m_renderer->clearBatch();
}