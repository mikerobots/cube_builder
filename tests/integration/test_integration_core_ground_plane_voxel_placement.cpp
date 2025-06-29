#include <gtest/gtest.h>
#include <memory>
#include "voxel_data/VoxelDataManager.h"
#include "visual_feedback/FaceDetector.h"
#include "input/PlacementValidation.h"
#include "undo_redo/VoxelCommands.h"
#include "undo_redo/HistoryManager.h"
#include "math/Ray.h"
#include "math/Vector3f.h"
#include "events/EventDispatcher.h"
#include "logging/Logger.h"

namespace VoxelEditor {

// Integration test that demonstrates clicking on the ground plane adds voxels
class GroundPlaneVoxelPlacementTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup logging to file only
        auto& logger = Logging::Logger::getInstance();
        logger.setLevel(Logging::LogLevel::Debug);
        logger.clearOutputs();
        logger.addOutput(std::make_unique<Logging::FileOutput>("ground_plane_test.log", "TestLog", false));
        
        // Create event dispatcher
        eventDispatcher = std::make_unique<Events::EventDispatcher>();
        
        // Create voxel manager with workspace
        voxelManager = std::make_unique<VoxelData::VoxelDataManager>(eventDispatcher.get());
        voxelManager->resizeWorkspace(Math::Vector3f(8.0f, 8.0f, 8.0f));
        voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_1cm);
        
        // Create history manager for undo/redo
        historyManager = std::make_unique<UndoRedo::HistoryManager>();
        
        // Start with empty workspace (no initial voxels)
        ASSERT_EQ(voxelManager->getVoxelCount(), 0) << "Should start with empty workspace";
    }
    
    void TearDown() override {
        historyManager.reset();
        voxelManager.reset();
        eventDispatcher.reset();
    }
    
    // Simulates clicking on the ground plane to place a voxel
    bool simulateGroundPlaneClick(const Math::Vector3f& worldPosition) {
        // 1. Create ray from above the ground plane pointing down to the click position
        Math::Vector3f rayOrigin = worldPosition + Math::Vector3f(0.0f, 5.0f, 0.0f);
        Math::Vector3f rayDirection = Math::Vector3f(0.0f, -1.0f, 0.0f);
        Math::Ray ray(rayOrigin, rayDirection);
        
        // 2. Detect ground plane intersection
        VisualFeedback::FaceDetector detector;
        VisualFeedback::Ray vfRay(ray.origin, ray.direction);
        
        const VoxelData::VoxelGrid* grid = voxelManager->getGrid(
            voxelManager->getActiveResolution()
        );
        
        // Try to detect face first, then fall back to ground plane
        VisualFeedback::Face face = detector.detectFaceOrGround(
            vfRay, 
            *grid, 
            voxelManager->getActiveResolution()
        );
        
        if (!face.isValid()) {
            return false;
        }
        
        // 3. Calculate placement position using PlacementValidation
        Math::Vector3f hitPoint;
        if (face.isGroundPlane()) {
            hitPoint = face.getGroundPlaneHitPoint().value();
        } else {
            // For voxel faces, use face center
            Math::IncrementCoordinates voxelPos = face.getVoxelPosition();
            hitPoint = Math::CoordinateConverter::incrementToWorld(voxelPos).value();
        }
        
        Math::Vector3f workspaceSize = Math::Vector3f(8.0f, 8.0f, 8.0f);
        bool shiftPressed = false; // Not using shift for this test
        
        // Convert hit point to increment coordinates
        Math::IncrementCoordinates incrementPos = Math::CoordinateConverter::worldToIncrement(
            Math::WorldCoordinates(hitPoint));
        
        // Use new VoxelDataManager validation API
        auto validation = voxelManager->validatePosition(incrementPos, voxelManager->getActiveResolution());
        
        if (!validation.valid) {
            return false;
        }
        
        // 4. Place the voxel using the command system
        auto cmd = std::make_unique<UndoRedo::VoxelEditCommand>(
            voxelManager.get(),
            incrementPos,  // Use exact position from validation
            voxelManager->getActiveResolution(),
            true  // Place voxel
        );
        
        return historyManager->executeCommand(std::move(cmd));
    }
    
    std::unique_ptr<Events::EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelData::VoxelDataManager> voxelManager;
    std::unique_ptr<UndoRedo::HistoryManager> historyManager;
};

// Test clicking on the ground plane at origin places a voxel at (0,0,0)
TEST_F(GroundPlaneVoxelPlacementTest, TestClickOriginPlacesVoxelAtOrigin) {
    // Simulate clicking at world origin (0,0,0)
    bool success = simulateGroundPlaneClick(Math::Vector3f(0.0f, 0.0f, 0.0f));
    ASSERT_TRUE(success) << "Failed to place voxel on ground plane at origin";
    
    // Verify voxel was placed at grid position (0,0,0)
    EXPECT_TRUE(voxelManager->getVoxel(Math::Vector3i(0,0,0), VoxelData::VoxelResolution::Size_1cm))
        << "Voxel should be placed at (0,0,0) when clicking at world origin";
    
    EXPECT_EQ(voxelManager->getVoxelCount(), 1) << "Should have exactly 1 voxel after ground plane click";
}

// Test clicking on ground plane at various positions
TEST_F(GroundPlaneVoxelPlacementTest, TestClickVariousGroundPositions) {
    struct TestCase {
        Math::Vector3f worldPos;
        Math::Vector3i expectedGridPos;
        std::string description;
    };
    
    // Test various positions - using 1cm resolution so 1 world unit = 100 grid units
    std::vector<TestCase> testCases = {
        {Math::Vector3f(0.0f, 0.0f, 0.0f), Math::Vector3i(0, 0, 0), "Origin"},
        {Math::Vector3f(0.01f, 0.0f, 0.0f), Math::Vector3i(1, 0, 0), "1cm X offset"},
        {Math::Vector3f(0.0f, 0.0f, 0.01f), Math::Vector3i(0, 0, 1), "1cm Z offset"},
        {Math::Vector3f(0.05f, 0.0f, 0.03f), Math::Vector3i(5, 0, 3), "5cm X, 3cm Z"},
        {Math::Vector3f(0.1f, 0.0f, 0.1f), Math::Vector3i(10, 0, 10), "10cm diagonal"}
    };
    
    for (size_t i = 0; i < testCases.size(); i++) {
        const auto& testCase = testCases[i];
        
        // Simulate the click
        bool success = simulateGroundPlaneClick(testCase.worldPos);
        ASSERT_TRUE(success) 
            << "Failed to place voxel for test case: " << testCase.description;
        
        // Verify voxel was placed at expected position
        EXPECT_TRUE(voxelManager->getVoxel(testCase.expectedGridPos, VoxelData::VoxelResolution::Size_1cm))
            << "Voxel should be placed at " << testCase.expectedGridPos.toString() 
            << " for test case: " << testCase.description;
        
        // Verify total count
        EXPECT_EQ(voxelManager->getVoxelCount(), i + 1) 
            << "Should have " << (i + 1) << " voxels after test case: " << testCase.description;
    }
}

// Test that ground plane placement works with exact 1cm increment positions
TEST_F(GroundPlaneVoxelPlacementTest, TestGroundPlaneExactPositioning) {
    // Click at exact 1cm increment positions - no snapping should occur
    Math::Vector3f exactPos(0.01f, 0.0f, 0.02f);  // Exactly at 1cm, 0cm, 2cm
    
    bool success = simulateGroundPlaneClick(exactPos);
    ASSERT_TRUE(success) << "Failed to place voxel at exact position";
    
    // Should be placed at exact grid coordinates (1,0,2)
    EXPECT_TRUE(voxelManager->getVoxel(Math::Vector3i(1, 0, 2), VoxelData::VoxelResolution::Size_1cm))
        << "Voxel should be at exact position (1,0,2) when clicking at (0.01, 0, 0.02)";
    
    // Verify only one voxel was placed
    EXPECT_EQ(voxelManager->getVoxelCount(), 1) << "Should have exactly 1 voxel";
}

// Test clicking near existing voxel vs ground plane
TEST_F(GroundPlaneVoxelPlacementTest, TestGroundVsExistingVoxel) {
    // First, place a voxel at (5,0,5)
    Math::Vector3f firstClick(0.05f, 0.0f, 0.05f);
    bool success1 = simulateGroundPlaneClick(firstClick);
    ASSERT_TRUE(success1) << "Failed to place first voxel";
    ASSERT_TRUE(voxelManager->getVoxel(Math::Vector3i(5, 0, 5), VoxelData::VoxelResolution::Size_1cm));
    
    // Now click very close to the existing voxel on the ground
    // This should place a new voxel adjacent to the first one, not on top
    Math::Vector3f secondClick(0.06f, 0.0f, 0.05f);  // 1cm to the right
    bool success2 = simulateGroundPlaneClick(secondClick);
    ASSERT_TRUE(success2) << "Failed to place second voxel";
    
    // Should place at (6,0,5)
    EXPECT_TRUE(voxelManager->getVoxel(Math::Vector3i(6, 0, 5), VoxelData::VoxelResolution::Size_1cm))
        << "Second voxel should be placed adjacent to first";
    
    EXPECT_EQ(voxelManager->getVoxelCount(), 2) << "Should have 2 voxels";
}

// Test building a simple pattern on the ground plane
TEST_F(GroundPlaneVoxelPlacementTest, TestBuildGroundPattern) {
    // Build a 3x3 square pattern on the ground plane
    std::vector<Math::Vector3f> positions;
    for (int x = 0; x < 3; x++) {
        for (int z = 0; z < 3; z++) {
            positions.push_back(Math::Vector3f(x * 0.01f, 0.0f, z * 0.01f));
        }
    }
    
    // Place all voxels
    for (size_t i = 0; i < positions.size(); i++) {
        bool success = simulateGroundPlaneClick(positions[i]);
        ASSERT_TRUE(success) << "Failed to place voxel " << i;
    }
    
    // Verify the pattern
    EXPECT_EQ(voxelManager->getVoxelCount(), 9) << "Should have 9 voxels in 3x3 pattern";
    
    for (int x = 0; x < 3; x++) {
        for (int z = 0; z < 3; z++) {
            EXPECT_TRUE(voxelManager->getVoxel(Math::Vector3i(x, 0, z), VoxelData::VoxelResolution::Size_1cm))
                << "Should have voxel at (" << x << ",0," << z << ")";
        }
    }
}

// Test that undo/redo works with ground plane placement
TEST_F(GroundPlaneVoxelPlacementTest, TestUndoRedoGroundPlacement) {
    // Place a voxel
    bool success = simulateGroundPlaneClick(Math::Vector3f(0.02f, 0.0f, 0.03f));
    ASSERT_TRUE(success);
    ASSERT_EQ(voxelManager->getVoxelCount(), 1);
    ASSERT_TRUE(voxelManager->getVoxel(Math::Vector3i(2, 0, 3), VoxelData::VoxelResolution::Size_1cm));
    
    // Undo the placement
    bool undoSuccess = historyManager->undo();
    ASSERT_TRUE(undoSuccess) << "Undo should succeed";
    EXPECT_EQ(voxelManager->getVoxelCount(), 0) << "Should have no voxels after undo";
    EXPECT_FALSE(voxelManager->getVoxel(Math::Vector3i(2, 0, 3), VoxelData::VoxelResolution::Size_1cm))
        << "Voxel should be removed after undo";
    
    // Redo the placement
    bool redoSuccess = historyManager->redo();
    ASSERT_TRUE(redoSuccess) << "Redo should succeed";
    EXPECT_EQ(voxelManager->getVoxelCount(), 1) << "Should have 1 voxel after redo";
    EXPECT_TRUE(voxelManager->getVoxel(Math::Vector3i(2, 0, 3), VoxelData::VoxelResolution::Size_1cm))
        << "Voxel should be restored after redo";
}

// Test the new requirement: voxels of any size can be placed at any 1cm position
TEST_F(GroundPlaneVoxelPlacementTest, TestArbitrarySizeVoxelsAtAnyPosition) {
    // Test that larger voxels can be placed at positions that would have been invalid under old snapping rules
    
    // Set to 4cm resolution
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_4cm);
    
    // Place a 4cm voxel at position (1,0,1) - under old rules this would snap to (0,0,0)
    // Under new rules, it should stay at (1,0,1)
    Math::Vector3f nonAlignedPos(0.01f, 0.0f, 0.01f);  // 1cm, 0cm, 1cm in world space
    bool success1 = simulateGroundPlaneClick(nonAlignedPos);
    ASSERT_TRUE(success1) << "Failed to place 4cm voxel at non-aligned position";
    
    // Verify it was placed at exact position (1,0,1), not snapped to (0,0,0)
    EXPECT_TRUE(voxelManager->getVoxel(Math::Vector3i(1, 0, 1), VoxelData::VoxelResolution::Size_4cm))
        << "4cm voxel should be at exact position (1,0,1)";
    EXPECT_FALSE(voxelManager->getVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_4cm))
        << "4cm voxel should NOT be at (0,0,0) - no snapping should occur";
    
    // Place another 4cm voxel at (10,0,10) - non-overlapping position
    // 4cm voxel at (1,0,1) extends to (5,0,5), so (10,0,10) to (14,0,14) won't overlap
    Math::Vector3f anotherPos(0.10f, 0.0f, 0.10f);  // 10cm, 0cm, 10cm in world space
    bool success2 = simulateGroundPlaneClick(anotherPos);
    ASSERT_TRUE(success2) << "Failed to place second 4cm voxel at non-overlapping position";
    
    // Verify both voxels exist at their exact positions
    EXPECT_TRUE(voxelManager->getVoxel(Math::Vector3i(10, 0, 10), VoxelData::VoxelResolution::Size_4cm))
        << "Second 4cm voxel should be at exact position (10,0,10)";
    EXPECT_EQ(voxelManager->getVoxelCount(), 2) << "Should have 2 voxels at exact positions";
    
    // Try with 16cm voxel at position (50,0,50) - well away from other voxels
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_16cm);
    Math::Vector3f largeVoxelPos(0.50f, 0.0f, 0.50f);  // 50cm, 0cm, 50cm in world space
    bool success3 = simulateGroundPlaneClick(largeVoxelPos);
    ASSERT_TRUE(success3) << "Failed to place 16cm voxel at non-aligned position";
    
    EXPECT_TRUE(voxelManager->getVoxel(Math::Vector3i(50, 0, 50), VoxelData::VoxelResolution::Size_16cm))
        << "16cm voxel should be at exact position (50,0,50)";
    EXPECT_EQ(voxelManager->getVoxelCount(VoxelData::VoxelResolution::Size_16cm), 1) 
        << "Should have 1 voxel in 16cm resolution";
}

} // namespace VoxelEditor