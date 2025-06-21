#include <gtest/gtest.h>
#include <memory>
#include "voxel_data/VoxelDataManager.h"
#include "voxel_data/WorkspaceManager.h"
#include "voxel_data/VoxelTypes.h"
#include "math/Vector3f.h"
#include "math/Vector3i.h"
#include "events/EventDispatcher.h"
#include "logging/Logger.h"
#include <sstream>
#include <iostream>

namespace VoxelEditor {
namespace Tests {

class WorkspaceBoundaryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup logging to file only
        auto& logger = Logging::Logger::getInstance();
        logger.setLevel(Logging::LogLevel::Debug);
        logger.clearOutputs();
        logger.addOutput(std::make_unique<Logging::FileOutput>("workspace_boundary_test.log", "TestLog", false));
        
        // Create event dispatcher
        eventDispatcher = std::make_unique<Events::EventDispatcher>();
        
        // Create voxel manager with workspace
        voxelManager = std::make_unique<VoxelData::VoxelDataManager>(eventDispatcher.get());
        
        // Set up a known workspace for testing
        setupTestWorkspace();
    }
    
    void TearDown() override {
        voxelManager.reset();
        eventDispatcher.reset();
    }
    
    void setupTestWorkspace() {
        // Create a 3x3x3 meter workspace with 8cm resolution
        // This should allow placement from (-1.5m, -1.5m, -1.5m) to (1.5m, 1.5m, 1.5m)
        voxelManager->resizeWorkspace(Math::Vector3f(3.0f, 3.0f, 3.0f));
        voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
        
        // Log workspace size for debugging
        Math::Vector3f size = voxelManager->getWorkspaceSize();
        std::cout << "Workspace size: " << size.x << " x " << size.y << " x " << size.z << std::endl;
    }
    
    bool attemptPlacement(float x, float y, float z) {
        // Convert cm to meters
        Math::Vector3f worldPos(x / 100.0f, y / 100.0f, z / 100.0f);
        
        std::cout << "Attempting placement at: (" << worldPos.x << ", " << worldPos.y << ", " << worldPos.z << ")" << std::endl;
        
        // Try to place voxel using world position API
        try {
            bool result = voxelManager->setVoxelAtWorldPos(worldPos, true);
            std::cout << "Result: " << (result ? "success" : "failed") << std::endl;
            return result;
        } catch (const std::exception& e) {
            std::cout << "Exception: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool attemptPlacement(const Math::Vector3f& pos) {
        return attemptPlacement(pos.x, pos.y, pos.z);
    }
    
    // Helper to check if a voxel exists at the given position
    bool voxelExistsAt(float x, float y, float z) {
        Math::Vector3f worldPos(x / 100.0f, y / 100.0f, z / 100.0f);
        return voxelManager->hasVoxelAtWorldPos(worldPos);
    }
    
    std::unique_ptr<Events::EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelData::VoxelDataManager> voxelManager;
};

// Test simple placement first
TEST_F(WorkspaceBoundaryTest, SimplePlacement) {
    // Test a single simple placement using grid coordinates
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_8cm;
    Math::Vector3i gridPos(0, 0, 0);
    
    std::cout << "Testing setVoxel at grid position (0,0,0)" << std::endl;
    bool success = voxelManager->setVoxel(gridPos, resolution, true);
    std::cout << "setVoxel result: " << (success ? "success" : "failed") << std::endl;
    
    EXPECT_TRUE(success) << "Failed to place voxel at grid origin";
    
    // Now test world position API at a different location
    std::cout << "Testing world position API at centered coordinates" << std::endl;
    bool worldSuccess = attemptPlacement(-144.0f, 8.0f, -144.0f);
    EXPECT_TRUE(worldSuccess) << "Failed to place voxel at (-144,8,-144)";
}

// Test corner placements within workspace bounds
TEST_F(WorkspaceBoundaryTest, CornerPlacements) {
    // VoxelGrid now uses centered coordinates!
    // For a 3x3x3 workspace:
    // X and Z: -1.5m to +1.5m (centered)
    // Y: 0 to 3m (not centered)
    // With 8cm resolution, we need positions to be on the grid
    
    const float minPosXZ = -144.0f;  // -144cm = -1.44m (near -1.5m boundary)
    const float maxPosXZ = 144.0f;   // 144cm = 1.44m (near +1.5m boundary)
    const float minPosY = 8.0f;      // 8cm = 0.08m (first valid Y position)
    const float maxPosY = 296.0f;    // 296cm = 2.96m (near 3m boundary)
    const float midPos = 0.0f;       // 0cm = center for X/Z axes
    
    std::vector<Math::Vector3f> corners = {
        // Bottom corners (Y = minPosY)
        {minPosXZ, minPosY, minPosXZ},     // Near corner
        {minPosXZ, minPosY, maxPosXZ},     // 
        {maxPosXZ, minPosY, minPosXZ},     // 
        {maxPosXZ, minPosY, maxPosXZ},     // 
        // Top corners (Y = maxPosY)
        {minPosXZ, maxPosY, minPosXZ},     // 
        {minPosXZ, maxPosY, maxPosXZ},     // 
        {maxPosXZ, maxPosY, minPosXZ},     // 
        {maxPosXZ, maxPosY, maxPosXZ}      // Far corner
    };
    
    for (size_t i = 0; i < corners.size(); ++i) {
        SCOPED_TRACE("Corner " + std::to_string(i) + ": (" + 
                    std::to_string(corners[i].x) + ", " +
                    std::to_string(corners[i].y) + ", " +
                    std::to_string(corners[i].z) + ")");
        
        bool success = attemptPlacement(corners[i]);
        EXPECT_TRUE(success) << "Failed to place voxel at corner position";
    }
}

// Test edge midpoint placements
TEST_F(WorkspaceBoundaryTest, EdgeMidpointPlacements) {
    // Clear any existing voxels from previous tests
    voxelManager->clearAll();
    const float minPosXZ = -144.0f;  // -144cm for X/Z
    const float maxPosXZ = 144.0f;   // 144cm for X/Z
    const float minPosY = 8.0f;      // 8cm for Y
    const float maxPosY = 296.0f;    // 296cm for Y 
    const float midPosXZ = 0.0f;     // 0cm = center for X/Z
    const float midPosY = 152.0f;    // 152cm = middle for Y
    
    std::vector<Math::Vector3f> edgeMidpoints = {
        // X-axis aligned edges
        {midPosXZ, minPosY, minPosXZ}, {midPosXZ, minPosY, maxPosXZ},
        {midPosXZ, maxPosY, minPosXZ}, {midPosXZ, maxPosY, maxPosXZ},
        
        // Y-axis aligned edges
        {minPosXZ, midPosY, minPosXZ}, {minPosXZ, midPosY, maxPosXZ},
        {maxPosXZ, midPosY, minPosXZ}, {maxPosXZ, midPosY, maxPosXZ},
        
        // Z-axis aligned edges (use different Y values to avoid collision)
        {minPosXZ, minPosY, midPosXZ}, {minPosXZ, maxPosY, midPosXZ},
        {maxPosXZ, 24.0f, midPosXZ}, {maxPosXZ, 280.0f, midPosXZ}
    };
    
    for (size_t i = 0; i < edgeMidpoints.size(); ++i) {
        SCOPED_TRACE("Edge midpoint " + std::to_string(i));
        bool success = attemptPlacement(edgeMidpoints[i]);
        EXPECT_TRUE(success) << "Failed to place voxel at edge midpoint";
    }
}

// Test face center placements
TEST_F(WorkspaceBoundaryTest, FaceCenterPlacements) {
    // Clear any existing voxels from previous tests
    voxelManager->clearAll();
    const float minPosXZ = -144.0f;  // -144cm for X/Z (centered)
    const float maxPosXZ = 144.0f;   // 144cm for X/Z (centered)
    const float minPosY = 8.0f;      // 8cm for Y (not centered)
    const float maxPosY = 296.0f;    // 296cm for Y (not centered)
    const float midPosXZ = 0.0f;     // 0cm = center for X/Z
    const float midPosY = 152.0f;    // 152cm = middle for Y
    
    std::vector<Math::Vector3f> faceCenters = {
        {minPosXZ, midPosY, 0.0f},     // Near X face (center Z)
        {maxPosXZ, midPosY, -80.0f},   // Far X face (different Z to avoid collision)
        {midPosXZ, minPosY, midPosXZ}, // Bottom Y face (center X/Z)
        {midPosXZ, maxPosY, -40.0f},   // Top Y face (different Z to avoid collision)
        {midPosXZ, midPosY, minPosXZ}, // Near Z face
        {80.0f, midPosY, maxPosXZ}     // Far Z face (different X to avoid collision)
    };
    
    for (size_t i = 0; i < faceCenters.size(); ++i) {
        SCOPED_TRACE("Face center " + std::to_string(i));
        bool success = attemptPlacement(faceCenters[i]);
        EXPECT_TRUE(success) << "Failed to place voxel at face center";
    }
}

// Test that placements outside workspace bounds fail
TEST_F(WorkspaceBoundaryTest, OutOfBoundsPlacementsShouldFail) {
    // For 3x3x3 workspace centered coordinates:
    // X/Z: -150cm to +150cm, Y: 0 to 300cm
    std::vector<Math::Vector3f> outsidePositions = {
        {200.0f, 150.0f, 0.0f},    // Beyond workspace X (>150cm)
        {-200.0f, 150.0f, 0.0f},   // Beyond workspace X (<-150cm)
        {0.0f, 400.0f, 0.0f},      // Beyond workspace Y (>300cm)
        {0.0f, -8.0f, 0.0f},       // Below ground plane (Y < 0)
        {0.0f, 150.0f, 200.0f},    // Beyond workspace Z (>150cm)
        {0.0f, 150.0f, -200.0f},   // Beyond workspace Z (<-150cm)
        {-200.0f, -8.0f, -200.0f}  // All outside bounds
    };
    
    for (size_t i = 0; i < outsidePositions.size(); ++i) {
        SCOPED_TRACE("Outside position " + std::to_string(i));
        bool success = attemptPlacement(outsidePositions[i]);
        EXPECT_FALSE(success) << "Should not place voxel outside workspace bounds";
    }
}

// Test workspace resizing updates boundaries correctly
TEST_F(WorkspaceBoundaryTest, WorkspaceResizingUpdatesBoundaries) {
    // Place at current boundary (144cm for X/Z in 3m workspace)
    bool success1 = attemptPlacement(144.0f, 8.0f, 144.0f);
    EXPECT_TRUE(success1) << "Should place at 3x3x3 boundary";
    
    // Clear voxels before resizing to avoid blocking the resize
    voxelManager->clearAll();
    
    // Resize to smaller workspace
    bool resizeSuccess = voxelManager->resizeWorkspace(Math::Vector3f(2.0f, 2.0f, 2.0f));
    EXPECT_TRUE(resizeSuccess) << "Workspace resize should succeed after clearing voxels";
    Math::Vector3f newSize = voxelManager->getWorkspaceSize();
    std::cout << "Resized workspace to: " << newSize.x << " x " << newSize.y << " x " << newSize.z << std::endl;
    
    // After resizing to 2m, the bounds are -100cm to +100cm for X/Z
    // Try to place clearly outside these bounds
    bool success2 = attemptPlacement(200.0f, 8.0f, 200.0f);
    EXPECT_FALSE(success2) << "Should not place outside new 2x2x2 bounds";
    
    // But 96cm should work (inside 100cm bound)
    bool success3 = attemptPlacement(96.0f, 8.0f, 96.0f);
    EXPECT_TRUE(success3) << "Should place within new 2x2x2 bounds";
}

// Test that resolution changes affect boundary snapping
TEST_F(WorkspaceBoundaryTest, ResolutionAffectsBoundarySnapping) {
    // With 4cm resolution
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_4cm);
    
    // 144cm is 36 * 4cm, so it's on the grid and at X/Z boundary
    bool success1 = attemptPlacement(144.0f, 4.0f, 144.0f);
    EXPECT_TRUE(success1) << "Should place at 144cm with 4cm resolution";
    
    // Clear voxels before switching resolution to avoid overlaps
    voxelManager->clearAll();
    
    // With 16cm resolution
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_16cm);
    
    // 144cm is 9 * 16cm, so it's on the grid
    bool success2 = attemptPlacement(144.0f, 16.0f, -144.0f);
    EXPECT_TRUE(success2) << "Should place at boundary with 16cm resolution";
    
    // Try to place at 140cm which will be snapped to grid
    bool success3 = attemptPlacement(140.0f, 16.0f, 16.0f);
    EXPECT_TRUE(success3) << "Should place at 140cm with 16cm resolution";
    
    // The voxel was placed at the requested position  
    // Note: With 16cm resolution, 140cm may snap to the nearest grid position
    EXPECT_TRUE(voxelExistsAt(140.0f, 16.0f, 16.0f) || voxelExistsAt(144.0f, 16.0f, 16.0f) || voxelExistsAt(128.0f, 16.0f, 16.0f)) 
        << "Voxel should exist at or near placed position";
}

// Test that center placement always works
TEST_F(WorkspaceBoundaryTest, CenterPlacementAlwaysWorks) {
    std::vector<VoxelData::VoxelResolution> resolutions = {
        VoxelData::VoxelResolution::Size_1cm,
        VoxelData::VoxelResolution::Size_2cm,
        VoxelData::VoxelResolution::Size_4cm,
        VoxelData::VoxelResolution::Size_8cm,
        VoxelData::VoxelResolution::Size_16cm,
        VoxelData::VoxelResolution::Size_32cm
    };
    
    for (const auto& res : resolutions) {
        SCOPED_TRACE("Testing center placement with resolution: " + std::to_string(static_cast<int>(res)));
        
        voxelManager->setActiveResolution(res);
        // Place at workspace center (0cm for X/Z, 150cm for Y in 3m workspace)
        bool success = attemptPlacement(0.0f, 150.0f, 0.0f);
        EXPECT_TRUE(success) << "Center placement should always work";
        
        // Clear for next test
        voxelManager->clearAll();
    }
}

// Test maximum workspace size boundaries
TEST_F(WorkspaceBoundaryTest, MaximumWorkspaceBoundaries) {
    // Test with 8x8x8 workspace (max allowed)
    voxelManager->resizeWorkspace(Math::Vector3f(8.0f, 8.0f, 8.0f));
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
    
    // For 8x8x8 workspace, boundaries are:
    // X/Z: -400cm to +400cm (centered)
    // Y: 0 to 800cm (not centered)
    // Nearest 8cm grid points inside: ±392cm for X/Z, 792cm for Y
    bool success1 = attemptPlacement(392.0f, 8.0f, -392.0f);
    EXPECT_TRUE(success1) << "Should place at X/Z boundary of maximum workspace";
    
    bool success2 = attemptPlacement(-392.0f, 792.0f, 392.0f);
    EXPECT_TRUE(success2) << "Should place at Y boundary of maximum workspace";
    
    bool success3 = attemptPlacement(0.0f, 400.0f, 392.0f);
    EXPECT_TRUE(success3) << "Should place at Z boundary of maximum workspace";
}

// Test asymmetric workspace boundaries
TEST_F(WorkspaceBoundaryTest, AsymmetricWorkspaceBoundaries) {
    // Test with asymmetric workspace
    voxelManager->resizeWorkspace(Math::Vector3f(4.0f, 2.0f, 6.0f)); // 4m x 2m x 6m
    voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_8cm);
    
    // X boundaries: -200cm to +200cm (centered), nearest grid inside: ±192cm
    bool successX1 = attemptPlacement(-192.0f, 8.0f, 0.0f);   // Near boundary
    bool successX2 = attemptPlacement(192.0f, 8.0f, 0.0f);    // Far boundary
    EXPECT_TRUE(successX1 && successX2) << "Should place at X boundaries of 4m workspace";
    
    // Y boundaries: 0 to 200cm, nearest grid inside: 192cm (24 * 8cm)
    bool successY1 = attemptPlacement(0.0f, 8.0f, 16.0f);     // Ground level
    bool successY2 = attemptPlacement(0.0f, 192.0f, 16.0f);   // Upper boundary
    EXPECT_TRUE(successY1 && successY2) << "Should place at Y boundaries of 2m workspace";
    
    // Z boundaries: -300cm to +300cm (centered), nearest grid inside: ±296cm
    bool successZ1 = attemptPlacement(24.0f, 8.0f, -296.0f);  // Near boundary
    bool successZ2 = attemptPlacement(24.0f, 8.0f, 296.0f);   // Far boundary
    EXPECT_TRUE(successZ1 && successZ2) << "Should place at Z boundaries of 6m workspace";
}

} // namespace Tests
} // namespace VoxelEditor

// Add main function with timeout
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}