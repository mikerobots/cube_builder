#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "visual_feedback/FaceDetector.h"
#include "voxel_data/VoxelDataManager.h"
#include "voxel_data/VoxelGrid.h"
#include "math/CoordinateTypes.h"
#include "math/CoordinateConverter.h"
#include "logging/Logger.h"

using namespace VoxelEditor;

class FaceDetectionBoundaryTest : public ::testing::Test {
protected:
    std::unique_ptr<VisualFeedback::FaceDetector> faceDetector;
    std::unique_ptr<VoxelData::VoxelDataManager> voxelDataManager;
    
    void SetUp() override {
        // Create face detector and voxel data manager
        faceDetector = std::make_unique<VisualFeedback::FaceDetector>();
        voxelDataManager = std::make_unique<VoxelData::VoxelDataManager>();
        
        // Set default workspace size to 5m x 5m x 5m
        voxelDataManager->resizeWorkspace(Math::Vector3f(5.0f, 5.0f, 5.0f));
    }
    
    void TearDown() override {
        faceDetector.reset();
        voxelDataManager.reset();
    }
};

// Test that voxels placed at workspace boundaries can be detected by rays
TEST_F(FaceDetectionBoundaryTest, VoxelsAtWorkspaceBoundariesAreDetectable) {
    // Get workspace bounds
    auto workspaceSize = voxelDataManager->getWorkspaceSize();
    float halfX = workspaceSize.x * 0.5f;
    float halfZ = workspaceSize.z * 0.5f;
    
    // Place voxels at various boundary positions
    // Testing with 16cm voxels for visibility
    auto resolution = VoxelData::VoxelResolution::Size_16cm;
    voxelDataManager->setActiveResolution(resolution);
    
    struct BoundaryVoxel {
        Math::IncrementCoordinates pos;
        std::string description;
    };
    
    std::vector<BoundaryVoxel> boundaryVoxels = {
        // Near X boundaries (convert from world to increment coordinates)
        {Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(halfX - 0.16f, 0.0f, 0.0f)), "Near positive X boundary"},
        {Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(-halfX + 0.16f, 0.0f, 0.0f)), "Near negative X boundary"},
        
        // Near Z boundaries
        {Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(0.0f, 0.0f, halfZ - 0.16f)), "Near positive Z boundary"},
        {Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(0.0f, 0.0f, -halfZ + 0.16f)), "Near negative Z boundary"},
        
        // Corner positions
        {Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(halfX - 0.16f, 0.0f, halfZ - 0.16f)), "Near positive X,Z corner"},
        {Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(-halfX + 0.16f, 0.0f, -halfZ + 0.16f)), "Near negative X,Z corner"},
        
        // Near top boundary
        {Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(0.0f, workspaceSize.y - 0.16f, 0.0f)), "Near top boundary"}
    };
    
    // Place all boundary voxels
    for (const auto& bv : boundaryVoxels) {
        bool placed = voxelDataManager->setVoxel(bv.pos, resolution, true);
        ASSERT_TRUE(placed) << "Failed to place voxel at " << bv.description 
                           << " pos: (" << bv.pos.x() << ", " << bv.pos.y() << ", " << bv.pos.z() << ")";
        // Voxel placed successfully for boundary testing
    }
    
    // Test ray detection from various angles
    struct RayTest {
        Math::WorldCoordinates origin;
        Math::Vector3f direction;
        std::string description;
        size_t expectedVoxelIndex; // Which boundary voxel should be hit
    };
    
    std::vector<RayTest> rayTests = {
        // Rays from outside workspace towards boundary voxels
        // Use Y=0.08 which is half the voxel height (16cm/2 = 8cm = 0.08m) - middle of voxel
        {{halfX + 1.0f, 0.08f, 0.0f}, {-1.0f, 0.0f, 0.0f}, "Ray from outside +X boundary", 0},
        {{-halfX - 1.0f, 0.08f, 0.0f}, {1.0f, 0.0f, 0.0f}, "Ray from outside -X boundary", 1},
        {{0.0f, 0.08f, halfZ + 1.0f}, {0.0f, 0.0f, -1.0f}, "Ray from outside +Z boundary", 2},
        {{0.0f, 0.08f, -halfZ - 1.0f}, {0.0f, 0.0f, 1.0f}, "Ray from outside -Z boundary", 3},
        
        // Diagonal rays towards corners
        {{halfX + 1.0f, 0.08f, halfZ + 1.0f}, {-0.707f, 0.0f, -0.707f}, "Diagonal ray to +X,+Z corner", 4},
        {{-halfX - 1.0f, 0.08f, -halfZ - 1.0f}, {0.707f, 0.0f, 0.707f}, "Diagonal ray to -X,-Z corner", 5},
        
        // Ray from above towards top boundary voxel (aim at middle of top voxel)
        {{0.0f, workspaceSize.y + 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, "Ray from above workspace", 6}
    };
    
    // Get the active grid for face detection
    auto* grid = voxelDataManager->getGrid(resolution);
    ASSERT_NE(grid, nullptr) << "Failed to get voxel grid";
    
    // Test each ray
    for (const auto& rayTest : rayTests) {
        // Normalize direction
        Math::Vector3f normalizedDir = rayTest.direction.normalized();
        
        // Create ray
        VisualFeedback::Ray ray(rayTest.origin, normalizedDir);
        
        // Detect face
        auto result = faceDetector->detectFace(ray, *grid, resolution);
        
        // Verify detection
        EXPECT_TRUE(result.isValid()) 
            << "Failed to detect boundary voxel with " << rayTest.description
            << " from (" << rayTest.origin.x() << ", " << rayTest.origin.y() << ", " << rayTest.origin.z() << ")"
            << " direction (" << normalizedDir.x << ", " << normalizedDir.y << ", " << normalizedDir.z << ")";
        
        if (result.isValid()) {
            auto voxelPos = result.getVoxelPosition();
            
            // Verify it's one of our boundary voxels
            bool foundMatch = false;
            for (const auto& bv : boundaryVoxels) {
                if (voxelPos.x() == bv.pos.x() && 
                    voxelPos.y() == bv.pos.y() && 
                    voxelPos.z() == bv.pos.z()) {
                    foundMatch = true;
                    break;
                }
            }
            
            EXPECT_TRUE(foundMatch) 
                << "Detected voxel at (" << voxelPos.x() << ", " << voxelPos.y() << ", " << voxelPos.z() 
                << ") is not one of our boundary voxels";
        }
    }
}

// Test edge case: ray starting exactly at workspace boundary
TEST_F(FaceDetectionBoundaryTest, RayStartingAtWorkspaceBoundary) {
    auto workspaceSize = voxelDataManager->getWorkspaceSize();
    float halfX = workspaceSize.x * 0.5f;
    
    // Place a voxel just inside the boundary
    auto resolution = VoxelData::VoxelResolution::Size_16cm;
    voxelDataManager->setActiveResolution(resolution);
    
    Math::IncrementCoordinates voxelPos = Math::CoordinateConverter::worldToIncrement(
        Math::WorldCoordinates(halfX - 0.16f, 0.0f, 0.0f)
    );
    
    bool placed = voxelDataManager->setVoxel(voxelPos, resolution, true);
    ASSERT_TRUE(placed) << "Failed to place voxel";
    
    // Create ray starting exactly at workspace boundary
    VisualFeedback::Ray ray(
        Math::WorldCoordinates(halfX, 0.08f, 0.0f),  // Start at boundary, middle of voxel height
        Math::Vector3f(-1.0f, 0.0f, 0.0f)  // Pointing into workspace
    );
    
    // Get the active grid for face detection
    auto* grid = voxelDataManager->getGrid(resolution);
    ASSERT_NE(grid, nullptr) << "Failed to get voxel grid";
    
    auto result = faceDetector->detectFace(ray, *grid, resolution);
    
    EXPECT_TRUE(result.isValid()) 
        << "Failed to detect voxel with ray starting at workspace boundary";
}

// Test multiple voxels near boundaries with complex ray paths
TEST_F(FaceDetectionBoundaryTest, ComplexBoundaryScenario) {
    auto workspaceSize = voxelDataManager->getWorkspaceSize();
    float halfX = workspaceSize.x * 0.5f;
    float halfZ = workspaceSize.z * 0.5f;
    
    // Use smaller voxels for more precise testing
    auto resolution = VoxelData::VoxelResolution::Size_4cm;
    voxelDataManager->setActiveResolution(resolution);
    
    // Create a wall of voxels near the X boundary
    // Extended Z range to ensure ray intersection
    for (int z = -20; z <= 30; z++) {
        for (int y = 0; y <= 5; y++) {
            Math::IncrementCoordinates pos = Math::CoordinateConverter::worldToIncrement(
                Math::WorldCoordinates(halfX - 0.04f, y * 0.04f, z * 0.04f)
            );
            voxelDataManager->setVoxel(pos, resolution, true);
        }
    }
    
    // Test ray that grazes along the boundary
    Math::Vector3f rayDir = Math::Vector3f(-0.1f, 0.0f, 0.9f).normalized();
    VisualFeedback::Ray grazingRay(
        Math::WorldCoordinates(halfX + 0.1f, 0.1f, -0.5f),
        rayDir  // Mostly along Z, slight X component
    );
    
    // Get the active grid for face detection
    auto* grid = voxelDataManager->getGrid(resolution);
    ASSERT_NE(grid, nullptr) << "Failed to get voxel grid";
    
    auto result = faceDetector->detectFace(grazingRay, *grid, resolution);
    
    EXPECT_TRUE(result.isValid()) 
        << "Failed to detect voxel wall with grazing ray along boundary";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}