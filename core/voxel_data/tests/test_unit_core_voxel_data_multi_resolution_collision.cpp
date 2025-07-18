#include <gtest/gtest.h>
#include "../VoxelDataManager.h"
#include "../../foundation/math/CoordinateTypes.h"
#include "../../foundation/math/CoordinateConverter.h"
#include "../../foundation/events/EventDispatcher.h"

using namespace VoxelEditor;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;

class MultiResolutionCollisionTest : public ::testing::Test {
protected:
    void SetUp() override {
        eventDispatcher = std::make_unique<Events::EventDispatcher>();
        voxelManager = std::make_unique<VoxelDataManager>(eventDispatcher.get());
        voxelManager->resizeWorkspace(Vector3f(10.0f, 10.0f, 10.0f));
        
        // Enable debug logging
        auto& logger = Logging::Logger::getInstance();
        logger.setLevel(Logging::LogLevel::Debug);
        logger.clearOutputs();
        logger.addOutput(std::make_unique<Logging::FileOutput>("multi_res_collision_test.log", "TestLog", false));
    }
    
    void TearDown() override {
        voxelManager.reset();
        eventDispatcher.reset();
    }
    
    std::unique_ptr<Events::EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelDataManager> voxelManager;
};

// Test collision detection between different voxel sizes
TEST_F(MultiResolutionCollisionTest, CollisionDetection_SmallVoxelInsideLargeVoxel) {
    // Place a large 32cm voxel at origin
    voxelManager->setActiveResolution(VoxelResolution::Size_32cm);
    IncrementCoordinates largeVoxelPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(largeVoxelPos, VoxelResolution::Size_32cm, true));
    
    // Try to place small 1cm voxels inside the large voxel (should fail)
    voxelManager->setActiveResolution(VoxelResolution::Size_1cm);
    
    // Test positions inside the large voxel
    // Large voxel at (0,0,0) with size 32cm has bounds (-16,0,-16) to (16,32,16)
    std::vector<IncrementCoordinates> insidePositions = {
        IncrementCoordinates(0, 16, 0),    // Center of large voxel
        IncrementCoordinates(1, 1, 1),     // Near bottom corner  
        IncrementCoordinates(15, 31, 15),  // Near top corner (inside)
        IncrementCoordinates(-15, 15, -15),// Other corner (inside)
    };
    
    for (size_t i = 0; i < insidePositions.size(); ++i) {
        IncrementCoordinates smallPos = insidePositions[i];
        
        // Validate position first
        auto validation = voxelManager->validatePosition(smallPos, VoxelResolution::Size_1cm);
        EXPECT_FALSE(validation.valid) << "Position should be invalid (inside large voxel) for test " << i;
        
        // Try to place voxel (should fail)
        bool placed = voxelManager->setVoxel(smallPos, VoxelResolution::Size_1cm, true);
        EXPECT_FALSE(placed) << "Should not be able to place small voxel inside large voxel for test " << i;
        
        // Verify voxel was not placed
        EXPECT_FALSE(voxelManager->hasVoxel(smallPos.value(), VoxelResolution::Size_1cm)) 
            << "Small voxel should not exist after failed placement for test " << i;
    }
}

TEST_F(MultiResolutionCollisionTest, CollisionDetection_LargeVoxelOverlappingSmallVoxel) {
    // Place small 1cm voxels first
    voxelManager->setActiveResolution(VoxelResolution::Size_1cm);
    std::vector<IncrementCoordinates> smallPositions = {
        IncrementCoordinates(5, 5, 5),
        IncrementCoordinates(10, 10, 10),
        IncrementCoordinates(15, 15, 15),
        IncrementCoordinates(20, 20, 20),
    };
    
    for (const auto& pos : smallPositions) {
        ASSERT_TRUE(voxelManager->setVoxel(pos, VoxelResolution::Size_1cm, true));
    }
    
    // Try to place a large 32cm voxel that would overlap with small voxels
    voxelManager->setActiveResolution(VoxelResolution::Size_32cm);
    IncrementCoordinates largeVoxelPos(0, 0, 0);
    
    // Validate position first
    auto validation = voxelManager->validatePosition(largeVoxelPos, VoxelResolution::Size_32cm);
    EXPECT_FALSE(validation.valid) << "Large voxel position should be invalid (overlaps with small voxels)";
    
    // Try to place voxel (should fail)
    bool placed = voxelManager->setVoxel(largeVoxelPos, VoxelResolution::Size_32cm, true);
    EXPECT_FALSE(placed) << "Should not be able to place large voxel overlapping small voxels";
    
    // Verify large voxel was not placed
    EXPECT_FALSE(voxelManager->hasVoxel(largeVoxelPos.value(), VoxelResolution::Size_32cm)) 
        << "Large voxel should not exist after failed placement";
    
    // Verify small voxels are still there
    for (const auto& pos : smallPositions) {
        EXPECT_TRUE(voxelManager->hasVoxel(pos.value(), VoxelResolution::Size_1cm)) 
            << "Small voxel should still exist at position (" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }
}

// Test overlap validation when placing small voxels on large voxel faces
TEST_F(MultiResolutionCollisionTest, OverlapValidation_SmallVoxelOnLargeFace) {
    // Place a large 64cm voxel at origin
    voxelManager->setActiveResolution(VoxelResolution::Size_64cm);
    IncrementCoordinates largeVoxelPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(largeVoxelPos, VoxelResolution::Size_64cm, true));
    
    // Try to place 4cm voxels on different faces of the large voxel
    voxelManager->setActiveResolution(VoxelResolution::Size_4cm);
    
    struct FaceTest {
        IncrementCoordinates position;
        std::string description;
        bool shouldSucceed;
    };
    
    std::vector<FaceTest> faceTests = {
        // Positions on top face (should succeed)
        {IncrementCoordinates(0, 64, 0), "Top face - corner", true},
        {IncrementCoordinates(32, 64, 32), "Top face - center", true},
        {IncrementCoordinates(60, 64, 60), "Top face - near edge", true},
        
        // Positions on side faces (should succeed)
        {IncrementCoordinates(64, 0, 0), "Right face - bottom", true},
        {IncrementCoordinates(64, 32, 32), "Right face - center", true},
        {IncrementCoordinates(64, 60, 60), "Right face - top", true},
        
        // Positions on front face (should succeed)
        {IncrementCoordinates(0, 0, 64), "Front face - bottom", true},
        {IncrementCoordinates(32, 32, 64), "Front face - center", true},
        {IncrementCoordinates(60, 60, 64), "Front face - top", true},
        
        // Positions that would overlap (should fail)
        // 64cm voxel at (0,0,0) has bounds (-32,0,-32) to (32,64,32)
        {IncrementCoordinates(0, 0, 0), "Same position as large voxel", false},
        {IncrementCoordinates(0, 32, 0), "Inside large voxel center", false},
        {IncrementCoordinates(31, 63, 31), "Inside large voxel near edge", false},
    };
    
    for (const auto& test : faceTests) {
        // Validate position first
        auto validation = voxelManager->validatePosition(test.position, VoxelResolution::Size_4cm);
        EXPECT_EQ(validation.valid, test.shouldSucceed) 
            << "Position validation incorrect for " << test.description;
        
        // Try to place voxel
        bool placed = voxelManager->setVoxel(test.position, VoxelResolution::Size_4cm, true);
        EXPECT_EQ(placed, test.shouldSucceed) 
            << "Placement result incorrect for " << test.description;
        
        // Verify placement result
        bool exists = voxelManager->hasVoxel(test.position.value(), VoxelResolution::Size_4cm);
        EXPECT_EQ(exists, test.shouldSucceed) 
            << "Voxel existence incorrect for " << test.description;
        
        // Clean up successful placements for next test
        if (placed) {
            voxelManager->setVoxel(test.position, VoxelResolution::Size_4cm, false);
        }
    }
}

// Test adjacent placement validation across resolution boundaries
TEST_F(MultiResolutionCollisionTest, AdjacentPlacementValidation_ResolutionBoundaries) {
    // Place a 32cm voxel at origin
    voxelManager->setActiveResolution(VoxelResolution::Size_32cm);
    IncrementCoordinates largeVoxelPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(largeVoxelPos, VoxelResolution::Size_32cm, true));
    
    // Test placing 1cm voxels adjacent to the large voxel
    voxelManager->setActiveResolution(VoxelResolution::Size_1cm);
    
    struct AdjacentTest {
        IncrementCoordinates position;
        std::string description;
        bool shouldSucceed;
    };
    
    std::vector<AdjacentTest> adjacentTests = {
        // Adjacent positions (should succeed)
        {IncrementCoordinates(32, 0, 0), "Adjacent +X", true},
        {IncrementCoordinates(-17, 0, 0), "Adjacent -X", true}, // 32cm extends to -16, 1cm voxel has 0.5cm radius
        {IncrementCoordinates(0, 32, 0), "Adjacent +Y", true},
        {IncrementCoordinates(0, -1, 0), "Adjacent -Y", false}, // Below ground plane
        {IncrementCoordinates(0, 0, 32), "Adjacent +Z", true},
        {IncrementCoordinates(0, 0, -17), "Adjacent -Z", true}, // 32cm extends to -16, 1cm voxel has 0.5cm radius
        
        // Corner adjacent positions (should succeed)
        {IncrementCoordinates(32, 32, 0), "Corner +X+Y", true},
        {IncrementCoordinates(32, 0, 32), "Corner +X+Z", true},
        {IncrementCoordinates(0, 32, 32), "Corner +Y+Z", true},
        {IncrementCoordinates(32, 32, 32), "Corner +X+Y+Z", true},
        
        // Positions inside large voxel (should fail)
        // 32cm voxel at (0,0,0) has bounds (-16,0,-16) to (16,32,16)
        {IncrementCoordinates(1, 1, 1), "Inside near corner", false},
        {IncrementCoordinates(15, 31, 15), "Inside far corner", false},
        {IncrementCoordinates(0, 16, 0), "Inside center", false},
    };
    
    for (const auto& test : adjacentTests) {
        // Validate position first
        auto validation = voxelManager->validatePosition(test.position, VoxelResolution::Size_1cm);
        EXPECT_EQ(validation.valid, test.shouldSucceed) 
            << "Position validation incorrect for " << test.description;
        
        // Try to place voxel
        bool placed = voxelManager->setVoxel(test.position, VoxelResolution::Size_1cm, true);
        EXPECT_EQ(placed, test.shouldSucceed) 
            << "Placement result incorrect for " << test.description;
        
        // Verify placement result
        bool exists = voxelManager->hasVoxel(test.position.value(), VoxelResolution::Size_1cm);
        EXPECT_EQ(exists, test.shouldSucceed) 
            << "Voxel existence incorrect for " << test.description;
        
        // Clean up successful placements for next test
        if (placed) {
            voxelManager->setVoxel(test.position, VoxelResolution::Size_1cm, false);
        }
    }
}

// Test workspace bounds validation for mixed resolutions
TEST_F(MultiResolutionCollisionTest, WorkspaceBoundsValidation_MixedResolutions) {
    // Set a smaller workspace for testing
    voxelManager->resizeWorkspace(Vector3f(5.0f, 5.0f, 5.0f));
    
    // Test different voxel sizes at workspace boundaries
    struct BoundaryTest {
        VoxelResolution resolution;
        IncrementCoordinates position;
        std::string description;
        bool shouldSucceed;
    };
    
    std::vector<BoundaryTest> boundaryTests = {
        // Positions within workspace (should succeed)
        {VoxelResolution::Size_1cm, IncrementCoordinates(0, 0, 0), "1cm at origin", true},
        {VoxelResolution::Size_4cm, IncrementCoordinates(0, 0, 0), "4cm at origin", true},
        {VoxelResolution::Size_16cm, IncrementCoordinates(0, 0, 0), "16cm at origin", true},
        {VoxelResolution::Size_64cm, IncrementCoordinates(0, 0, 0), "64cm at origin", true},
        
        // Positions near workspace boundaries (should succeed if within bounds)
        {VoxelResolution::Size_1cm, IncrementCoordinates(249, 0, 0), "1cm near +X boundary", true},
        {VoxelResolution::Size_4cm, IncrementCoordinates(248, 0, 0), "4cm near +X boundary", true},
        {VoxelResolution::Size_16cm, IncrementCoordinates(240, 0, 0), "16cm near +X boundary", true},
        
        // Positions outside workspace (should fail)
        {VoxelResolution::Size_1cm, IncrementCoordinates(250, 0, 0), "1cm outside +X boundary", false},
        {VoxelResolution::Size_4cm, IncrementCoordinates(252, 0, 0), "4cm outside +X boundary", false},
        {VoxelResolution::Size_16cm, IncrementCoordinates(256, 0, 0), "16cm outside +X boundary", false},
        {VoxelResolution::Size_64cm, IncrementCoordinates(219, 0, 0), "64cm outside +X boundary", false}, // Would extend to 251
        
        // Positions outside workspace in other directions
        {VoxelResolution::Size_1cm, IncrementCoordinates(-250, 0, 0), "1cm outside -X boundary", false},
        {VoxelResolution::Size_1cm, IncrementCoordinates(0, 500, 0), "1cm outside +Y boundary", false}, // Y goes 0 to 500
        {VoxelResolution::Size_1cm, IncrementCoordinates(0, 0, 250), "1cm outside +Z boundary", false},
        {VoxelResolution::Size_1cm, IncrementCoordinates(0, 0, -250), "1cm outside -Z boundary", false},
    };
    
    for (const auto& test : boundaryTests) {
        voxelManager->setActiveResolution(test.resolution);
        
        // Validate position first
        auto validation = voxelManager->validatePosition(test.position, test.resolution);
        EXPECT_EQ(validation.valid, test.shouldSucceed) 
            << "Position validation incorrect for " << test.description;
        
        if (!test.shouldSucceed) {
            EXPECT_FALSE(validation.errorMessage.empty()) 
                << "Should have error message for " << test.description;
        }
        
        // Try to place voxel
        bool placed = voxelManager->setVoxel(test.position, test.resolution, true);
        EXPECT_EQ(placed, test.shouldSucceed) 
            << "Placement result incorrect for " << test.description;
        
        // Clean up successful placements for next test
        if (placed) {
            voxelManager->setVoxel(test.position, test.resolution, false);
        }
    }
}

TEST_F(MultiResolutionCollisionTest, ComplexScenario_MultipleResolutionInteraction) {
    // Create a complex scenario with multiple voxel sizes interacting
    
    // Place a large 64cm voxel at origin
    voxelManager->setActiveResolution(VoxelResolution::Size_64cm);
    IncrementCoordinates largeVoxelPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(largeVoxelPos, VoxelResolution::Size_64cm, true));
    
    // Place medium 16cm voxels on top and sides
    voxelManager->setActiveResolution(VoxelResolution::Size_16cm);
    std::vector<IncrementCoordinates> mediumPositions = {
        IncrementCoordinates(0, 64, 0),     // On top
        IncrementCoordinates(16, 64, 16),   // On top, offset
        IncrementCoordinates(64, 0, 0),     // On right side
        IncrementCoordinates(64, 16, 16),   // On right side, offset
    };
    
    for (const auto& pos : mediumPositions) {
        ASSERT_TRUE(voxelManager->setVoxel(pos, VoxelResolution::Size_16cm, true)) 
            << "Should be able to place 16cm voxel at (" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }
    
    // Place small 4cm voxels on the medium voxels
    voxelManager->setActiveResolution(VoxelResolution::Size_4cm);
    std::vector<IncrementCoordinates> smallPositions = {
        IncrementCoordinates(0, 80, 0),     // On top of first medium voxel
        IncrementCoordinates(4, 80, 4),     // On top of first medium voxel, offset
        IncrementCoordinates(16, 80, 16),   // On top of second medium voxel
        IncrementCoordinates(80, 0, 0),     // On the right side medium voxel
    };
    
    for (const auto& pos : smallPositions) {
        ASSERT_TRUE(voxelManager->setVoxel(pos, VoxelResolution::Size_4cm, true)) 
            << "Should be able to place 4cm voxel at (" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }
    
    // Try to place voxels that would conflict
    std::vector<std::pair<IncrementCoordinates, VoxelResolution>> conflictingPositions = {
        {IncrementCoordinates(0, 0, 0), VoxelResolution::Size_32cm},        // Would overlap with large voxel
        {IncrementCoordinates(32, 32, 32), VoxelResolution::Size_1cm},      // Would be inside large voxel
        {IncrementCoordinates(0, 64, 0), VoxelResolution::Size_8cm},        // Would overlap with medium voxel
        {IncrementCoordinates(80, 0, 0), VoxelResolution::Size_2cm},        // Would overlap with small voxel
    };
    
    for (const auto& conflict : conflictingPositions) {
        IncrementCoordinates pos = conflict.first;
        VoxelResolution res = conflict.second;
        
        voxelManager->setActiveResolution(res);
        
        // Validate position first
        auto validation = voxelManager->validatePosition(pos, res);
        EXPECT_FALSE(validation.valid) 
            << "Conflicting position should be invalid at (" << pos.x() << "," << pos.y() << "," << pos.z() 
            << ") with resolution " << static_cast<int>(res);
        
        // Try to place voxel (should fail)
        bool placed = voxelManager->setVoxel(pos, res, true);
        EXPECT_FALSE(placed) 
            << "Should not be able to place conflicting voxel at (" << pos.x() << "," << pos.y() << "," << pos.z() 
            << ") with resolution " << static_cast<int>(res);
    }
    
    // Verify all original voxels are still there
    EXPECT_TRUE(voxelManager->hasVoxel(largeVoxelPos.value(), VoxelResolution::Size_64cm)) 
        << "Large voxel should still exist";
    
    for (const auto& pos : mediumPositions) {
        EXPECT_TRUE(voxelManager->hasVoxel(pos.value(), VoxelResolution::Size_16cm)) 
            << "Medium voxel should still exist at (" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }
    
    for (const auto& pos : smallPositions) {
        EXPECT_TRUE(voxelManager->hasVoxel(pos.value(), VoxelResolution::Size_4cm)) 
            << "Small voxel should still exist at (" << pos.x() << "," << pos.y() << "," << pos.z() << ")";
    }
}

TEST_F(MultiResolutionCollisionTest, PrecisionTest_VoxelBoundaryCollisions) {
    // Test collision detection precision at voxel boundaries
    
    // Place a 32cm voxel at origin
    voxelManager->setActiveResolution(VoxelResolution::Size_32cm);
    IncrementCoordinates largeVoxelPos(0, 0, 0);
    ASSERT_TRUE(voxelManager->setVoxel(largeVoxelPos, VoxelResolution::Size_32cm, true));
    
    // Test 1cm voxels at exact boundary positions
    voxelManager->setActiveResolution(VoxelResolution::Size_1cm);
    
    struct BoundaryTest {
        IncrementCoordinates position;
        std::string description;
        bool shouldSucceed;
    };
    
    std::vector<BoundaryTest> boundaryTests = {
        // Positions exactly at boundaries (should succeed)
        {IncrementCoordinates(32, 0, 0), "Exactly at +X boundary", true},
        {IncrementCoordinates(-17, 0, 0), "Exactly at -X boundary", true}, // Actually adjacent, not overlapping
        {IncrementCoordinates(0, 32, 0), "Exactly at +Y boundary", true},
        {IncrementCoordinates(0, -1, 0), "Exactly at -Y boundary", false}, // Below ground plane
        {IncrementCoordinates(0, 0, 32), "Exactly at +Z boundary", true},
        {IncrementCoordinates(0, 0, -17), "Exactly at -Z boundary", true}, // Actually adjacent, not overlapping
        
        // Positions just inside boundaries (should fail)
        // 32cm voxel at (0,0,0) has bounds (-16,0,-16) to (16,32,16)
        {IncrementCoordinates(15, 0, 0), "Just inside +X boundary", false},
        {IncrementCoordinates(0, 0, 0), "At origin (inside)", false},
        {IncrementCoordinates(1, 1, 1), "Just inside corner", false},
        {IncrementCoordinates(15, 31, 15), "Just inside far corner", false},
        
        // Positions just outside boundaries (should succeed)
        {IncrementCoordinates(33, 0, 0), "Just outside +X boundary", true},
        {IncrementCoordinates(-18, 0, 0), "Just outside -X boundary", true}, // Further away than adjacent position
        {IncrementCoordinates(0, 33, 0), "Just outside +Y boundary", true},
        {IncrementCoordinates(0, -2, 0), "Just outside -Y boundary", false}, // Below ground plane
    };
    
    for (const auto& test : boundaryTests) {
        // Validate position first
        auto validation = voxelManager->validatePosition(test.position, VoxelResolution::Size_1cm);
        EXPECT_EQ(validation.valid, test.shouldSucceed) 
            << "Position validation incorrect for " << test.description;
        
        // Try to place voxel
        bool placed = voxelManager->setVoxel(test.position, VoxelResolution::Size_1cm, true);
        EXPECT_EQ(placed, test.shouldSucceed) 
            << "Placement result incorrect for " << test.description;
        
        // Clean up successful placements for next test
        if (placed) {
            voxelManager->setVoxel(test.position, VoxelResolution::Size_1cm, false);
        }
    }
}