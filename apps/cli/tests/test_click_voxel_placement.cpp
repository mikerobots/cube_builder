#include <gtest/gtest.h>
#include <memory>
#include "voxel_data/VoxelDataManager.h"
#include "visual_feedback/FaceDetector.h"
#include "undo_redo/VoxelCommands.h"
#include "undo_redo/HistoryManager.h"
#include "math/Ray.h"
#include "math/Vector3f.h"
#include "events/EventDispatcher.h"
#include "logging/Logger.h"

namespace VoxelEditor {

// Test fixture that simulates the complete click-to-place-voxel flow
class ClickVoxelPlacementTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup logging to file only
        auto& logger = Logging::Logger::getInstance();
        logger.setLevel(Logging::LogLevel::Debug);
        logger.clearOutputs();
        logger.addOutput(std::make_unique<Logging::FileOutput>("click_test.log", "TestLog", false));
        
        // Create event dispatcher
        eventDispatcher = std::make_unique<Events::EventDispatcher>();
        
        // Create voxel manager
        voxelManager = std::make_unique<VoxelData::VoxelDataManager>(eventDispatcher.get());
        voxelManager->resizeWorkspace(Math::Vector3f(8.0f, 8.0f, 8.0f));
        voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_64cm);
        
        // Create history manager for undo/redo
        historyManager = std::make_unique<UndoRedo::HistoryManager>();
        
        // Place initial voxel at (5,5,5) for testing
        voxelManager->setVoxel(
            Math::Vector3i(5, 5, 5), 
            VoxelData::VoxelResolution::Size_64cm, 
            true
        );
    }
    
    void TearDown() override {
        historyManager.reset();
        voxelManager.reset();
        eventDispatcher.reset();
    }
    
    // Simulates the complete flow of clicking on a voxel face to place a new voxel
    bool simulateClickPlacement(const Math::Ray& ray) {
        // 1. Detect which face the ray hits
        VisualFeedback::FaceDetector detector;
        VisualFeedback::Ray vfRay(ray.origin, ray.direction);
        
        const VoxelData::VoxelGrid* grid = voxelManager->getGrid(
            voxelManager->getActiveResolution()
        );
        
        VisualFeedback::Face face = detector.detectFace(
            vfRay, 
            *grid, 
            voxelManager->getActiveResolution()
        );
        
        if (!face.isValid()) {
            return false;
        }
        
        // 2. Calculate placement position based on the face
        Math::Vector3i clickedVoxel = face.getVoxelPosition();
        Math::Vector3f normal = face.getNormal();
        
        Math::Vector3i newPos = clickedVoxel;
        if (normal.x > 0.5f) newPos.x += 1;
        else if (normal.x < -0.5f) newPos.x -= 1;
        else if (normal.y > 0.5f) newPos.y += 1;
        else if (normal.y < -0.5f) newPos.y -= 1;
        else if (normal.z > 0.5f) newPos.z += 1;
        else if (normal.z < -0.5f) newPos.z -= 1;
        
        // 3. Place the voxel using the same command system as MouseInteraction
        auto cmd = std::make_unique<UndoRedo::VoxelEditCommand>(
            voxelManager.get(),
            newPos,
            voxelManager->getActiveResolution(),
            true  // Place voxel
        );
        
        return historyManager->executeCommand(std::move(cmd));
    }
    
    std::unique_ptr<Events::EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelData::VoxelDataManager> voxelManager;
    std::unique_ptr<UndoRedo::HistoryManager> historyManager;
};

// Test clicking on two different faces adds two voxels
TEST_F(ClickVoxelPlacementTest, TestClickingTwoFacesAddsTwoVoxels) {
    // Verify initial state - one voxel at (5,5,5)
    ASSERT_TRUE(voxelManager->getVoxel(Math::Vector3i(5,5,5), VoxelData::VoxelResolution::Size_64cm));
    ASSERT_EQ(voxelManager->getVoxelCount(), 1);
    
    float voxelSize = 0.64f;
    Math::Vector3f voxelCenter(5.0f * voxelSize + voxelSize * 0.5f,
                               5.0f * voxelSize + voxelSize * 0.5f,
                               5.0f * voxelSize + voxelSize * 0.5f);
    
    // Test 1: Click on the positive X face (right side)
    {
        // Ray from right side hitting the right face
        Math::Vector3f rayOrigin = voxelCenter + Math::Vector3f(2.0f, 0, 0);
        Math::Vector3f rayTarget = voxelCenter + Math::Vector3f(0.5f * voxelSize, 0, 0);
        Math::Vector3f direction = (rayTarget - rayOrigin).normalized();
        Math::Ray ray(rayOrigin, direction);
        
        // Simulate the click
        bool success = simulateClickPlacement(ray);
        ASSERT_TRUE(success) << "Failed to place voxel on positive X face";
        
        // Verify a new voxel was placed at (6,5,5)
        ASSERT_TRUE(voxelManager->getVoxel(Math::Vector3i(6,5,5), VoxelData::VoxelResolution::Size_64cm))
            << "Voxel should be placed at (6,5,5) when clicking positive X face";
        ASSERT_EQ(voxelManager->getVoxelCount(), 2) << "Should have 2 voxels after first click";
    }
    
    // Test 2: Click on the positive Y face (top)
    {
        // Ray from above hitting the top face
        Math::Vector3f rayOrigin = voxelCenter + Math::Vector3f(0, 2.0f, 0);
        Math::Vector3f rayTarget = voxelCenter + Math::Vector3f(0, 0.5f * voxelSize, 0);
        Math::Vector3f direction = (rayTarget - rayOrigin).normalized();
        Math::Ray ray(rayOrigin, direction);
        
        // Simulate the click
        bool success = simulateClickPlacement(ray);
        ASSERT_TRUE(success) << "Failed to place voxel on positive Y face";
        
        // Verify a new voxel was placed at (5,6,5)
        ASSERT_TRUE(voxelManager->getVoxel(Math::Vector3i(5,6,5), VoxelData::VoxelResolution::Size_64cm))
            << "Voxel should be placed at (5,6,5) when clicking positive Y face";
        ASSERT_EQ(voxelManager->getVoxelCount(), 3) << "Should have 3 voxels after second click";
    }
    
    // Verify final state
    EXPECT_TRUE(voxelManager->getVoxel(Math::Vector3i(5,5,5), VoxelData::VoxelResolution::Size_64cm));
    EXPECT_TRUE(voxelManager->getVoxel(Math::Vector3i(6,5,5), VoxelData::VoxelResolution::Size_64cm));
    EXPECT_TRUE(voxelManager->getVoxel(Math::Vector3i(5,6,5), VoxelData::VoxelResolution::Size_64cm));
}

// Test sequential clicking to build a row of voxels
TEST_F(ClickVoxelPlacementTest, TestSequentialClicking) {
    // Start with one voxel at (5,5,5)
    ASSERT_TRUE(voxelManager->getVoxel(Math::Vector3i(5,5,5), VoxelData::VoxelResolution::Size_64cm));
    ASSERT_EQ(voxelManager->getVoxelCount(), 1);
    
    float voxelSize = 0.64f;
    
    // Place 3 more voxels in a row by clicking on the positive X face repeatedly
    for (int i = 0; i < 3; i++) {
        // Calculate current rightmost voxel position
        int currentX = 5 + i;
        Math::Vector3f voxelCenter(
            currentX * voxelSize + voxelSize * 0.5f,
            5.0f * voxelSize + voxelSize * 0.5f,
            5.0f * voxelSize + voxelSize * 0.5f
        );
        
        // Ray hitting its positive X face
        Math::Vector3f rayOrigin = voxelCenter + Math::Vector3f(2.0f, 0, 0);
        Math::Vector3f rayTarget = voxelCenter + Math::Vector3f(0.5f * voxelSize, 0, 0);
        Math::Vector3f direction = (rayTarget - rayOrigin).normalized();
        Math::Ray ray(rayOrigin, direction);
        
        // Simulate the click
        bool success = simulateClickPlacement(ray);
        ASSERT_TRUE(success) 
            << "Failed to place voxel " << (i + 1) << " at position (" << (currentX + 1) << ",5,5)";
        
        // Verify new voxel was placed
        ASSERT_TRUE(voxelManager->getVoxel(Math::Vector3i(currentX + 1, 5, 5), VoxelData::VoxelResolution::Size_64cm))
            << "Voxel should be placed at (" << (currentX + 1) << ",5,5)";
        ASSERT_EQ(voxelManager->getVoxelCount(), i + 2) 
            << "Should have " << (i + 2) << " voxels after click " << (i + 1);
    }
    
    // Verify we have a row of 4 voxels
    for (int x = 5; x <= 8; x++) {
        EXPECT_TRUE(voxelManager->getVoxel(Math::Vector3i(x, 5, 5), VoxelData::VoxelResolution::Size_64cm))
            << "Should have voxel at (" << x << ",5,5)";
    }
}

// Test that clicking on a newly placed voxel works correctly
TEST_F(ClickVoxelPlacementTest, TestClickingNewlyPlacedVoxel) {
    // Start with one voxel at (5,5,5)
    ASSERT_TRUE(voxelManager->getVoxel(Math::Vector3i(5,5,5), VoxelData::VoxelResolution::Size_64cm));
    
    float voxelSize = 0.64f;
    Math::Vector3f voxelCenter(5.0f * voxelSize + voxelSize * 0.5f,
                               5.0f * voxelSize + voxelSize * 0.5f,
                               5.0f * voxelSize + voxelSize * 0.5f);
    
    // Step 1: Click on positive X face to place voxel at (6,5,5)
    {
        Math::Vector3f rayOrigin = voxelCenter + Math::Vector3f(2.0f, 0, 0);
        Math::Vector3f rayTarget = voxelCenter + Math::Vector3f(0.5f * voxelSize, 0, 0);
        Math::Vector3f direction = (rayTarget - rayOrigin).normalized();
        Math::Ray ray(rayOrigin, direction);
        
        bool success = simulateClickPlacement(ray);
        ASSERT_TRUE(success) << "Failed to place first voxel";
        ASSERT_TRUE(voxelManager->getVoxel(Math::Vector3i(6,5,5), VoxelData::VoxelResolution::Size_64cm));
    }
    
    // Step 2: Click on the newly placed voxel's positive X face to place at (7,5,5)
    {
        Math::Vector3f newVoxelCenter(6.0f * voxelSize + voxelSize * 0.5f,
                                      5.0f * voxelSize + voxelSize * 0.5f,
                                      5.0f * voxelSize + voxelSize * 0.5f);
        
        Math::Vector3f rayOrigin = newVoxelCenter + Math::Vector3f(2.0f, 0, 0);
        Math::Vector3f rayTarget = newVoxelCenter + Math::Vector3f(0.5f * voxelSize, 0, 0);
        Math::Vector3f direction = (rayTarget - rayOrigin).normalized();
        Math::Ray ray(rayOrigin, direction);
        
        // This is the critical test - can we click on the newly placed voxel?
        bool success = simulateClickPlacement(ray);
        ASSERT_TRUE(success) << "Failed to click on newly placed voxel";
        ASSERT_TRUE(voxelManager->getVoxel(Math::Vector3i(7,5,5), VoxelData::VoxelResolution::Size_64cm))
            << "Should be able to place voxel by clicking on the newly placed voxel";
    }
    
    // Verify final state - 3 voxels in a row
    EXPECT_EQ(voxelManager->getVoxelCount(), 3);
    EXPECT_TRUE(voxelManager->getVoxel(Math::Vector3i(5,5,5), VoxelData::VoxelResolution::Size_64cm));
    EXPECT_TRUE(voxelManager->getVoxel(Math::Vector3i(6,5,5), VoxelData::VoxelResolution::Size_64cm));
    EXPECT_TRUE(voxelManager->getVoxel(Math::Vector3i(7,5,5), VoxelData::VoxelResolution::Size_64cm));
}

} // namespace VoxelEditor