#include <gtest/gtest.h>
#include "../PlaneDetector.h"
#include "../../voxel_data/VoxelDataManager.h"
#include "../../voxel_data/VoxelTypes.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Input;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

class PlaneDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create voxel data manager
        m_voxelManager = std::make_unique<VoxelDataManager>();
        
        // Create plane detector
        m_planeDetector = std::make_unique<PlaneDetector>(m_voxelManager.get());
        
        // Set up a test workspace (5m³)
        m_workspaceSize = Vector3f(5.0f, 5.0f, 5.0f);
    }
    
    void TearDown() override {
        m_planeDetector.reset();
        m_voxelManager.reset();
    }
    
    // Helper function to place a voxel
    void placeVoxel(const IncrementCoordinates& pos, VoxelResolution resolution) {
        m_voxelManager->setVoxel(pos, resolution, true);
    }
    
    // Helper function to create detection context
    PlaneDetectionContext createContext(const Vector3f& worldPos, VoxelResolution resolution = VoxelResolution::Size_32cm) {
        PlaneDetectionContext context;
        context.worldPosition = worldPos;
        context.rayOrigin = Vector3f(worldPos.x, 10.0f, worldPos.z); // Ray from above
        context.rayDirection = Vector3f(0.0f, -1.0f, 0.0f); // Downward
        context.currentResolution = resolution;
        return context;
    }
    
    std::unique_ptr<VoxelDataManager> m_voxelManager;
    std::unique_ptr<PlaneDetector> m_planeDetector;
    Vector3f m_workspaceSize;
};

// Test basic plane detection on ground plane
TEST_F(PlaneDetectorTest, DetectGroundPlane) {
    // No voxels placed - should detect ground plane
    auto context = createContext(Vector3f(0.0f, 0.0f, 0.0f));
    auto result = m_planeDetector->detectPlane(context);
    
    EXPECT_TRUE(result.found);
    EXPECT_TRUE(result.plane.isGroundPlane);
    EXPECT_FLOAT_EQ(result.plane.height, 0.0f);
}

// Test plane detection with single voxel
TEST_F(PlaneDetectorTest, DetectPlaneWithSingleVoxel) {
    // Place a 32cm voxel at (0,0,0)
    placeVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_32cm);
    
    // Test detection directly above the voxel
    auto context = createContext(Vector3f(0.0f, 0.5f, 0.0f));
    auto result = m_planeDetector->detectPlane(context);
    
    EXPECT_TRUE(result.found);
    EXPECT_FALSE(result.plane.isGroundPlane);
    EXPECT_FLOAT_EQ(result.plane.height, 0.32f); // Top of 32cm voxel
    EXPECT_EQ(result.plane.referenceVoxel, IncrementCoordinates(0, 0, 0));
    EXPECT_EQ(result.plane.resolution, VoxelResolution::Size_32cm);
}

// Test plane detection with multiple voxels at same height
TEST_F(PlaneDetectorTest, DetectPlaneWithMultipleVoxels) {
    // Place multiple 32cm voxels at same Y level
    placeVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_32cm);
    placeVoxel(IncrementCoordinates(1, 0, 0), VoxelResolution::Size_32cm);
    placeVoxel(IncrementCoordinates(0, 0, 1), VoxelResolution::Size_32cm);
    
    // Test detection should find the plane at height 0.32m
    auto context = createContext(Vector3f(0.16f, 0.5f, 0.16f));
    auto result = m_planeDetector->detectPlane(context);
    
    EXPECT_TRUE(result.found);
    EXPECT_FLOAT_EQ(result.plane.height, 0.32f);
    EXPECT_GE(result.voxelsOnPlane.size(), 1); // Should find at least one voxel on plane
}

// Test highest voxel detection with multiple heights
TEST_F(PlaneDetectorTest, FindHighestVoxelUnderCursor) {
    // Place voxels at different heights
    placeVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_32cm);  // Top height: 1 * 0.32 = 0.32m
    placeVoxel(IncrementCoordinates(0, 1, 0), VoxelResolution::Size_32cm);  // Top height: 2 * 0.32 = 0.64m
    placeVoxel(IncrementCoordinates(0, 4, 0), VoxelResolution::Size_16cm);  // Top height: 5 * 0.16 = 0.80m (highest)
    
    auto highestVoxelInfo = m_planeDetector->findHighestVoxelUnderCursor(Vector3f(0.0f, 0.0f, 0.0f));
    
    EXPECT_TRUE(highestVoxelInfo.has_value());
    // Should find the highest voxel (at y=4 for 16cm resolution)
    EXPECT_EQ(highestVoxelInfo.value().position, IncrementCoordinates(0, 4, 0));
    EXPECT_EQ(highestVoxelInfo.value().resolution, VoxelResolution::Size_16cm);
}

// Test plane persistence during overlap
TEST_F(PlaneDetectorTest, PlanePersistenceDuringOverlap) {
    // Set up a voxel and establish a plane
    placeVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_32cm);
    
    auto context = createContext(Vector3f(0.0f, 0.5f, 0.0f));
    auto result = m_planeDetector->detectPlane(context);
    m_planeDetector->setCurrentPlane(result.plane);
    
    // Simulate preview overlapping the plane
    IncrementCoordinates previewPos(1, 0, 0); // Adjacent to existing voxel
    VoxelResolution previewRes = VoxelResolution::Size_32cm;
    
    // Update persistence - should maintain plane
    m_planeDetector->updatePlanePersistence(previewPos, previewRes, 0.016f); // 60 FPS frame time
    
    auto currentPlane = m_planeDetector->getCurrentPlane();
    EXPECT_TRUE(currentPlane.has_value());
    EXPECT_FLOAT_EQ(currentPlane->height, 0.32f);
}

// Test plane clearing when preview moves away
TEST_F(PlaneDetectorTest, PlaneClearingWhenPreviewClears) {
    // Set up a voxel and establish a plane
    placeVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_32cm);
    
    auto context = createContext(Vector3f(0.0f, 0.5f, 0.0f));
    auto result = m_planeDetector->detectPlane(context);
    m_planeDetector->setCurrentPlane(result.plane);
    
    // Simulate preview moving far away (no overlap)
    IncrementCoordinates previewPos(10, 0, 10); // Far from existing voxel
    VoxelResolution previewRes = VoxelResolution::Size_32cm;
    
    // Update persistence multiple times to trigger timeout
    for (int i = 0; i < 60; ++i) { // Simulate 1 second at 60 FPS
        m_planeDetector->updatePlanePersistence(previewPos, previewRes, 1.0f/60.0f);
    }
    
    auto currentPlane = m_planeDetector->getCurrentPlane();
    EXPECT_FALSE(currentPlane.has_value()); // Should be cleared
}

// Test different voxel size combinations
TEST_F(PlaneDetectorTest, DifferentVoxelSizes) {
    // Place voxels of different sizes at correct grid positions
    placeVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_32cm);   // 32cm voxel at world (0, 0, 0)
    placeVoxel(IncrementCoordinates(2, 0, 0), VoxelResolution::Size_16cm);   // 16cm voxel at world (0.32, 0, 0)
    placeVoxel(IncrementCoordinates(6, 0, 0), VoxelResolution::Size_8cm);    // 8cm voxel at world (0.48, 0, 0)
    
    // Test detection over each voxel
    auto context32 = createContext(Vector3f(0.16f, 0.5f, 0.0f));
    auto result32 = m_planeDetector->detectPlane(context32);
    EXPECT_TRUE(result32.found);
    EXPECT_FLOAT_EQ(result32.plane.height, 0.32f);
    
    auto context16 = createContext(Vector3f(0.32f, 0.5f, 0.0f));
    auto result16 = m_planeDetector->detectPlane(context16);
    EXPECT_TRUE(result16.found);
    EXPECT_FLOAT_EQ(result16.plane.height, 0.16f);
    
    auto context8 = createContext(Vector3f(0.48f, 0.5f, 0.0f));
    auto result8 = m_planeDetector->detectPlane(context8);
    EXPECT_TRUE(result8.found);
    EXPECT_FLOAT_EQ(result8.plane.height, 0.08f);
}

// Test voxel top height calculation
TEST_F(PlaneDetectorTest, VoxelTopHeightCalculation) {
    float height32 = m_planeDetector->calculateVoxelTopHeight(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_32cm);
    EXPECT_FLOAT_EQ(height32, 0.32f);
    
    float height16 = m_planeDetector->calculateVoxelTopHeight(IncrementCoordinates(0, 1, 0), VoxelResolution::Size_16cm);
    EXPECT_FLOAT_EQ(height16, 0.17f); // 0.01 + 0.16 = 0.17f
    
    float height8 = m_planeDetector->calculateVoxelTopHeight(IncrementCoordinates(0, 3, 0), VoxelResolution::Size_8cm);
    EXPECT_FLOAT_EQ(height8, 0.11f); // 0.03 + 0.08 = 0.11f
}

// Test plane transition logic
TEST_F(PlaneDetectorTest, PlaneTransitionLogic) {
    // Set current plane at 32cm height
    PlacementPlane currentPlane(0.32f, IncrementCoordinates(0, 0, 0), VoxelResolution::Size_32cm);
    m_planeDetector->setCurrentPlane(currentPlane);
    
    // Test transition to higher plane
    PlacementPlane higherPlane(0.64f, IncrementCoordinates(0, 1, 0), VoxelResolution::Size_32cm);
    PlaneDetectionResult higherResult = PlaneDetectionResult::Found(higherPlane);
    EXPECT_TRUE(m_planeDetector->shouldTransitionToNewPlane(higherResult));
    
    // Test no transition to same height plane
    PlacementPlane sameHeightPlane(0.32f, IncrementCoordinates(1, 0, 0), VoxelResolution::Size_32cm);
    PlaneDetectionResult sameResult = PlaneDetectionResult::Found(sameHeightPlane);
    EXPECT_FALSE(m_planeDetector->shouldTransitionToNewPlane(sameResult));
    
    // Test no transition to lower plane
    PlacementPlane lowerPlane(0.16f, IncrementCoordinates(0, 0, 1), VoxelResolution::Size_16cm);
    PlaneDetectionResult lowerResult = PlaneDetectionResult::Found(lowerPlane);
    EXPECT_FALSE(m_planeDetector->shouldTransitionToNewPlane(lowerResult));
}

// Test voxels at specific height query
TEST_F(PlaneDetectorTest, VoxelsAtSpecificHeight) {
    // Place voxels at different heights
    placeVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_32cm);  // Top at 0.32m
    placeVoxel(IncrementCoordinates(1, 0, 0), VoxelResolution::Size_32cm);  // Top at 0.32m
    placeVoxel(IncrementCoordinates(0, 1, 0), VoxelResolution::Size_32cm);  // Top at 0.33m
    
    auto voxelsAt32cm = m_planeDetector->getVoxelsAtHeight(0.32f);
    EXPECT_GE(voxelsAt32cm.size(), 2); // Should find at least 2 voxels
    
    auto voxelsAt64cm = m_planeDetector->getVoxelsAtHeight(0.64f);
    EXPECT_GE(voxelsAt64cm.size(), 1); // Should find at least 1 voxel
}

// Test preview overlap detection
TEST_F(PlaneDetectorTest, PreviewOverlapDetection) {
    // Place a voxel and set up plane
    placeVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_32cm);
    PlacementPlane plane(0.32f, IncrementCoordinates(0, 0, 0), VoxelResolution::Size_32cm);
    m_planeDetector->setCurrentPlane(plane);
    
    // Test overlapping preview
    bool overlaps = m_planeDetector->previewOverlapsCurrentPlane(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_32cm);
    EXPECT_TRUE(overlaps);
    
    // Test non-overlapping preview
    bool noOverlap = m_planeDetector->previewOverlapsCurrentPlane(IncrementCoordinates(10, 0, 10), VoxelResolution::Size_32cm);
    EXPECT_FALSE(noOverlap);
}

// Test reset functionality
TEST_F(PlaneDetectorTest, ResetFunctionality) {
    // Set up some state
    placeVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_32cm);
    PlacementPlane plane(0.32f, IncrementCoordinates(0, 0, 0), VoxelResolution::Size_32cm);
    m_planeDetector->setCurrentPlane(plane);
    
    EXPECT_TRUE(m_planeDetector->getCurrentPlane().has_value());
    
    // Reset and verify state is cleared
    m_planeDetector->reset();
    EXPECT_FALSE(m_planeDetector->getCurrentPlane().has_value());
}

// Test empty workspace
TEST_F(PlaneDetectorTest, EmptyWorkspace) {
    // No voxels placed - test that findHighestVoxelUnderCursor returns empty
    auto highestVoxel = m_planeDetector->findHighestVoxelUnderCursor(Vector3f(0.0f, 0.0f, 0.0f));
    EXPECT_FALSE(highestVoxel.has_value());
    
    // Detect plane should return ground plane
    auto context = createContext(Vector3f(0.0f, 0.0f, 0.0f));
    auto result = m_planeDetector->detectPlane(context);
    EXPECT_TRUE(result.found);
    EXPECT_TRUE(result.plane.isGroundPlane);
    EXPECT_FLOAT_EQ(result.plane.height, 0.0f);
}

// Test edge case: voxel at workspace boundary
TEST_F(PlaneDetectorTest, VoxelAtBoundary) {
    // Place voxel at edge of workspace
    // For 32cm voxels: grid position 7 = 7 * 0.32m = 2.24m (near edge of 5m workspace)
    Vector3i boundaryPos(7, 0, 7);
    placeVoxel(boundaryPos, VoxelResolution::Size_32cm);
    
    auto context = createContext(Vector3f(2.24f, 0.5f, 2.24f));
    auto result = m_planeDetector->detectPlane(context);
    
    EXPECT_TRUE(result.found);
    EXPECT_NEAR(result.plane.height, 0.32f, 0.0001f);
}

// Test complex stacking scenario
TEST_F(PlaneDetectorTest, ComplexStackingScenario) {
    // Create a pyramid-like structure
    placeVoxel(Vector3i(0, 0, 0), VoxelResolution::Size_32cm);  // Base level
    placeVoxel(Vector3i(1, 0, 0), VoxelResolution::Size_32cm);
    placeVoxel(Vector3i(0, 0, 1), VoxelResolution::Size_32cm);
    placeVoxel(Vector3i(1, 0, 1), VoxelResolution::Size_32cm);
    
    // For 16cm voxels on top of 32cm voxels: grid Y = 2 (since 32cm = 2 * 16cm)
    placeVoxel(Vector3i(0, 2, 0), VoxelResolution::Size_16cm);  // Second level
    placeVoxel(Vector3i(1, 2, 0), VoxelResolution::Size_16cm);
    
    // For 8cm voxels on top of 32cm+16cm: grid Y = 6 (since 32cm = 4*8cm, 16cm = 2*8cm)
    placeVoxel(Vector3i(0, 6, 0), VoxelResolution::Size_8cm);   // Top level
    
    // Test detection at different positions
    auto topResult = m_planeDetector->detectPlane(createContext(Vector3f(0.04f, 1.0f, 0.04f)));
    EXPECT_TRUE(topResult.found);
    EXPECT_NEAR(topResult.plane.height, 0.56f, 0.0001f); // 0.32 + 0.16 + 0.08 = 0.56m
    
    auto middleResult = m_planeDetector->detectPlane(createContext(Vector3f(0.16f, 1.0f, 0.04f)));
    EXPECT_TRUE(middleResult.found);
    EXPECT_NEAR(middleResult.plane.height, 0.48f, 0.0001f); // 0.32 + 0.16 = 0.48m
    
    auto baseResult = m_planeDetector->detectPlane(createContext(Vector3f(0.16f, 1.0f, 0.16f)));
    EXPECT_TRUE(baseResult.found);
    EXPECT_NEAR(baseResult.plane.height, 0.32f, 0.0001f); // Base level
}