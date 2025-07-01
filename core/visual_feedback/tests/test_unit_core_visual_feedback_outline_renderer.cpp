#include <gtest/gtest.h>
#include "../include/visual_feedback/OutlineRenderer.h"
#include "../../../foundation/math/CoordinateConverter.h"

using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;

class OutlineRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        renderer = std::make_unique<OutlineRenderer>();
    }
    
    std::unique_ptr<OutlineRenderer> renderer;
};

TEST_F(OutlineRendererTest, VoxelOutline) {
    // REQ-2.2.1, REQ-3.2.1, REQ-4.1.1: Green outline preview rendering
    Vector3i position(1, 2, 3);
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    OutlineStyle style = OutlineStyle::VoxelPreview();
    
    EXPECT_NO_THROW(renderer->renderVoxelOutline(position, resolution, style));
}

TEST_F(OutlineRendererTest, BoxOutline) {
    BoundingBox box(Vector3f(0, 0, 0), Vector3f(1, 1, 1));
    OutlineStyle style = OutlineStyle::SelectionBox();
    
    EXPECT_NO_THROW(renderer->renderBoxOutline(box, style));
}

TEST_F(OutlineRendererTest, CustomOutline) {
    std::vector<Vector3f> points = {
        Vector3f(0, 0, 0),
        Vector3f(1, 0, 0),
        Vector3f(1, 1, 0),
        Vector3f(0, 1, 0)
    };
    
    OutlineStyle style = OutlineStyle::GroupBoundary();
    
    EXPECT_NO_THROW(renderer->renderCustomOutline(points, style, true));
    EXPECT_NO_THROW(renderer->renderCustomOutline(points, style, false));
}

TEST_F(OutlineRendererTest, BatchMode) {
    OutlineStyle style = OutlineStyle::VoxelPreview();
    
    // Test batch mode
    renderer->beginBatch();
    
    renderer->renderVoxelOutline(Vector3i(0, 0, 0), VoxelResolution::Size_32cm, style);
    renderer->renderVoxelOutline(Vector3i(1, 0, 0), VoxelResolution::Size_32cm, style);
    renderer->renderVoxelOutline(Vector3i(0, 1, 0), VoxelResolution::Size_32cm, style);
    
    renderer->endBatch();
    
    // Should not crash
    EXPECT_NO_THROW(renderer->clearBatch());
}

TEST_F(OutlineRendererTest, EmptyOutlines) {
    // Empty points vector
    std::vector<Vector3f> emptyPoints;
    EXPECT_NO_THROW(renderer->renderCustomOutline(emptyPoints, OutlineStyle::GroupBoundary()));
    
    // Single point
    std::vector<Vector3f> singlePoint = {Vector3f(0, 0, 0)};
    EXPECT_NO_THROW(renderer->renderCustomOutline(singlePoint, OutlineStyle::GroupBoundary()));
}

TEST_F(OutlineRendererTest, PatternSettings) {
    renderer->setPatternScale(2.0f);
    renderer->setPatternOffset(0.5f);
    
    Vector3i pos(0, 0, 0);
    OutlineStyle style = OutlineStyle::GroupBoundary();
    style.pattern = LinePattern::Dashed;
    
    EXPECT_NO_THROW(renderer->renderVoxelOutline(pos, VoxelResolution::Size_32cm, style));
}

TEST_F(OutlineRendererTest, Animation) {
    // REQ-2.2.3, REQ-4.1.3: Real-time preview updates
    EXPECT_NO_THROW(renderer->update(0.016f));
    
    // Add animated outline
    OutlineStyle style = OutlineStyle::SelectionBox();
    style.animated = true;
    style.animationSpeed = 2.0f;
    
    renderer->renderBoxOutline(BoundingBox(Vector3f(0, 0, 0), Vector3f(1, 1, 1)), style);
    
    EXPECT_NO_THROW(renderer->update(0.016f));
}

TEST_F(OutlineRendererTest, VoxelOutlineGenerator) {
    Vector3i position(5, 10, 15);
    VoxelResolution resolution = VoxelResolution::Size_32cm;
    
    auto edges = VoxelOutlineGenerator::generateVoxelEdges(position, resolution);
    
    // Should have 24 points (12 edges * 2 points each)
    EXPECT_EQ(edges.size(), 24);
    
    // Test that edges form a valid cube using proper coordinate conversion
    float voxelSize = getVoxelSize(resolution);
    
    // Convert increment coordinates to world coordinates using CoordinateConverter
    VoxelEditor::Math::IncrementCoordinates incrementPos(position.x, position.y, position.z);
    VoxelEditor::Math::WorldCoordinates worldPos = VoxelEditor::Math::CoordinateConverter::incrementToWorld(incrementPos);
    Vector3f basePos = worldPos.value();
    
    // Calculate expected bounds with bottom-center positioning
    // X and Z are centered, Y starts at bottom
    Vector3f expectedMin(
        basePos.x - voxelSize * 0.5f,
        basePos.y,
        basePos.z - voxelSize * 0.5f
    );
    
    Vector3f expectedMax(
        basePos.x + voxelSize * 0.5f,
        basePos.y + voxelSize,
        basePos.z + voxelSize * 0.5f
    );
    
    // Check that all points are within the expected bounds
    for (const auto& point : edges) {
        EXPECT_GE(point.x, expectedMin.x);
        EXPECT_GE(point.y, expectedMin.y);
        EXPECT_GE(point.z, expectedMin.z);
        EXPECT_LE(point.x, expectedMax.x);
        EXPECT_LE(point.y, expectedMax.y);
        EXPECT_LE(point.z, expectedMax.z);
    }
}

TEST_F(OutlineRendererTest, DifferentLinePatterns) {
    Vector3i pos(0, 0, 0);
    VoxelResolution res = VoxelResolution::Size_32cm;
    
    std::vector<LinePattern> patterns = {
        LinePattern::Solid,
        LinePattern::Dashed,
        LinePattern::Dotted,
        LinePattern::DashDot
    };
    
    for (auto pattern : patterns) {
        OutlineStyle style = OutlineStyle::VoxelPreview();
        style.pattern = pattern;
        
        EXPECT_NO_THROW(renderer->renderVoxelOutline(pos, res, style));
    }
}