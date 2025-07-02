#include <gtest/gtest.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <iostream>
#include "cli/Application.h"
#include "cli/MouseInteraction.h"
#include "voxel_data/VoxelDataManager.h"
#include "visual_feedback/FaceDetector.h"
#include "camera/CameraController.h"
#include "foundation/math/CoordinateConverter.h"
#include "foundation/logging/Logger.h"

namespace VoxelEditor {
namespace Tests {

// Friend class to access protected members of MouseInteraction
class MouseInteractionTestHelper : public MouseInteraction {
public:
    using MouseInteraction::MouseInteraction;
    using MouseInteraction::getPlacementPosition; // Make it public in test helper
};

class MouseFacePlacementTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Enable debug logging to see what's happening
        Logging::Logger::getInstance().setLevel(Logging::LogLevel::Debug);
        Logging::Logger::getInstance().addOutput(std::make_unique<Logging::ConsoleOutput>());
        
        // Create application in headless mode
        app = std::make_unique<Application>();
        
        // Initialize in headless mode
        int argc = 2;
        char arg0[] = "test";
        char arg1[] = "--headless";
        char* argv[] = {arg0, arg1, nullptr};
        
        bool initialized = app->initialize(argc, argv);
        ASSERT_TRUE(initialized) << "Failed to initialize application";
        
        // Get managers
        voxelManager = app->getVoxelManager();
        cameraController = app->getCameraController();
        
        // Create mouse interaction test helper
        mouseInteraction = std::make_unique<MouseInteractionTestHelper>(app.get());
        mouseInteraction->initialize();
    }
    
    void TearDown() override {
        mouseInteraction.reset();
        app.reset();
    }
    
    // Helper to simulate the full mouse interaction flow
    glm::ivec3 getPlacementPositionForFace(const VisualFeedback::Face& face) {
        return mouseInteraction->getPlacementPosition(face);
    }
    
    std::unique_ptr<Application> app;
    VoxelData::VoxelDataManager* voxelManager = nullptr;
    Camera::CameraController* cameraController = nullptr;
    std::unique_ptr<MouseInteractionTestHelper> mouseInteraction;
};

// Test the actual mouse interaction placement logic
TEST_F(MouseFacePlacementTest, TestMousePlacementOnTopFace) {
    // Test with different resolutions
    std::vector<VoxelData::VoxelResolution> resolutions = {
        VoxelData::VoxelResolution::Size_8cm,
        VoxelData::VoxelResolution::Size_16cm,
        VoxelData::VoxelResolution::Size_32cm,
        VoxelData::VoxelResolution::Size_64cm
    };
    
    for (auto resolution : resolutions) {
        voxelManager->clear();
        voxelManager->setActiveResolution(resolution);
        
        int voxelSize_cm = static_cast<int>(VoxelData::getVoxelSize(resolution) * 100.0f);
        
        // Place initial voxel at ground
        Math::IncrementCoordinates basePos(0, 0, 0);
        voxelManager->setVoxel(basePos.value(), resolution, true);
        
        // Create a face representing the top face of this voxel
        VisualFeedback::Face topFace(basePos, resolution, VisualFeedback::FaceDirection::PositiveY);
        
        // Get the placement position through mouse interaction
        glm::ivec3 placementPos = getPlacementPositionForFace(topFace);
        
        std::cout << "\nResolution: " << voxelSize_cm << "cm" << std::endl;
        std::cout << "Base voxel at: (0, 0, 0)" << std::endl;
        std::cout << "Placement position: (" << placementPos.x << ", " << placementPos.y << ", " << placementPos.z << ")" << std::endl;
        std::cout << "Expected: (0, " << voxelSize_cm << ", 0)" << std::endl;
        
        // The placement position should be exactly one voxel size above
        EXPECT_EQ(placementPos.x, 0) << "X position incorrect for " << voxelSize_cm << "cm voxel";
        EXPECT_EQ(placementPos.y, voxelSize_cm) << "Y position incorrect for " << voxelSize_cm << "cm voxel";
        EXPECT_EQ(placementPos.z, 0) << "Z position incorrect for " << voxelSize_cm << "cm voxel";
        
        // Place the voxel at the calculated position
        bool placed = voxelManager->setVoxel(Math::Vector3i(placementPos.x, placementPos.y, placementPos.z), resolution, true);
        EXPECT_TRUE(placed) << "Failed to place voxel at calculated position";
        
        // Verify vertex alignment instead of gap detection
        // In sparse voxel system, intermediate positions are not "occupied" - only discrete voxel positions exist
        // Validate that the placement creates perfect face-to-face alignment
        Math::WorldCoordinates baseWorldPos = Math::CoordinateConverter::incrementToWorld(Math::IncrementCoordinates(0, 0, 0));
        Math::WorldCoordinates topWorldPos = Math::CoordinateConverter::incrementToWorld(Math::IncrementCoordinates(placementPos.x, placementPos.y, placementPos.z));
        
        float voxelSizeMeters = VoxelData::getVoxelSize(resolution);
        float baseTopY = baseWorldPos.value().y + voxelSizeMeters;  // Top face of base voxel
        float topBottomY = topWorldPos.value().y;                   // Bottom face of top voxel
        
        // The faces should align exactly - no gap, no overlap
        EXPECT_FLOAT_EQ(baseTopY, topBottomY) 
            << "Top face of base voxel should align exactly with bottom face of top voxel for " << voxelSize_cm << "cm voxels. "
            << "Base top Y: " << baseTopY << ", Top bottom Y: " << topBottomY;
    }
}

// Test with smart snapping disabled (shift key pressed)
TEST_F(MouseFacePlacementTest, TestPlacementWithShiftKey) {
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    voxelManager->setActiveResolution(resolution);
    
    // Place base voxel
    Math::IncrementCoordinates basePos(0, 0, 0);
    voxelManager->setVoxel(basePos.value(), resolution, true);
    
    // Create top face
    VisualFeedback::Face topFace(basePos, resolution, VisualFeedback::FaceDirection::PositiveY);
    
    // Note: In headless mode, we can't actually press shift, but we can test the logic
    // by checking what the smart snapping would do
    glm::ivec3 placementPos = getPlacementPositionForFace(topFace);
    
    std::cout << "\n32cm voxel placement (simulated):" << std::endl;
    std::cout << "Placement position: (" << placementPos.x << ", " << placementPos.y << ", " << placementPos.z << ")" << std::endl;
    
    // Should still place at Y=32 for proper alignment
    EXPECT_EQ(placementPos.y, 32) << "Y position should be 32cm above base";
}

// Test placement on different faces
TEST_F(MouseFacePlacementTest, TestPlacementOnAllFaces) {
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_16cm;
    voxelManager->setActiveResolution(resolution);
    
    // Place center voxel
    Math::IncrementCoordinates centerPos(0, 16, 0); // One voxel above ground
    voxelManager->setVoxel(centerPos.value(), resolution, true);
    
    struct FaceTest {
        VisualFeedback::FaceDirection faceDir;
        Math::Vector3i expectedOffset;
        std::string name;
    };
    
    std::vector<FaceTest> faceTests = {
        {VisualFeedback::FaceDirection::PositiveX, Math::Vector3i(16, 0, 0), "PositiveX"},
        {VisualFeedback::FaceDirection::NegativeX, Math::Vector3i(-16, 0, 0), "NegativeX"},
        {VisualFeedback::FaceDirection::PositiveY, Math::Vector3i(0, 16, 0), "PositiveY"},
        {VisualFeedback::FaceDirection::NegativeY, Math::Vector3i(0, -16, 0), "NegativeY"},
        {VisualFeedback::FaceDirection::PositiveZ, Math::Vector3i(0, 0, 16), "PositiveZ"},
        {VisualFeedback::FaceDirection::NegativeZ, Math::Vector3i(0, 0, -16), "NegativeZ"}
    };
    
    for (const auto& test : faceTests) {
        VisualFeedback::Face face(centerPos, resolution, test.faceDir);
        glm::ivec3 placementPos = getPlacementPositionForFace(face);
        
        Math::Vector3i expected = centerPos.value() + test.expectedOffset;
        
        std::cout << "\nFace: " << test.name << std::endl;
        std::cout << "Center voxel at: (" << centerPos.x() << ", " << centerPos.y() << ", " << centerPos.z() << ")" << std::endl;
        std::cout << "Placement: (" << placementPos.x << ", " << placementPos.y << ", " << placementPos.z << ")" << std::endl;
        std::cout << "Expected: (" << expected.x << ", " << expected.y << ", " << expected.z << ")" << std::endl;
        
        EXPECT_EQ(placementPos.x, expected.x) << "X incorrect for " << test.name;
        EXPECT_EQ(placementPos.y, expected.y) << "Y incorrect for " << test.name;
        EXPECT_EQ(placementPos.z, expected.z) << "Z incorrect for " << test.name;
    }
}

} // namespace Tests
} // namespace VoxelEditor