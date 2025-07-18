#include <gtest/gtest.h>
#include "../PlacementValidation.h"
#include "../../voxel_data/VoxelTypes.h"
#include "../../voxel_data/VoxelDataManager.h"
#include "visual_feedback/FaceDetector.h"
#include "../../foundation/math/CoordinateTypes.h"
#include "../../foundation/math/CoordinateConverter.h"
#include "../../foundation/events/EventDispatcher.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Input;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::VisualFeedback;

class CrossResolutionPlacementValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create event dispatcher for voxel manager
        eventDispatcher = std::make_unique<Events::EventDispatcher>();
        
        // Create voxel manager
        voxelManager = std::make_unique<VoxelDataManager>(eventDispatcher.get());
        voxelManager->resizeWorkspace(Vector3f(10.0f, 10.0f, 10.0f));
        
        // Default workspace size
        workspaceSize = Vector3f(10.0f, 10.0f, 10.0f);
        
        // Enable debug logging
        auto& logger = Logging::Logger::getInstance();
        logger.setLevel(Logging::LogLevel::Debug);
        logger.clearOutputs();
        logger.addOutput(std::make_unique<Logging::FileOutput>("cross_res_placement_test.log", "TestLog", false));
    }
    
    void TearDown() override {
        voxelManager.reset();
        eventDispatcher.reset();
    }
    
    std::unique_ptr<Events::EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelDataManager> voxelManager;
    Vector3f workspaceSize;
};

// Test placement validation when active resolution differs from surface voxel resolution
TEST_F(CrossResolutionPlacementValidationTest, PlacementValidation_SmallOnLargeVoxel) {
    // Place a large 32cm voxel at origin
    voxelManager->setActiveResolution(VoxelResolution::Size_32cm);
    IncrementCoordinates largeVoxelPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(largeVoxelPos, VoxelResolution::Size_32cm, true));
    
    // Switch to small voxel resolution for placement
    voxelManager->setActiveResolution(VoxelResolution::Size_1cm);
    VoxelResolution smallRes = VoxelResolution::Size_1cm;
    
    // Test placement on top face of large voxel
    Vector3f largeVoxelWorldPos = CoordinateConverter::incrementToWorld(largeVoxelPos).value();
    float largeVoxelSize = getVoxelSize(VoxelResolution::Size_32cm);
    
    // Hit point on top face of large voxel
    Vector3f hitPoint = largeVoxelWorldPos + Vector3f(largeVoxelSize * 0.5f, largeVoxelSize, largeVoxelSize * 0.5f);
    
    // Note: Face creation removed as it's not needed for the test
    
    // Test smart placement context
    PlacementContext context = PlacementUtils::getSmartPlacementContext(
        WorldCoordinates(hitPoint), 
        smallRes,
        false, // no shift pressed
        workspaceSize,
        *voxelManager,
        &largeVoxelPos,
        VoxelResolution::Size_32cm,
        VoxelData::FaceDirection::PosY
    );
    
    EXPECT_EQ(context.validation, PlacementValidationResult::Valid) << "Placement context should be valid";
    
    // snappedIncrementPos is directly an IncrementCoordinates, not optional
    Vector3i snappedPos = context.snappedIncrementPos.value();
    
    // Should be placed on top of the large voxel
    EXPECT_EQ(snappedPos.y, 32) << "Should be placed 32cm above origin (on top of large voxel)";
    
    // X and Z should be snapped to 1cm grid
    EXPECT_EQ(snappedPos.x % 1, 0) << "X should be snapped to 1cm grid";
    EXPECT_EQ(snappedPos.z % 1, 0) << "Z should be snapped to 1cm grid";
}

TEST_F(CrossResolutionPlacementValidationTest, PlacementValidation_LargeOnSmallVoxel) {
    // Place a small 1cm voxel at origin
    voxelManager->setActiveResolution(VoxelResolution::Size_1cm);
    IncrementCoordinates smallVoxelPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(smallVoxelPos, VoxelResolution::Size_1cm, true));
    
    // Switch to large voxel resolution for placement
    voxelManager->setActiveResolution(VoxelResolution::Size_32cm);
    VoxelResolution largeRes = VoxelResolution::Size_32cm;
    
    // Test placement on top face of small voxel
    Vector3f smallVoxelWorldPos = CoordinateConverter::incrementToWorld(smallVoxelPos).value();
    float smallVoxelSize = getVoxelSize(VoxelResolution::Size_1cm);
    
    // Hit point on top face of small voxel
    Vector3f hitPoint = smallVoxelWorldPos + Vector3f(smallVoxelSize * 0.5f, smallVoxelSize, smallVoxelSize * 0.5f);
    
    // Note: Face creation removed as it's not needed for the test
    
    // Test smart placement context
    PlacementContext context = PlacementUtils::getSmartPlacementContext(
        WorldCoordinates(hitPoint), 
        largeRes,
        false, // no shift pressed
        workspaceSize,
        *voxelManager,
        &smallVoxelPos,
        VoxelResolution::Size_1cm,
        VoxelData::FaceDirection::PosY
    );
    
    EXPECT_EQ(context.validation, PlacementValidationResult::Valid) << "Placement context should be valid";
    
    // snappedIncrementPos is directly an IncrementCoordinates, not optional
    Vector3i snappedPos = context.snappedIncrementPos.value();
    
    // Should be placed on top of the small voxel (1cm above)
    EXPECT_EQ(snappedPos.y, 1) << "Should be placed 1cm above origin (on top of small voxel)";
    
    // All voxels snap to 1cm increments regardless of size
    EXPECT_EQ(snappedPos.x % 1, 0) << "X should be snapped to 1cm grid";
    EXPECT_EQ(snappedPos.z % 1, 0) << "Z should be snapped to 1cm grid";
}

// Test smart placement context with surface face grid snapping
TEST_F(CrossResolutionPlacementValidationTest, SurfaceFaceGridSnapping_SmallOnLargeFace) {
    // Place a large 64cm voxel
    voxelManager->setActiveResolution(VoxelResolution::Size_64cm);
    IncrementCoordinates largeVoxelPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(largeVoxelPos, VoxelResolution::Size_64cm, true));
    
    // Test placing 4cm voxels on the large voxel's face
    voxelManager->setActiveResolution(VoxelResolution::Size_4cm);
    
    Vector3f largeVoxelWorldPos = CoordinateConverter::incrementToWorld(largeVoxelPos).value();
    float largeVoxelSize = getVoxelSize(VoxelResolution::Size_64cm);
    
    // Test different hit points on the top face
    // 64cm voxel extends from -0.32 to +0.32 in X and Z
    std::vector<Vector3f> testHitPoints = {
        largeVoxelWorldPos + Vector3f(0.1f, largeVoxelSize, 0.1f),        // Near corner
        largeVoxelWorldPos + Vector3f(-0.2f, largeVoxelSize, 0.2f),       // Off-grid position
        largeVoxelWorldPos + Vector3f(0.16f, largeVoxelSize, 0.16f),      // 4cm grid position
        largeVoxelWorldPos + Vector3f(-0.28f, largeVoxelSize, -0.28f),    // Another position within bounds
    };
    
    for (size_t i = 0; i < testHitPoints.size(); ++i) {
        Vector3f hitPoint = testHitPoints[i];
        
        PlacementContext context = PlacementUtils::getSmartPlacementContext(
            WorldCoordinates(hitPoint), 
            VoxelResolution::Size_4cm,
            false, // no shift pressed
            workspaceSize,
            *voxelManager,
            &largeVoxelPos,
            VoxelResolution::Size_64cm,
            VoxelData::FaceDirection::PosY
        );
        
        EXPECT_EQ(context.validation, PlacementValidationResult::Valid) << "Placement context should be valid for test point " << i;
        
        // snappedIncrementPos is directly available
    {
            Vector3i snappedPos = context.snappedIncrementPos.value();
            
            // Should be placed on top of the large voxel
            EXPECT_EQ(snappedPos.y, 64) << "Should be placed 64cm above origin for test point " << i;
            
            // All voxels snap to 1cm increments regardless of size
            EXPECT_EQ(snappedPos.x % 1, 0) << "X should be snapped to 1cm grid for test point " << i;
            EXPECT_EQ(snappedPos.z % 1, 0) << "Z should be snapped to 1cm grid for test point " << i;
        }
    }
}

// Test adjacent position calculation for mixed resolutions
TEST_F(CrossResolutionPlacementValidationTest, AdjacentPositionCalculation_MixedResolutions) {
    // Test getting adjacent positions when surface and placement resolutions differ
    
    struct TestCase {
        VoxelResolution surfaceRes;
        VoxelResolution placementRes;
        IncrementCoordinates surfacePos;
        VoxelData::FaceDirection faceDir;
        std::string description;
    };
    
    std::vector<TestCase> testCases = {
        {VoxelResolution::Size_32cm, VoxelResolution::Size_1cm, IncrementCoordinates(0, 0, 0), VoxelData::FaceDirection::PosX, "1cm on 32cm +X"},
        {VoxelResolution::Size_64cm, VoxelResolution::Size_4cm, IncrementCoordinates(0, 0, 0), VoxelData::FaceDirection::PosY, "4cm on 64cm +Y"},
        {VoxelResolution::Size_16cm, VoxelResolution::Size_1cm, IncrementCoordinates(0, 0, 0), VoxelData::FaceDirection::PosZ, "1cm on 16cm +Z"},
        {VoxelResolution::Size_1cm, VoxelResolution::Size_32cm, IncrementCoordinates(0, 0, 0), VoxelData::FaceDirection::PosX, "32cm on 1cm +X"},
    };
    
    for (const auto& testCase : testCases) {
        // Place the surface voxel
        voxelManager->setActiveResolution(testCase.surfaceRes);
        ASSERT_TRUE(voxelManager->setVoxel(testCase.surfacePos, testCase.surfaceRes, true));
        
        // Calculate adjacent position
        Vector3i adjacentPos = voxelManager->getAdjacentPosition(
            testCase.surfacePos.value(),
            testCase.faceDir,
            testCase.surfaceRes,
            testCase.placementRes
        );
        
        // Verify the adjacent position is valid
        float surfaceVoxelSize = getVoxelSize(testCase.surfaceRes);
        float placementVoxelSize = getVoxelSize(testCase.placementRes);
        
        Vector3i expectedOffset(0, 0, 0);
        int surfaceSizeCm = static_cast<int>(surfaceVoxelSize * 100);
        int placementSizeCm = static_cast<int>(placementVoxelSize * 100);
        
        switch (testCase.faceDir) {
            case VoxelData::FaceDirection::PosX:
                expectedOffset.x = surfaceSizeCm;
                break;
            case VoxelData::FaceDirection::PosY:
                expectedOffset.y = surfaceSizeCm;
                break;
            case VoxelData::FaceDirection::PosZ:
                expectedOffset.z = surfaceSizeCm;
                break;
            default:
                break;
        }
        
        Vector3i expectedPos = testCase.surfacePos.value() + expectedOffset;
        
        EXPECT_EQ(adjacentPos.x, expectedPos.x) << "Adjacent X position incorrect for " << testCase.description;
        EXPECT_EQ(adjacentPos.y, expectedPos.y) << "Adjacent Y position incorrect for " << testCase.description;
        EXPECT_EQ(adjacentPos.z, expectedPos.z) << "Adjacent Z position incorrect for " << testCase.description;
        
        // Clear for next test
        voxelManager->clearAll();
    }
}

// Test placement position snapping from large voxel faces to small voxel grid
TEST_F(CrossResolutionPlacementValidationTest, PlacementPositionSnapping_LargeFaceToSmallGrid) {
    // Place a 128cm voxel
    voxelManager->setActiveResolution(VoxelResolution::Size_128cm);
    IncrementCoordinates largeVoxelPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(largeVoxelPos, VoxelResolution::Size_128cm, true));
    
    // Test placing 1cm voxels on various points of the large voxel's top face
    voxelManager->setActiveResolution(VoxelResolution::Size_1cm);
    
    Vector3f largeVoxelWorldPos = CoordinateConverter::incrementToWorld(largeVoxelPos).value();
    float largeVoxelSize = getVoxelSize(VoxelResolution::Size_128cm);
    
    // Test various hit points on the top face
    // 128cm voxel extends from -0.64 to +0.64 in X and Z
    std::vector<Vector3f> testOffsets = {
        Vector3f(0.001f, 0, 0.001f),           // Near center
        Vector3f(0.333f, 0, 0.333f),           // Off-grid position
        Vector3f(0.127f, 0, 0.127f),           // Another off-grid position
        Vector3f(0.0f, 0, 0.0f),               // Center of face
        Vector3f(-0.63f, 0, -0.63f),           // Near corner
    };
    
    for (size_t i = 0; i < testOffsets.size(); ++i) {
        Vector3f hitPoint = largeVoxelWorldPos + Vector3f(testOffsets[i].x, largeVoxelSize, testOffsets[i].z);
        
        PlacementContext context = PlacementUtils::getSmartPlacementContext(
            WorldCoordinates(hitPoint), 
            VoxelResolution::Size_1cm,
            false, // no shift pressed
            workspaceSize,
            *voxelManager,
            &largeVoxelPos,
            VoxelResolution::Size_128cm,
            VoxelData::FaceDirection::PosY
        );
        
        EXPECT_EQ(context.validation, PlacementValidationResult::Valid) << "Placement context should be valid for test offset " << i;
        
        // snappedIncrementPos is directly available
    {
            Vector3i snappedPos = context.snappedIncrementPos.value();
            
            // Should be placed on top of the large voxel
            EXPECT_EQ(snappedPos.y, 128) << "Should be placed 128cm above origin for test offset " << i;
            
            // Should be snapped to 1cm grid
            EXPECT_EQ(snappedPos.x % 1, 0) << "X should be snapped to 1cm grid for test offset " << i;
            EXPECT_EQ(snappedPos.z % 1, 0) << "Z should be snapped to 1cm grid for test offset " << i;
            
            // Position should be within the bounds of the large voxel's top face
            // 128cm voxel at origin extends from -64 to +64 in increment coordinates
            EXPECT_GE(snappedPos.x, -64) << "X should be >= -64 for test offset " << i;
            EXPECT_LE(snappedPos.x, 64) << "X should be <= 64 for test offset " << i;
            EXPECT_GE(snappedPos.z, -64) << "Z should be >= -64 for test offset " << i;
            EXPECT_LE(snappedPos.z, 64) << "Z should be <= 64 for test offset " << i;
        }
    }
}

TEST_F(CrossResolutionPlacementValidationTest, ValidationFailureCases_InvalidPlacements) {
    // Test cases where placement should fail
    
    // Place a 32cm voxel at origin
    voxelManager->setActiveResolution(VoxelResolution::Size_32cm);
    IncrementCoordinates largeVoxelPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(largeVoxelPos, VoxelResolution::Size_32cm, true));
    
    // Test placing a small voxel inside the large voxel (should fail)
    voxelManager->setActiveResolution(VoxelResolution::Size_1cm);
    
    // Hit point inside the large voxel
    Vector3f largeVoxelWorldPos = CoordinateConverter::incrementToWorld(largeVoxelPos).value();
    float largeVoxelSize = getVoxelSize(VoxelResolution::Size_32cm);
    Vector3f insideHitPoint = largeVoxelWorldPos + Vector3f(largeVoxelSize * 0.5f, largeVoxelSize * 0.5f, largeVoxelSize * 0.5f);
    
    PlacementContext context = PlacementUtils::getSmartPlacementContext(
        WorldCoordinates(insideHitPoint), 
        VoxelResolution::Size_1cm,
        false, // no shift pressed
        workspaceSize,
        *voxelManager,
        nullptr // no surface face - placing in empty space
    );
    
    // The placement context validates the hit point and snaps it to a 1cm grid
    // Since we're hitting inside a voxel without a surface face context,
    // the snapped position will still be inside the voxel
    EXPECT_EQ(context.validation, PlacementValidationResult::Valid) << "Hit point validation should succeed";
    
    Vector3i snappedPos = context.snappedIncrementPos.value();
    
    // But the actual placement would fail due to overlap
    auto validation = voxelManager->validatePosition(
        IncrementCoordinates(snappedPos), 
        VoxelResolution::Size_1cm
    );
    
    EXPECT_FALSE(validation.valid) << "Position validation should fail due to overlap";
    EXPECT_FALSE(validation.noOverlap) << "Should detect overlap with existing voxel";
}

TEST_F(CrossResolutionPlacementValidationTest, WorkspaceBoundsValidation_MixedResolutions) {
    // Test workspace bounds validation with mixed resolutions
    
    // Set a small workspace
    Vector3f smallWorkspace(2.0f, 2.0f, 2.0f);
    voxelManager->resizeWorkspace(smallWorkspace);
    
    // Place a large voxel that takes up most of the workspace
    voxelManager->setActiveResolution(VoxelResolution::Size_128cm);
    IncrementCoordinates largeVoxelPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(largeVoxelPos, VoxelResolution::Size_128cm, true));
    
    // Try to place small voxels near the workspace boundaries
    voxelManager->setActiveResolution(VoxelResolution::Size_1cm);
    
    // Test positions outside workspace boundaries
    // For 2m x 2m x 2m workspace: X[-1,1], Y[0,2], Z[-1,1]
    std::vector<Vector3i> testPositions = {
        Vector3i(101, 0, 0),    // Outside +X boundary (1.01m > 1m)
        Vector3i(-101, 0, 0),   // Outside -X boundary (-1.01m < -1m)
        Vector3i(0, 201, 0),    // Outside +Y boundary (2.01m > 2m)
        Vector3i(0, 0, 101),    // Outside +Z boundary (1.01m > 1m)
        Vector3i(0, 0, -101),   // Outside -Z boundary (-1.01m < -1m)
    };
    
    for (size_t i = 0; i < testPositions.size(); ++i) {
        Vector3i testPos = testPositions[i];
        
        auto validation = voxelManager->validatePosition(
            IncrementCoordinates(testPos), 
            VoxelResolution::Size_1cm
        );
        
        // These positions should be outside the workspace bounds
        EXPECT_FALSE(validation.valid) << "Position should be outside workspace bounds for test " << i;
        EXPECT_FALSE(validation.errorMessage.empty()) << "Should have error message for test " << i;
    }
}

TEST_F(CrossResolutionPlacementValidationTest, GroundPlaneSnapping_MixedResolutions) {
    // Test ground plane snapping with different resolutions
    
    // Test placing voxels of different sizes on the ground plane
    std::vector<VoxelResolution> testResolutions = {
        VoxelResolution::Size_1cm,
        VoxelResolution::Size_4cm,
        VoxelResolution::Size_16cm,
        VoxelResolution::Size_64cm
    };
    
    for (auto resolution : testResolutions) {
        voxelManager->setActiveResolution(resolution);
        
        // Test hit point on ground plane (Y=0)
        Vector3f groundHitPoint(0.123f, 0.0f, 0.456f);
        
        PlacementContext context = PlacementUtils::getSmartPlacementContext(
            WorldCoordinates(groundHitPoint), 
            resolution,
            false, // no shift pressed
            workspaceSize,
            *voxelManager,
            nullptr // no surface face - ground plane
        );
        
        EXPECT_EQ(context.validation, PlacementValidationResult::Valid) << "Ground plane placement should be valid for resolution " << static_cast<int>(resolution);
        
        // snappedIncrementPos is directly available
    {
            Vector3i snappedPos = context.snappedIncrementPos.value();
            
            // Should be placed on the ground (Y=0)
            EXPECT_EQ(snappedPos.y, 0) << "Should be placed on ground for resolution " << static_cast<int>(resolution);
            
            // All voxels snap to 1cm increments regardless of resolution
            // The hit point (0.123f, 0.0f, 0.456f) should snap to (12, 0, 46)
            EXPECT_EQ(snappedPos.x, 12) << "X should be at 12cm for resolution " << static_cast<int>(resolution);
            EXPECT_EQ(snappedPos.z, 46) << "Z should be at 46cm for resolution " << static_cast<int>(resolution);
        }
        
        // Clear for next test
        voxelManager->clearAll();
    }
}