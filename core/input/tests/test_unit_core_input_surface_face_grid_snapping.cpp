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

class SurfaceFaceGridSnappingTest : public ::testing::Test {
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
        logger.addOutput(std::make_unique<Logging::FileOutput>("surface_face_grid_test.log", "TestLog", false));
    }
    
    void TearDown() override {
        voxelManager.reset();
        eventDispatcher.reset();
    }
    
    std::unique_ptr<Events::EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelDataManager> voxelManager;
    Vector3f workspaceSize;
};

// Test snapping to surface voxel grid when placing smaller voxels
TEST_F(SurfaceFaceGridSnappingTest, SnappingToSurfaceGrid_SmallOnLargeTopFace) {
    // Place a large 64cm voxel at origin
    voxelManager->setActiveResolution(VoxelResolution::Size_64cm);
    IncrementCoordinates largeVoxelPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(largeVoxelPos, VoxelResolution::Size_64cm, true));
    
    // Test placing 4cm voxels on the top face with various hit points
    voxelManager->setActiveResolution(VoxelResolution::Size_4cm);
    
    Vector3f largeVoxelWorldPos = CoordinateConverter::incrementToWorld(largeVoxelPos).value();
    float largeVoxelSize = getVoxelSize(VoxelResolution::Size_64cm);
    
    // Test various hit points on the top face
    struct SnapTest {
        Vector3f hitOffset;  // Offset from large voxel origin on top face
        Vector3i expectedSnap;  // Expected snapped position in increment coordinates
        std::string description;
    };
    
    std::vector<SnapTest> snapTests = {
        // Corner positions - snap to 4cm grid
        // 64cm voxel extends from -0.32 to +0.32 in X and Z
        // Grid origin is at (-32, 64, -32) for top face
        {Vector3f(-0.319f, largeVoxelSize, -0.319f), Vector3i(-32, 64, -32), "Near corner (-32,-32)"},
        {Vector3f(0.319f, largeVoxelSize, -0.319f), Vector3i(32, 64, -32), "Near corner (32,-32)"},
        {Vector3f(-0.319f, largeVoxelSize, 0.319f), Vector3i(-32, 64, 32), "Near corner (-32,32)"},
        {Vector3f(0.319f, largeVoxelSize, 0.319f), Vector3i(32, 64, 32), "Near corner (32,32)"},
        
        // Grid-aligned positions - already on 4cm grid
        {Vector3f(0.04f, largeVoxelSize, 0.04f), Vector3i(4, 64, 4), "4cm grid position"},
        {Vector3f(0.16f, largeVoxelSize, 0.16f), Vector3i(16, 64, 16), "16cm grid position"},
        {Vector3f(-0.16f, largeVoxelSize, -0.16f), Vector3i(-16, 64, -16), "Negative 16cm grid position"},
        {Vector3f(0.0f, largeVoxelSize, 0.0f), Vector3i(0, 64, 0), "Center position"},
        
        // Off-grid positions that should snap to nearest 4cm grid
        {Vector3f(0.033f, largeVoxelSize, 0.033f), Vector3i(4, 64, 4), "Off-grid near 3cm -> 4cm"},
        {Vector3f(0.157f, largeVoxelSize, 0.157f), Vector3i(16, 64, 16), "Off-grid near 16cm"},
        {Vector3f(-0.243f, largeVoxelSize, -0.243f), Vector3i(-24, 64, -24), "Off-grid near -24cm"},
        {Vector3f(0.285f, largeVoxelSize, 0.285f), Vector3i(28, 64, 28), "Off-grid near 29cm -> 28cm"},
        
        // Various positions within bounds - snap to 4cm grid
        {Vector3f(0.0f, largeVoxelSize, 0.0f), Vector3i(0, 64, 0), "Center of face"},
        {Vector3f(0.1f, largeVoxelSize, -0.2f), Vector3i(12, 64, -20), "Asymmetric position"},
        {Vector3f(-0.15f, largeVoxelSize, 0.25f), Vector3i(-16, 64, 24), "Another asymmetric position"},
    };
    
    for (const auto& test : snapTests) {
        Vector3f hitPoint = largeVoxelWorldPos + test.hitOffset;
        
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
        
        EXPECT_EQ(context.validation, PlacementValidationResult::Valid) << "Placement context should be valid for " << test.description;
        // snappedIncrementPos is always available << "Should have snapped position for " << test.description;
        
        // snappedIncrementPos is directly available
        {
            Vector3i snappedPos = context.snappedIncrementPos.value();
            
            EXPECT_EQ(snappedPos.x, test.expectedSnap.x) << "X snap incorrect for " << test.description;
            EXPECT_EQ(snappedPos.y, test.expectedSnap.y) << "Y snap incorrect for " << test.description;
            EXPECT_EQ(snappedPos.z, test.expectedSnap.z) << "Z snap incorrect for " << test.description;
        }
    }
}

TEST_F(SurfaceFaceGridSnappingTest, SnappingToSurfaceGrid_SmallOnLargeSideFace) {
    // Place a large 128cm voxel at origin
    voxelManager->setActiveResolution(VoxelResolution::Size_128cm);
    IncrementCoordinates largeVoxelPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(largeVoxelPos, VoxelResolution::Size_128cm, true));
    
    // Test placing 8cm voxels on the right side face (+X face)
    voxelManager->setActiveResolution(VoxelResolution::Size_8cm);
    
    Vector3f largeVoxelWorldPos = CoordinateConverter::incrementToWorld(largeVoxelPos).value();
    float largeVoxelSize = getVoxelSize(VoxelResolution::Size_128cm);
    
    // Test various hit points on the right side face
    struct SnapTest {
        Vector3f hitOffset;  // Offset from large voxel origin on right face
        Vector3i expectedSnap;  // Expected snapped position in increment coordinates
        std::string description;
    };
    
    std::vector<SnapTest> snapTests = {
        // Edge positions
        // 128cm voxel at origin has bottom at Y=0, extends to Y=1.28
        // In Z direction, extends from -0.64 to +0.64
        // Right face is at X = 64cm, voxel center will be at 68cm (64 + 4cm half-size)
        {Vector3f(largeVoxelSize, 0.001f, -0.639f), Vector3i(68, 0, -64), "Bottom-front edge"},
        {Vector3f(largeVoxelSize, 1.279f, -0.639f), Vector3i(68, 128, -64), "Top-front edge"},
        {Vector3f(largeVoxelSize, 0.001f, 0.639f), Vector3i(68, 0, 64), "Bottom-back edge"},
        {Vector3f(largeVoxelSize, 1.279f, 0.639f), Vector3i(68, 128, 64), "Top-back edge"},
        
        // Grid-aligned positions
        {Vector3f(largeVoxelSize, 0.08f, 0.08f), Vector3i(68, 8, 8), "8cm grid position"},
        {Vector3f(largeVoxelSize, 0.32f, 0.32f), Vector3i(68, 32, 32), "32cm grid position"},
        {Vector3f(largeVoxelSize, 0.64f, 0.0f), Vector3i(68, 64, 0), "64cm grid position"},
        {Vector3f(largeVoxelSize, 1.12f, -0.48f), Vector3i(68, 112, -48), "112cm grid position"},
        
        // Off-grid positions that should snap to 8cm grid
        {Vector3f(largeVoxelSize, 0.077f, 0.077f), Vector3i(68, 8, 8), "Off-grid near 8cm"},
        {Vector3f(largeVoxelSize, 0.323f, 0.323f), Vector3i(68, 32, 32), "Off-grid near 32cm"},
        {Vector3f(largeVoxelSize, 0.643f, -0.357f), Vector3i(68, 64, -32), "Off-grid near 64cm -> snap to -32"},
        
        // Center and various positions
        {Vector3f(largeVoxelSize, 0.64f, 0.0f), Vector3i(68, 64, 0), "Center of face"},
        {Vector3f(largeVoxelSize, 0.24f, -0.12f), Vector3i(68, 24, -8), "Asymmetric position -> snap to grid"},
    };
    
    for (const auto& test : snapTests) {
        Vector3f hitPoint = largeVoxelWorldPos + test.hitOffset;
        
        PlacementContext context = PlacementUtils::getSmartPlacementContext(
            WorldCoordinates(hitPoint), 
            VoxelResolution::Size_8cm,
            false, // no shift pressed
            workspaceSize,
            *voxelManager,
            &largeVoxelPos,
            VoxelResolution::Size_128cm,
            VoxelData::FaceDirection::PosX
        );
        
        EXPECT_EQ(context.validation, PlacementValidationResult::Valid) << "Placement context should be valid for " << test.description;
        // snappedIncrementPos is always available << "Should have snapped position for " << test.description;
        
        // snappedIncrementPos is directly available
        {
            Vector3i snappedPos = context.snappedIncrementPos.value();
            
            EXPECT_EQ(snappedPos.x, test.expectedSnap.x) << "X snap incorrect for " << test.description;
            EXPECT_EQ(snappedPos.y, test.expectedSnap.y) << "Y snap incorrect for " << test.description;
            EXPECT_EQ(snappedPos.z, test.expectedSnap.z) << "Z snap incorrect for " << test.description;
        }
    }
}

// Test grid alignment calculation for different resolution combinations
TEST_F(SurfaceFaceGridSnappingTest, GridAlignmentCalculation_DifferentResolutionCombinations) {
    struct ResolutionTest {
        VoxelResolution surfaceRes;
        VoxelResolution placementRes;
        int expectedGridSize;  // Expected grid size in cm
        std::string description;
    };
    
    std::vector<ResolutionTest> resolutionTests = {
        {VoxelResolution::Size_32cm, VoxelResolution::Size_1cm, 1, "1cm on 32cm"},
        {VoxelResolution::Size_64cm, VoxelResolution::Size_2cm, 2, "2cm on 64cm"},
        {VoxelResolution::Size_128cm, VoxelResolution::Size_4cm, 4, "4cm on 128cm"},
        {VoxelResolution::Size_256cm, VoxelResolution::Size_8cm, 8, "8cm on 256cm"},
        {VoxelResolution::Size_64cm, VoxelResolution::Size_16cm, 16, "16cm on 64cm"},
        {VoxelResolution::Size_128cm, VoxelResolution::Size_32cm, 32, "32cm on 128cm"},
    };
    
    for (const auto& resTest : resolutionTests) {
        // Place surface voxel
        voxelManager->setActiveResolution(resTest.surfaceRes);
        IncrementCoordinates surfaceVoxelPos(0, 0, 0);
        ASSERT_TRUE(voxelManager->setVoxel(surfaceVoxelPos, resTest.surfaceRes, true));
        
        // Test placement on top face
        voxelManager->setActiveResolution(resTest.placementRes);
        
        Vector3f surfaceVoxelWorldPos = CoordinateConverter::incrementToWorld(surfaceVoxelPos).value();
        float surfaceVoxelSize = getVoxelSize(resTest.surfaceRes);
        float placementVoxelSize = getVoxelSize(resTest.placementRes);
        
        // Test hit point at center of top face
        Vector3f hitPoint = surfaceVoxelWorldPos + Vector3f(surfaceVoxelSize * 0.5f, surfaceVoxelSize, surfaceVoxelSize * 0.5f);
        
        PlacementContext context = PlacementUtils::getSmartPlacementContext(
            WorldCoordinates(hitPoint), 
            resTest.placementRes,
            false, // no shift pressed
            workspaceSize,
            *voxelManager,
            &surfaceVoxelPos,
            resTest.surfaceRes,
            VoxelData::FaceDirection::PosY
        );
        
        EXPECT_EQ(context.validation, PlacementValidationResult::Valid) << "Placement context should be valid for " << resTest.description;
        // snappedIncrementPos is always available << "Should have snapped position for " << resTest.description;
        
        // snappedIncrementPos is directly available
        {
            Vector3i snappedPos = context.snappedIncrementPos.value();
            
            // Should be placed on top of the surface voxel
            int expectedY = static_cast<int>(surfaceVoxelSize * 100); // Convert to cm
            EXPECT_EQ(snappedPos.y, expectedY) << "Y position incorrect for " << resTest.description;
            
            // Voxels snap to their own grid size (not 1cm) without shift
            EXPECT_EQ(snappedPos.x % resTest.expectedGridSize, 0) << "X should be snapped to " << resTest.expectedGridSize << "cm grid for " << resTest.description;
            EXPECT_EQ(snappedPos.z % resTest.expectedGridSize, 0) << "Z should be snapped to " << resTest.expectedGridSize << "cm grid for " << resTest.description;
        }
        
        // Clear for next test
        voxelManager->clearAll();
    }
}

// Test surface face coordinate system mapping
TEST_F(SurfaceFaceGridSnappingTest, SurfaceFaceCoordinateMapping_AllFaces) {
    // Place a 64cm voxel at origin
    voxelManager->setActiveResolution(VoxelResolution::Size_64cm);
    IncrementCoordinates surfaceVoxelPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(surfaceVoxelPos, VoxelResolution::Size_64cm, true));
    
    // Test placing 4cm voxels on all faces
    voxelManager->setActiveResolution(VoxelResolution::Size_4cm);
    
    Vector3f surfaceVoxelWorldPos = CoordinateConverter::incrementToWorld(surfaceVoxelPos).value();
    float surfaceVoxelSize = getVoxelSize(VoxelResolution::Size_64cm);
    
    struct FaceTest {
        VoxelData::FaceDirection faceDirection;
        Vector3f hitOffset;
        Vector3i expectedSnap;
        std::string description;
    };
    
    std::vector<FaceTest> faceTests = {
        // Top face (+Y)
        {VoxelData::FaceDirection::PosY, Vector3f(0.0f, surfaceVoxelSize, 0.0f), Vector3i(0, 64, 0), "Top face center"},
        
        // Bottom face (-Y) - Skip this test as it would place below ground (Y<0)
        // {VoxelData::FaceDirection::NegY, Vector3f(0.0f, -0.04f, 0.0f), Vector3i(0, -4, 0), "Bottom face center"},
        
        // Right face (+X) - 64cm voxel extends from -0.32 to +0.32
        // Right face is at X = 32cm, 4cm voxel center will be at 34cm (32 + 2cm half-size)
        {VoxelData::FaceDirection::PosX, Vector3f(surfaceVoxelSize, 0.32f, 0.0f), Vector3i(34, 32, 0), "Right face center"},
        
        // Left face (-X)
        // Left face is at X = -32cm, 4cm voxel center will be at -34cm (-32 - 2cm half-size)
        {VoxelData::FaceDirection::NegX, Vector3f(-surfaceVoxelSize, 0.32f, 0.0f), Vector3i(-34, 32, 0), "Left face center"},
        
        // Front face (-Z)
        // Front face is at Z = -32cm, 4cm voxel center will be at -34cm (-32 - 2cm half-size)
        {VoxelData::FaceDirection::NegZ, Vector3f(0.0f, 0.32f, -surfaceVoxelSize), Vector3i(0, 32, -34), "Front face center"},
        
        // Back face (+Z)
        // Back face is at Z = 32cm, 4cm voxel center will be at 34cm (32 + 2cm half-size)
        {VoxelData::FaceDirection::PosZ, Vector3f(0.0f, 0.32f, surfaceVoxelSize), Vector3i(0, 32, 34), "Back face center"},
    };
    
    for (const auto& test : faceTests) {
        Vector3f hitPoint = surfaceVoxelWorldPos + test.hitOffset;
        
        PlacementContext context = PlacementUtils::getSmartPlacementContext(
            WorldCoordinates(hitPoint), 
            VoxelResolution::Size_4cm,
            false, // no shift pressed
            workspaceSize,
            *voxelManager,
            &surfaceVoxelPos,
            VoxelResolution::Size_64cm,
            test.faceDirection
        );
        
        EXPECT_EQ(context.validation, PlacementValidationResult::Valid) << "Placement context should be valid for " << test.description;
        // snappedIncrementPos is always available << "Should have snapped position for " << test.description;
        
        // snappedIncrementPos is directly available
        {
            Vector3i snappedPos = context.snappedIncrementPos.value();
            
            EXPECT_EQ(snappedPos.x, test.expectedSnap.x) << "X snap incorrect for " << test.description;
            EXPECT_EQ(snappedPos.y, test.expectedSnap.y) << "Y snap incorrect for " << test.description;
            EXPECT_EQ(snappedPos.z, test.expectedSnap.z) << "Z snap incorrect for " << test.description;
        }
    }
}

// Test edge case positioning (corners, edges of large voxels)
TEST_F(SurfaceFaceGridSnappingTest, EdgeCasePositioning_CornersAndEdges) {
    // Place a 64cm voxel at origin
    voxelManager->setActiveResolution(VoxelResolution::Size_64cm);
    IncrementCoordinates surfaceVoxelPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(surfaceVoxelPos, VoxelResolution::Size_64cm, true));
    
    // Test placing 2cm voxels at corners and edges
    voxelManager->setActiveResolution(VoxelResolution::Size_2cm);
    
    Vector3f surfaceVoxelWorldPos = CoordinateConverter::incrementToWorld(surfaceVoxelPos).value();
    float surfaceVoxelSize = getVoxelSize(VoxelResolution::Size_64cm);
    
    struct EdgeTest {
        Vector3f hitOffset;
        Vector3i expectedSnap;
        std::string description;
    };
    
    std::vector<EdgeTest> edgeTests = {
        // Corner positions on top face
        // 64cm voxel extends from -0.32 to +0.32 in X and Z
        {Vector3f(-0.319f, surfaceVoxelSize, -0.319f), Vector3i(-32, 64, -32), "Top face corner (-32,-32)"},
        {Vector3f(0.319f, surfaceVoxelSize, -0.319f), Vector3i(32, 64, -32), "Top face corner (32,-32)"},
        {Vector3f(-0.319f, surfaceVoxelSize, 0.319f), Vector3i(-32, 64, 32), "Top face corner (-32,32)"},
        {Vector3f(0.319f, surfaceVoxelSize, 0.319f), Vector3i(32, 64, 32), "Top face corner (32,32)"},
        
        // Edge positions on top face
        {Vector3f(0.0f, surfaceVoxelSize, -0.319f), Vector3i(0, 64, -32), "Top face front edge center"},
        {Vector3f(0.0f, surfaceVoxelSize, 0.319f), Vector3i(0, 64, 32), "Top face back edge center"},
        {Vector3f(-0.319f, surfaceVoxelSize, 0.0f), Vector3i(-32, 64, 0), "Top face left edge center"},
        {Vector3f(0.319f, surfaceVoxelSize, 0.0f), Vector3i(32, 64, 0), "Top face right edge center"},
        
        // Positions very close to edges (should snap to nearest 2cm grid)
        {Vector3f(0.011f, surfaceVoxelSize, 0.011f), Vector3i(2, 64, 2), "Near corner, should snap to 2cm grid"},
        {Vector3f(0.021f, surfaceVoxelSize, 0.021f), Vector3i(2, 64, 2), "Near corner, should snap to 2cm grid"},
        {Vector3f(0.031f, surfaceVoxelSize, 0.031f), Vector3i(4, 64, 4), "Near corner, should snap to 4cm grid"},
        
        // Positions at exact grid boundaries
        {Vector3f(0.02f, surfaceVoxelSize, 0.02f), Vector3i(2, 64, 2), "Exact 2cm grid position"},
        {Vector3f(0.04f, surfaceVoxelSize, 0.04f), Vector3i(4, 64, 4), "Exact 4cm grid position"},
        {Vector3f(0.06f, surfaceVoxelSize, 0.06f), Vector3i(6, 64, 6), "Exact 6cm grid position"},
        
        // Off-center positions (within bounds) - snap to 2cm grid
        {Vector3f(0.123f, surfaceVoxelSize, -0.156f), Vector3i(12, 64, -16), "Off-center position 1"},
        {Vector3f(-0.255f, surfaceVoxelSize, 0.222f), Vector3i(-26, 64, 22), "Off-center position 2"},
    };
    
    for (const auto& test : edgeTests) {
        Vector3f hitPoint = surfaceVoxelWorldPos + test.hitOffset;
        
        PlacementContext context = PlacementUtils::getSmartPlacementContext(
            WorldCoordinates(hitPoint), 
            VoxelResolution::Size_2cm,
            false, // no shift pressed
            workspaceSize,
            *voxelManager,
            &surfaceVoxelPos,
            VoxelResolution::Size_64cm,
            VoxelData::FaceDirection::PosY
        );
        
        EXPECT_EQ(context.validation, PlacementValidationResult::Valid) << "Placement context should be valid for " << test.description;
        // snappedIncrementPos is always available << "Should have snapped position for " << test.description;
        
        // snappedIncrementPos is directly available
        {
            Vector3i snappedPos = context.snappedIncrementPos.value();
            
            EXPECT_EQ(snappedPos.x, test.expectedSnap.x) << "X snap incorrect for " << test.description;
            EXPECT_EQ(snappedPos.y, test.expectedSnap.y) << "Y snap incorrect for " << test.description;
            EXPECT_EQ(snappedPos.z, test.expectedSnap.z) << "Z snap incorrect for " << test.description;
        }
    }
}

TEST_F(SurfaceFaceGridSnappingTest, SnapValidation_OutOfBounds) {
    // Test snapping behavior when hit points are outside the surface voxel bounds
    
    // Place a 32cm voxel at origin
    voxelManager->setActiveResolution(VoxelResolution::Size_32cm);
    IncrementCoordinates surfaceVoxelPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(surfaceVoxelPos, VoxelResolution::Size_32cm, true));
    
    // Test placing 1cm voxels with hit points outside the surface bounds
    voxelManager->setActiveResolution(VoxelResolution::Size_1cm);
    
    Vector3f surfaceVoxelWorldPos = CoordinateConverter::incrementToWorld(surfaceVoxelPos).value();
    float surfaceVoxelSize = getVoxelSize(VoxelResolution::Size_32cm);
    
    struct OutOfBoundsTest {
        Vector3f hitOffset;
        bool shouldBeValid;
        std::string description;
    };
    
    std::vector<OutOfBoundsTest> outOfBoundsTests = {
        // Hit points outside the surface voxel bounds on top face
        // 32cm voxel at origin extends from -0.16 to +0.16 in X and Z
        {Vector3f(-0.17f, surfaceVoxelSize, 0.0f), false, "Hit point outside -X bound"},
        {Vector3f(0.17f, surfaceVoxelSize, 0.0f), false, "Hit point outside +X bound"},
        {Vector3f(0.0f, surfaceVoxelSize, -0.17f), false, "Hit point outside -Z bound"},
        {Vector3f(0.0f, surfaceVoxelSize, 0.17f), false, "Hit point outside +Z bound"},
        
        // Hit points just inside the surface voxel bounds
        {Vector3f(-0.159f, surfaceVoxelSize, -0.159f), true, "Hit point just inside corner (-X,-Z)"},
        {Vector3f(0.159f, surfaceVoxelSize, 0.159f), true, "Hit point just inside opposite corner (+X,+Z)"},
        {Vector3f(0.0f, surfaceVoxelSize, 0.0f), true, "Hit point at center"},
        
        // Hit points exactly at the surface voxel bounds
        {Vector3f(-0.16f, surfaceVoxelSize, -0.16f), true, "Hit point at exact corner (-X,-Z)"},
        {Vector3f(0.16f, surfaceVoxelSize, 0.16f), true, "Hit point at exact opposite corner (+X,+Z)"},
    };
    
    for (const auto& test : outOfBoundsTests) {
        Vector3f hitPoint = surfaceVoxelWorldPos + test.hitOffset;
        
        PlacementContext context = PlacementUtils::getSmartPlacementContext(
            WorldCoordinates(hitPoint), 
            VoxelResolution::Size_1cm,
            false, // no shift pressed
            workspaceSize,
            *voxelManager,
            &surfaceVoxelPos,
            VoxelResolution::Size_32cm,
            VoxelData::FaceDirection::PosY
        );
        
        // Debug output
        std::cout << "Test: " << test.description 
                  << " hitOffset=(" << test.hitOffset.x << "," << test.hitOffset.y << "," << test.hitOffset.z << ")"
                  << " hitPoint=(" << hitPoint.x << "," << hitPoint.y << "," << hitPoint.z << ")"
                  << " valid=" << (context.validation == PlacementValidationResult::Valid)
                  << " expected=" << test.shouldBeValid << std::endl;
        
        EXPECT_EQ(context.validation == PlacementValidationResult::Valid, test.shouldBeValid) << "Validity incorrect for " << test.description;
        
        if (test.shouldBeValid) {
            // snappedIncrementPos is always available << "Should have snapped position for " << test.description;
            
            // snappedIncrementPos is directly available
        {
                Vector3i snappedPos = context.snappedIncrementPos.value();
                
                // Should be placed on top of the surface voxel
                EXPECT_EQ(snappedPos.y, 32) << "Y position incorrect for " << test.description;
                
                // With no shift, 1cm voxels snap to their own 1cm grid
                EXPECT_EQ(snappedPos.x % 1, 0) << "X should be snapped to 1cm grid for " << test.description;
                EXPECT_EQ(snappedPos.z % 1, 0) << "Z should be snapped to 1cm grid for " << test.description;
            }
        }
    }
}

TEST_F(SurfaceFaceGridSnappingTest, SnapConsistency_RepeatedSnapping) {
    // Test that snapping is consistent when called multiple times with the same input
    
    // Place a 64cm voxel at origin
    voxelManager->setActiveResolution(VoxelResolution::Size_64cm);
    IncrementCoordinates surfaceVoxelPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(surfaceVoxelPos, VoxelResolution::Size_64cm, true));
    
    // Test placing 4cm voxels
    voxelManager->setActiveResolution(VoxelResolution::Size_4cm);
    
    Vector3f surfaceVoxelWorldPos = CoordinateConverter::incrementToWorld(surfaceVoxelPos).value();
    float surfaceVoxelSize = getVoxelSize(VoxelResolution::Size_64cm);
    
    // Test hit point (within bounds of 64cm voxel: -0.32 to +0.32)
    Vector3f hitPoint = surfaceVoxelWorldPos + Vector3f(0.233f, surfaceVoxelSize, -0.155f);
    
    // Call snapping multiple times and verify consistency
    std::vector<Vector3i> snapResults;
    
    for (int i = 0; i < 10; ++i) {
        PlacementContext context = PlacementUtils::getSmartPlacementContext(
            WorldCoordinates(hitPoint), 
            VoxelResolution::Size_4cm,
            false, // no shift pressed
            workspaceSize,
            *voxelManager,
            &surfaceVoxelPos,
            VoxelResolution::Size_64cm,
            VoxelData::FaceDirection::PosY
        );
        
        EXPECT_EQ(context.validation, PlacementValidationResult::Valid) << "Placement context should be valid for iteration " << i;
        // snappedIncrementPos is always available << "Should have snapped position for iteration " << i;
        
        // snappedIncrementPos is directly available
        {
            Vector3i snappedPos = context.snappedIncrementPos.value();
            snapResults.push_back(snappedPos);
        }
    }
    
    // Verify all results are the same
    EXPECT_EQ(snapResults.size(), 10u) << "Should have 10 snap results";
    
    if (!snapResults.empty()) {
        Vector3i firstResult = snapResults[0];
        for (size_t i = 1; i < snapResults.size(); ++i) {
            EXPECT_EQ(snapResults[i].x, firstResult.x) << "X snap inconsistent at iteration " << i;
            EXPECT_EQ(snapResults[i].y, firstResult.y) << "Y snap inconsistent at iteration " << i;
            EXPECT_EQ(snapResults[i].z, firstResult.z) << "Z snap inconsistent at iteration " << i;
        }
    }
}