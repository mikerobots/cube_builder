#include <gtest/gtest.h>
#include <memory>
#include <chrono>
#include <thread>
#include "voxel_data/VoxelDataManager.h"
#include "visual_feedback/FaceDetector.h"
#include "input/PlacementValidation.h"
#include "undo_redo/VoxelCommands.h"
#include "undo_redo/HistoryManager.h"
#include "camera/OrbitCamera.h"
#include "math/Vector3f.h"
#include "math/Vector2f.h"
#include "math/Vector3i.h"
#include "math/Matrix4f.h"
#include "math/Ray.h"
#include "events/EventDispatcher.h"
#include "logging/Logger.h"

namespace VoxelEditor {
namespace Tests {

class MouseGroundPlaneClickingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup logging
        auto& logger = Logging::Logger::getInstance();
        logger.setLevel(Logging::LogLevel::Debug);
        logger.clearOutputs();
        logger.addOutput(std::make_unique<Logging::FileOutput>("mouse_ground_plane_test.log", "TestLog", false));
        
        // Create event dispatcher
        eventDispatcher = std::make_unique<Events::EventDispatcher>();
        
        // Create voxel manager with workspace
        voxelManager = std::make_unique<VoxelData::VoxelDataManager>(eventDispatcher.get());
        voxelManager->resizeWorkspace(Math::Vector3f(8.0f, 8.0f, 8.0f));
        voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
        
        // Create history manager for undo/redo
        historyManager = std::make_unique<UndoRedo::HistoryManager>();
        
        // Create camera
        camera = std::make_unique<Camera::OrbitCamera>();
        camera->setOrbitAngles(45.0f, 35.26f); // Isometric-like view
        camera->setDistance(10.0f);
        camera->setTarget(Math::WorldCoordinates(Math::Vector3f(0.0f, 0.0f, 0.0f)));
        camera->setAspectRatio(1.0f);
        
        // Start with empty workspace
        ASSERT_EQ(voxelManager->getVoxelCount(), 0) << "Should start with no voxels";
    }
    
    void TearDown() override {
        camera.reset();
        historyManager.reset();
        voxelManager.reset();
        eventDispatcher.reset();
    }
    
    // Simulate mouse click by generating a ray and testing ground plane intersection
    bool simulateGroundPlaneClick(const Math::Vector3f& worldPos) {
        // For ground plane clicks, we need a ray that will hit the target world position
        // when it intersects the Y=0 plane
        
        // Create a ray from above the target position pointing down
        // This simulates clicking on the ground plane at the desired location
        Math::Vector3f rayOrigin = worldPos + Math::Vector3f(0.0f, 10.0f, 0.0f);
        Math::Vector3f rayDirection = Math::Vector3f(0.0f, -1.0f, 0.0f);
        Math::Ray ray(rayOrigin, rayDirection);
        
        // Detect ground plane intersection
        VisualFeedback::FaceDetector detector;
        VisualFeedback::Ray vfRay(ray.origin, ray.direction);
        
        const VoxelData::VoxelGrid* grid = voxelManager->getGrid(
            voxelManager->getActiveResolution()
        );
        
        // Try to detect face or ground plane
        VisualFeedback::Face face = detector.detectFaceOrGround(
            vfRay, 
            *grid, 
            voxelManager->getActiveResolution()
        );
        
        if (!face.isValid()) {
            return false;
        }
        
        // Calculate placement position
        Math::Vector3f hitPoint;
        if (face.isGroundPlane()) {
            hitPoint = face.getGroundPlaneHitPoint().value();
        } else {
            // For voxel faces, calculate hit point based on face normal
            Math::IncrementCoordinates voxelPosInc = face.getVoxelPosition();
            Math::Vector3i voxelPos = voxelPosInc.value();
            float resolution = VoxelData::getVoxelSize(voxelManager->getActiveResolution());
            hitPoint = Math::Vector3f(
                voxelPos.x * resolution,
                voxelPos.y * resolution,
                voxelPos.z * resolution
            );
            
            // Offset by half resolution in face normal direction
            Math::Vector3f normal = face.getNormal();
            hitPoint = hitPoint + normal * (resolution * 0.5f);
        }
        
        Math::Vector3f workspaceSize = voxelManager->getWorkspaceSize();
        bool shiftPressed = false;
        
        // For ground plane, we don't have a surface face
        Input::PlacementContext context;
        if (face.isGroundPlane()) {
            context = Input::PlacementUtils::getSmartPlacementContext(
                Math::WorldCoordinates(hitPoint),
                voxelManager->getActiveResolution(),
                shiftPressed,
                workspaceSize,
                *voxelManager
            );
        } else {
            // For voxel faces, pass the face information
            Math::IncrementCoordinates faceVoxelPos = face.getVoxelPosition();
            // Convert VisualFeedback::FaceDirection to VoxelData::FaceDirection
            VoxelData::FaceDirection faceDir = VoxelData::FaceDirection::PosY; // Default to top
            switch (face.getDirection()) {
                case VisualFeedback::FaceDirection::PositiveX: faceDir = VoxelData::FaceDirection::PosX; break;
                case VisualFeedback::FaceDirection::NegativeX: faceDir = VoxelData::FaceDirection::NegX; break;
                case VisualFeedback::FaceDirection::PositiveY: faceDir = VoxelData::FaceDirection::PosY; break;
                case VisualFeedback::FaceDirection::NegativeY: faceDir = VoxelData::FaceDirection::NegY; break;
                case VisualFeedback::FaceDirection::PositiveZ: faceDir = VoxelData::FaceDirection::PosZ; break;
                case VisualFeedback::FaceDirection::NegativeZ: faceDir = VoxelData::FaceDirection::NegZ; break;
            }
            
            context = Input::PlacementUtils::getSmartPlacementContext(
                Math::WorldCoordinates(hitPoint),
                voxelManager->getActiveResolution(),
                shiftPressed,
                workspaceSize,
                *voxelManager,
                &faceVoxelPos,
                voxelManager->getActiveResolution(),
                faceDir
            );
        }
        
        if (context.validation != Input::PlacementValidationResult::Valid) {
            Logging::Logger::getInstance().errorfc("MouseGroundPlaneClickingTest",
                "Placement validation failed: %d for position (%.3f, %.3f, %.3f)",
                static_cast<int>(context.validation), hitPoint.x, hitPoint.y, hitPoint.z);
            return false;
        }
        
        // Place the voxel using the command system
        // Log the placement position for debugging
        Logging::Logger::getInstance().infofc("MouseGroundPlaneClickingTest",
            "Placing voxel at increment grid pos (%d, %d, %d) from hit point (%.3f, %.3f, %.3f)",
            context.snappedIncrementPos.x(), context.snappedIncrementPos.y(), context.snappedIncrementPos.z(),
            hitPoint.x, hitPoint.y, hitPoint.z);
        
        auto cmd = std::make_unique<UndoRedo::VoxelEditCommand>(
            voxelManager.get(),
            context.snappedIncrementPos,
            voxelManager->getActiveResolution(),
            true // add voxel
        );
        
        return historyManager->executeCommand(std::move(cmd));
    }
    
    // Helper to check if voxel exists at grid position
    // Note: This function expects VoxelGrid coordinates and converts them to
    // the increment coordinates that VoxelDataManager expects
    bool hasVoxelAt(const Math::Vector3i& gridPos) {
        // Convert VoxelGrid coordinates to world coordinates first
        const VoxelData::VoxelGrid* grid = voxelManager->getGrid(voxelManager->getActiveResolution());
        if (!grid) return false;
        
        Math::Vector3f worldPos = grid->incrementToWorld(Math::IncrementCoordinates(gridPos)).value();
        
        // Convert world coordinates to increment grid coordinates (1cm grid)
        const float INCREMENT_SIZE = 0.01f;
        Math::Vector3i incrementGridPos(
            static_cast<int>(std::round(worldPos.x / INCREMENT_SIZE)),
            static_cast<int>(std::round(worldPos.y / INCREMENT_SIZE)),
            static_cast<int>(std::round(worldPos.z / INCREMENT_SIZE))
        );
        
        // Check using VoxelDataManager which uses increment coordinates
        return voxelManager->getVoxel(incrementGridPos, voxelManager->getActiveResolution());
    }
    
    // Helper to get voxel count
    size_t getVoxelCount() {
        return voxelManager->getVoxelCount();
    }
    
    std::unique_ptr<Events::EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelData::VoxelDataManager> voxelManager;
    std::unique_ptr<UndoRedo::HistoryManager> historyManager;
    std::unique_ptr<Camera::OrbitCamera> camera;
};

// Test clicking on ground plane at workspace center
TEST_F(MouseGroundPlaneClickingTest, ClickGroundPlaneAtOrigin) {
    ASSERT_EQ(getVoxelCount(), 0) << "Should start with no voxels";
    
    // Click on ground plane at origin (centered coordinate system)
    // In the centered system, (0,0,0) in world space maps to (0,0,0) in increment coordinates
    bool success = simulateGroundPlaneClick(Math::Vector3f(0.0f, 0.0f, 0.0f));
    
    EXPECT_TRUE(success) << "Should successfully place voxel on ground plane";
    EXPECT_EQ(getVoxelCount(), 1) << "Should have placed one voxel";
    // The voxel should be at increment position (0,0,0)
    EXPECT_TRUE(hasVoxelAt(Math::Vector3i(0, 0, 0))) << "Voxel should be at increment position (0,0,0)";
}

// Test clicking on ground plane at various positions
TEST_F(MouseGroundPlaneClickingTest, ClickGroundPlaneMultiplePositions) {
    ASSERT_EQ(getVoxelCount(), 0) << "Should start with no voxels";
    
    // Test positions on ground plane (Y=0)
    // With new requirements, 8cm voxels can be placed at any 1cm position
    // We need non-overlapping positions for 8cm voxels
    std::vector<Math::Vector3f> testPositions = {
        {0.00f, 0.0f, 0.00f},    // Origin
        {0.20f, 0.0f, 0.00f},    // 20cm +X (non-overlapping)
        {0.00f, 0.0f, 0.20f},    // 20cm +Z (non-overlapping)
        {0.20f, 0.0f, 0.20f},    // 20cm +X+Z (non-overlapping)
        {-0.20f, 0.0f, 0.00f},   // 20cm -X (non-overlapping)
        {0.00f, 0.0f, -0.20f},   // 20cm -Z (non-overlapping)
        {-0.20f, 0.0f, -0.20f},  // 20cm -X-Z (non-overlapping)
    };
    
    // With new requirements, voxels can be placed at any 1cm position
    // No snapping to 8cm boundaries - direct conversion to increment coordinates
    std::vector<Math::Vector3i> expectedIncrementPos;
    for (const auto& worldPos : testPositions) {
        // Convert world position directly to increment position (1cm = 1 increment)
        const float INCREMENT_SIZE = 0.01f;
        Math::Vector3i incrementPos(
            static_cast<int>(std::round(worldPos.x / INCREMENT_SIZE)),
            static_cast<int>(std::round(worldPos.y / INCREMENT_SIZE)),
            static_cast<int>(std::round(worldPos.z / INCREMENT_SIZE))
        );
        expectedIncrementPos.push_back(incrementPos);
    }
    
    for (size_t i = 0; i < testPositions.size(); ++i) {
        bool success = simulateGroundPlaneClick(testPositions[i]);
        
        EXPECT_TRUE(success) << "Should place voxel at position " << i << " world pos (" 
            << testPositions[i].x << ", " << testPositions[i].y << ", " << testPositions[i].z << ")";
        
        // For debugging: Print where voxel was actually placed
        if (success) {
            Logging::Logger::getInstance().debugfc("MouseGroundPlaneClickingTest",
                "Placed voxel %zu at world position (%.2f, %.2f, %.2f)",
                i, testPositions[i].x, testPositions[i].y, testPositions[i].z);
        }
        
        // Check using increment coordinates directly
        EXPECT_TRUE(voxelManager->getVoxel(expectedIncrementPos[i], voxelManager->getActiveResolution())) 
            << "Voxel should be placed at increment position (" 
            << expectedIncrementPos[i].x << ", " 
            << expectedIncrementPos[i].y << ", " 
            << expectedIncrementPos[i].z << ")";
    }
    
    EXPECT_EQ(getVoxelCount(), testPositions.size()) 
        << "Should have placed " << testPositions.size() << " voxels";
}

// Test clicking near existing voxels
TEST_F(MouseGroundPlaneClickingTest, ClickNearExistingVoxel) {
    // Place initial voxel at origin
    Math::Vector3f initPos(0.0f, 0.0f, 0.0f);
    bool success = simulateGroundPlaneClick(initPos);
    
    ASSERT_TRUE(success) << "Should place initial voxel";
    ASSERT_EQ(getVoxelCount(), 1) << "Should have placed initial voxel";
    
    // Check using increment coordinates - with new requirements, no snapping
    const float INCREMENT_SIZE = 0.01f;
    Math::Vector3i expectedIncrement(
        static_cast<int>(std::round(initPos.x / INCREMENT_SIZE)),
        static_cast<int>(std::round(initPos.y / INCREMENT_SIZE)),
        static_cast<int>(std::round(initPos.z / INCREMENT_SIZE))
    );
    ASSERT_TRUE(voxelManager->getVoxel(expectedIncrement, voxelManager->getActiveResolution())) 
        << "Initial voxel should be at expected increment position";
    
    // Place adjacent voxel - with new requirements, can place at any 1cm position
    // Place an 8cm voxel at a position that won't overlap with the first one at (0,0,0)
    Math::Vector3f adjacentPos(0.10f, 0.0f, 0.0f); // 10cm away from origin
    success = simulateGroundPlaneClick(adjacentPos);
    
    EXPECT_TRUE(success) << "Should place adjacent voxel";
    EXPECT_EQ(getVoxelCount(), 2) << "Should have placed second voxel";
    
    // With new requirements, voxel should be at exact 1cm position (10,0,0)
    Math::Vector3i expectedIncrement2(10, 0, 0); // 10cm = 10 increment units
    EXPECT_TRUE(voxelManager->getVoxel(expectedIncrement2, voxelManager->getActiveResolution())) 
        << "Second voxel should be at exact position (10,0,0)";
}

// Test ground plane constraint
TEST_F(MouseGroundPlaneClickingTest, GroundPlaneYConstraint) {
    ASSERT_EQ(getVoxelCount(), 0) << "Should start with no voxels";
    
    // Our ray generation creates a ray from above pointing down, so it will always hit Y=0
    bool success = simulateGroundPlaneClick(Math::Vector3f(0.0f, 0.0f, 0.0f));
    
    // The click should succeed
    EXPECT_TRUE(success) << "Should place voxel on ground plane";
    
    if (success) {
        // In centered coordinates, clicking at (0,0,0) should place at increment (0,0,0)
        EXPECT_TRUE(hasVoxelAt(Math::Vector3i(0, 0, 0))) << "Voxel should be at Y=0";
    }
}

// Test workspace boundaries
TEST_F(MouseGroundPlaneClickingTest, WorkspaceBoundaryConstraints) {
    // Try to place voxels at workspace edges 
    // Workspace validation uses centered coords: -4 to +4
    std::vector<Math::Vector3f> boundaryPositions = {
        {3.92f, 0.0f, 0.0f},    // Near +X boundary (centered)
        {-3.96f, 0.0f, 0.0f},   // Near -X boundary (centered)
        {0.0f, 0.0f, 3.92f},    // Near +Z boundary (centered)
        {0.0f, 0.0f, -3.96f},   // Near -Z boundary (centered)
    };
    
    for (const auto& pos : boundaryPositions) {
        bool success = simulateGroundPlaneClick(pos);
        EXPECT_TRUE(success) << "Should place voxel near boundary at " 
                            << pos.x << ", " << pos.y << ", " << pos.z;
    }
    
    // Try to place outside boundaries (centered coords)
    std::vector<Math::Vector3f> outsidePositions = {
        {4.04f, 0.0f, 0.0f},    // Outside +X (> 4m in centered)
        {-4.04f, 0.0f, 0.0f},   // Outside -X (< -4m in centered)
        {0.0f, 0.0f, 4.04f},    // Outside +Z (> 4m in centered)
        {0.0f, 0.0f, -4.04f},   // Outside -Z (< -4m in centered)
    };
    
    size_t countBefore = getVoxelCount();
    for (const auto& pos : outsidePositions) {
        bool success = simulateGroundPlaneClick(pos);
        EXPECT_FALSE(success) << "Should not place voxel outside boundary at " 
                             << pos.x << ", " << pos.y << ", " << pos.z;
    }
    
    EXPECT_EQ(getVoxelCount(), countBefore) 
        << "No voxels should be placed outside boundaries";
}

// Test undo/redo functionality
TEST_F(MouseGroundPlaneClickingTest, UndoRedoGroundPlanePlacement) {
    // Place several voxels near bottom-left corner
    simulateGroundPlaneClick(Math::Vector3f(-3.96f, 0.0f, -3.96f));
    simulateGroundPlaneClick(Math::Vector3f(-3.88f, 0.0f, -3.96f));
    simulateGroundPlaneClick(Math::Vector3f(-3.96f, 0.0f, -3.88f));
    
    ASSERT_EQ(getVoxelCount(), 3) << "Should have placed 3 voxels";
    
    // Undo one placement
    historyManager->undo();
    EXPECT_EQ(getVoxelCount(), 2) << "Should have 2 voxels after undo";
    
    // Undo another
    historyManager->undo();
    EXPECT_EQ(getVoxelCount(), 1) << "Should have 1 voxel after second undo";
    
    // Redo
    historyManager->redo();
    EXPECT_EQ(getVoxelCount(), 2) << "Should have 2 voxels after redo";
    
    // Redo again
    historyManager->redo();
    EXPECT_EQ(getVoxelCount(), 3) << "Should have 3 voxels after second redo";
}

} // namespace Tests
} // namespace VoxelEditor