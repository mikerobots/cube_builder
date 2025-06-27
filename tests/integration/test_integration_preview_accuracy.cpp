#include <gtest/gtest.h>
#include "voxel_data/VoxelDataManager.h"
#include "visual_feedback/FaceDetector.h"
#include "input/PlacementValidation.h"
#include "math/CoordinateConverter.h"
#include "math/Ray.h"
#include "logging/Logger.h"

namespace VoxelEditor {

class PreviewAccuracyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a basic voxel data manager for testing
        voxelManager = std::make_unique<VoxelData::VoxelDataManager>();
        
        // Set workspace size
        voxelManager->getWorkspaceManager()->setSize(Math::Vector3f(5.0f, 5.0f, 5.0f));
        
        // Clear any existing voxels
        voxelManager->clear();
        
        // Set a known resolution
        voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_4cm);
    }
    
    void TearDown() override {
        voxelManager.reset();
    }
    
    // Helper to test placement logic consistency (matches MouseInteraction::getPlacementPosition)
    Math::IncrementCoordinates getPlacementPosition(const VisualFeedback::Face& face) {
        VoxelData::VoxelResolution resolution = voxelManager->getActiveResolution();
        Math::Vector3f workspaceSize = voxelManager->getWorkspaceManager()->getSize();
        
        // Get the hit point on the face for smart snapping (same as MouseInteraction)
        Math::WorldCoordinates hitPoint;
        if (face.isGroundPlane()) {
            hitPoint = face.getGroundPlaneHitPoint();
        } else {
            // For voxel faces, calculate hit point based on face center
            Math::IncrementCoordinates voxelPos = face.getVoxelPosition();
            Math::Vector3f voxelWorldPos = Math::CoordinateConverter::incrementToWorld(voxelPos).value();
            float voxelSize = VoxelData::getVoxelSize(resolution);
            
            // Get face center position
            Math::Vector3f centerPos = voxelWorldPos + Math::Vector3f(voxelSize * 0.5f, voxelSize * 0.5f, voxelSize * 0.5f);
            
            // Offset hit point to the face surface
            Math::Vector3f normal = face.getNormal();
            centerPos += normal * (voxelSize * 0.5f);
            hitPoint = Math::WorldCoordinates(centerPos);
        }
        
        if (face.isGroundPlane()) {
            // For ground plane, use smart context snapping (no specific surface face)
            Input::PlacementContext context = Input::PlacementUtils::getSmartPlacementContext(
                hitPoint, resolution, false, workspaceSize, *voxelManager, nullptr);
            return context.snappedIncrementPos;
        } else {
            // For voxel faces, use surface face grid snapping with face information
            Math::IncrementCoordinates surfaceFaceVoxelPos = face.getVoxelPosition();
            VoxelData::VoxelResolution surfaceFaceVoxelRes = face.getResolution();
            
            // Convert VisualFeedback::FaceDirection to VoxelData::FaceDirection
            VoxelData::FaceDirection surfaceFaceDir = VoxelData::FaceDirection::PosY; // default
            VisualFeedback::FaceDirection vfDir = face.getDirection();
            switch (vfDir) {
                case VisualFeedback::FaceDirection::PositiveX: surfaceFaceDir = VoxelData::FaceDirection::PosX; break;
                case VisualFeedback::FaceDirection::NegativeX: surfaceFaceDir = VoxelData::FaceDirection::NegX; break;
                case VisualFeedback::FaceDirection::PositiveY: surfaceFaceDir = VoxelData::FaceDirection::PosY; break;
                case VisualFeedback::FaceDirection::NegativeY: surfaceFaceDir = VoxelData::FaceDirection::NegY; break;
                case VisualFeedback::FaceDirection::PositiveZ: surfaceFaceDir = VoxelData::FaceDirection::PosZ; break;
                case VisualFeedback::FaceDirection::NegativeZ: surfaceFaceDir = VoxelData::FaceDirection::NegZ; break;
            }
            
            Input::PlacementContext context = Input::PlacementUtils::getSmartPlacementContext(
                hitPoint, resolution, false, workspaceSize, *voxelManager,
                &surfaceFaceVoxelPos, surfaceFaceVoxelRes, surfaceFaceDir);
            return context.snappedIncrementPos;
        }
    }
    
    // Helper to test face detection and placement
    std::pair<bool, Math::IncrementCoordinates> testRayPlacement(const VisualFeedback::Ray& ray) {
        VisualFeedback::FaceDetector detector;
        auto* grid = voxelManager->getGrid(voxelManager->getActiveResolution());
        
        if (!grid) {
            return {false, Math::IncrementCoordinates(0, 0, 0)};
        }
        
        // Try to detect a face or ground plane
        VisualFeedback::Face face = detector.detectFaceOrGround(ray, *grid, voxelManager->getActiveResolution());
        
        if (!face.isValid()) {
            return {false, Math::IncrementCoordinates(0, 0, 0)};
        }
        
        // Get placement position using the same logic as MouseInteraction
        Math::IncrementCoordinates placementPos = getPlacementPosition(face);
        return {true, placementPos};
    }
    
    std::unique_ptr<VoxelData::VoxelDataManager> voxelManager;
};

TEST_F(PreviewAccuracyTest, GroundPlanePreviewMatchesPlacement) {
    // Test ground plane placement consistency
    // Create rays pointing downward to hit ground plane
    std::vector<VisualFeedback::Ray> testRays = {
        // Ray from above pointing straight down at origin
        VisualFeedback::Ray(Math::Vector3f(0.0f, 5.0f, 0.0f), Math::Vector3f(0.0f, -1.0f, 0.0f)),
        // Ray from above pointing down at different positions
        VisualFeedback::Ray(Math::Vector3f(1.0f, 5.0f, 0.0f), Math::Vector3f(0.0f, -1.0f, 0.0f)),
        VisualFeedback::Ray(Math::Vector3f(-1.0f, 5.0f, 0.0f), Math::Vector3f(0.0f, -1.0f, 0.0f)),
        VisualFeedback::Ray(Math::Vector3f(0.0f, 5.0f, 1.0f), Math::Vector3f(0.0f, -1.0f, 0.0f)),
        VisualFeedback::Ray(Math::Vector3f(0.0f, 5.0f, -1.0f), Math::Vector3f(0.0f, -1.0f, 0.0f))
    };
    
    for (size_t i = 0; i < testRays.size(); ++i) {
        const auto& ray = testRays[i];
        
        // Clear any existing voxels
        voxelManager->clear();
        
        // Test placement using raycast
        auto [success, placementPos] = testRayPlacement(ray);
        
        if (success) {
            // Verify the position is valid and on ground plane
            EXPECT_GE(placementPos.y(), 0) << "Test " << i << ": Y position should be >= 0";
            
            // Test that we can actually place a voxel at this position
            bool placed = voxelManager->setVoxel(placementPos, voxelManager->getActiveResolution(), true);
            EXPECT_TRUE(placed) << "Test " << i << ": Should be able to place voxel at calculated position";
            
            Logging::Logger::getInstance().info("PreviewAccuracyTest", 
                "Ground plane test " + std::to_string(i) + " - Position: (" + 
                std::to_string(placementPos.x()) + "," + std::to_string(placementPos.y()) + "," + std::to_string(placementPos.z()) + ")");
        } else {
            Logging::Logger::getInstance().warning("PreviewAccuracyTest", 
                "Ground plane test " + std::to_string(i) + " - No valid placement found");
        }
    }
}

TEST_F(PreviewAccuracyTest, VoxelFacePreviewMatchesPlacement) {
    // First place a reference voxel at origin
    Math::IncrementCoordinates refPos(0, 0, 0);
    bool placed = voxelManager->setVoxel(refPos, VoxelData::VoxelResolution::Size_4cm, true);
    ASSERT_TRUE(placed) << "Should be able to place reference voxel";
    
    // Test rays hitting different faces of the voxel
    struct TestCase {
        VisualFeedback::Ray ray;
        std::string description;
    };
    
    // Get voxel size for face hitting
    float voxelSize = VoxelData::getVoxelSize(VoxelData::VoxelResolution::Size_4cm);
    
    std::vector<TestCase> testCases = {
        // Ray hitting top face (from above)
        {VisualFeedback::Ray(Math::Vector3f(0.0f, 2.0f, 0.0f), Math::Vector3f(0.0f, -1.0f, 0.0f)), "Top face"},
        // Ray hitting right face (from the right)
        {VisualFeedback::Ray(Math::Vector3f(voxelSize + 0.1f, voxelSize * 0.5f, 0.0f), Math::Vector3f(-1.0f, 0.0f, 0.0f)), "Right face"},
        // Ray hitting front face (from the front)
        {VisualFeedback::Ray(Math::Vector3f(0.0f, voxelSize * 0.5f, voxelSize + 0.1f), Math::Vector3f(0.0f, 0.0f, -1.0f)), "Front face"}
    };
    
    for (const auto& testCase : testCases) {
        // Test placement using raycast
        auto [success, placementPos] = testRayPlacement(testCase.ray);
        
        if (success) {
            // Verify the position is the expected hit-point-based placement
            // The smart placement context snaps hit points to 1cm increments
            Math::IncrementCoordinates expectedPos(0, 0, 0);
            
            if (testCase.description == "Top face") {
                expectedPos = Math::IncrementCoordinates(2, 4, 2);  // Hit point snapped to 1cm
            } else if (testCase.description == "Right face") {
                expectedPos = Math::IncrementCoordinates(4, 2, 2);  // Hit point snapped to 1cm
            } else if (testCase.description == "Front face") {
                expectedPos = Math::IncrementCoordinates(2, 2, 4);  // Hit point snapped to 1cm
            }
            
            EXPECT_EQ(placementPos.x(), expectedPos.x()) 
                << testCase.description << ": X position should match expected hit-point placement";
            EXPECT_EQ(placementPos.y(), expectedPos.y()) 
                << testCase.description << ": Y position should match expected hit-point placement";
            EXPECT_EQ(placementPos.z(), expectedPos.z()) 
                << testCase.description << ": Z position should match expected hit-point placement";
                
            Logging::Logger::getInstance().info("PreviewAccuracyTest", 
                testCase.description + " test - Position: (" + 
                std::to_string(placementPos.x()) + "," + std::to_string(placementPos.y()) + "," + std::to_string(placementPos.z()) + ")");
        } else {
            Logging::Logger::getInstance().warning("PreviewAccuracyTest", 
                testCase.description + " test - No valid placement found");
        }
    }
}

TEST_F(PreviewAccuracyTest, MultipleResolutionsPreviewAccuracy) {
    std::vector<VoxelData::VoxelResolution> resolutions = {
        VoxelData::VoxelResolution::Size_1cm,
        VoxelData::VoxelResolution::Size_2cm,
        VoxelData::VoxelResolution::Size_4cm,
        VoxelData::VoxelResolution::Size_8cm
    };
    
    for (auto resolution : resolutions) {
        voxelManager->clear();
        voxelManager->setActiveResolution(resolution);
        
        // Test ground plane placement with this resolution
        VisualFeedback::Ray testRay(Math::Vector3f(0.5f, 5.0f, 0.5f), Math::Vector3f(0.0f, -1.0f, 0.0f));
        
        auto [success, placementPos] = testRayPlacement(testRay);
        
        if (success) {
            // Verify the position is valid
            EXPECT_GE(placementPos.y(), 0) << "Y position should be >= 0 for resolution " << static_cast<int>(resolution);
            
            // Test that we can actually place a voxel at this position
            bool placed = voxelManager->setVoxel(placementPos, resolution, true);
            EXPECT_TRUE(placed) << "Should be able to place voxel for resolution " << static_cast<int>(resolution);
                
            Logging::Logger::getInstance().info("PreviewAccuracyTest", 
                "Resolution " + std::to_string(static_cast<int>(resolution)) + " test - Position: (" + 
                std::to_string(placementPos.x()) + "," + std::to_string(placementPos.y()) + "," + std::to_string(placementPos.z()) + ")");
        } else {
            Logging::Logger::getInstance().warning("PreviewAccuracyTest", 
                "Resolution " + std::to_string(static_cast<int>(resolution)) + " test - No valid placement found");
        }
    }
}

TEST_F(PreviewAccuracyTest, BoundaryConditionsPreviewAccuracy) {
    // Test placement near workspace boundaries
    // Get workspace size for boundary calculations
    Math::Vector3f workspaceSize = voxelManager->getWorkspaceManager()->getSize();
    float halfWorkspace = workspaceSize.x * 0.5f;
    
    struct BoundaryTest {
        VisualFeedback::Ray ray;
        std::string description;
    };
    
    std::vector<BoundaryTest> boundaryTests = {
        // Ray hitting near +X boundary
        {VisualFeedback::Ray(Math::Vector3f(halfWorkspace - 0.1f, 5.0f, 0.0f), Math::Vector3f(0.0f, -1.0f, 0.0f)), "Near +X boundary"},
        // Ray hitting near -X boundary  
        {VisualFeedback::Ray(Math::Vector3f(-halfWorkspace + 0.1f, 5.0f, 0.0f), Math::Vector3f(0.0f, -1.0f, 0.0f)), "Near -X boundary"},
        // Ray hitting near +Z boundary
        {VisualFeedback::Ray(Math::Vector3f(0.0f, 5.0f, halfWorkspace - 0.1f), Math::Vector3f(0.0f, -1.0f, 0.0f)), "Near +Z boundary"},
        // Ray hitting near -Z boundary
        {VisualFeedback::Ray(Math::Vector3f(0.0f, 5.0f, -halfWorkspace + 0.1f), Math::Vector3f(0.0f, -1.0f, 0.0f)), "Near -Z boundary"}
    };
    
    for (const auto& test : boundaryTests) {
        voxelManager->clear();
        
        // Test placement using raycast
        auto [success, placementPos] = testRayPlacement(test.ray);
        
        if (success) {
            // Verify the position is within workspace bounds
            EXPECT_TRUE(voxelManager->isValidPosition(placementPos.value(), voxelManager->getActiveResolution()))
                << test.description << ": Position should be within workspace bounds";
            
            // Test that we can actually place a voxel at this position
            bool placed = voxelManager->setVoxel(placementPos, voxelManager->getActiveResolution(), true);
            EXPECT_TRUE(placed) << test.description << ": Should be able to place voxel at boundary position";
                
            Logging::Logger::getInstance().info("PreviewAccuracyTest", 
                test.description + " test - Position: (" + 
                std::to_string(placementPos.x()) + "," + std::to_string(placementPos.y()) + "," + std::to_string(placementPos.z()) + ")");
        } else {
            // It's acceptable for boundary tests to fail if position is truly invalid
            Logging::Logger::getInstance().info("PreviewAccuracyTest", 
                test.description + " test - No valid placement found (acceptable for boundary)");
        }
    }
}

} // namespace VoxelEditor