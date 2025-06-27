#include <gtest/gtest.h>
#include <limits>
#include "voxel_data/VoxelDataManager.h"
#include "camera/CameraController.h"
#include "camera/OrbitCamera.h"
#include "visual_feedback/FaceDetector.h"
#include "input/PlacementValidation.h"
#include "undo_redo/VoxelCommands.h"
#include "undo_redo/HistoryManager.h"
#include "events/EventDispatcher.h"
#include "math/Ray.h"
#include "math/CoordinateConverter.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Camera;
using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::Input;
using namespace VoxelEditor::UndoRedo;
using namespace VoxelEditor::Events;

class FaceClickPipelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create all components
        eventDispatcher = std::make_unique<EventDispatcher>();
        voxelManager = std::make_unique<VoxelDataManager>();
        cameraController = std::make_unique<CameraController>(eventDispatcher.get());
        faceDetector = std::make_unique<FaceDetector>();
        historyManager = std::make_unique<HistoryManager>();
        
        // Setup workspace
        Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
        voxelManager->getWorkspaceManager()->setSize(workspaceSize);
        
        // Setup camera
        cameraController->setViewportSize(800, 600);
        cameraController->setViewPreset(ViewPreset::ISOMETRIC);
        cameraController->getCamera()->setDistance(5.0f);
        cameraController->getCamera()->setTarget(WorldCoordinates(Vector3f(0, 0, 0)));
        
        // Place some initial voxels for testing
        setupTestScene();
    }
    
    void setupTestScene() {
        // Place a few voxels to click on
        voxelManager->setVoxel(IncrementCoordinates(0, 0, 0), VoxelResolution::Size_32cm, true);
        voxelManager->setVoxel(IncrementCoordinates(32, 0, 0), VoxelResolution::Size_32cm, true);
        voxelManager->setVoxel(IncrementCoordinates(0, 32, 0), VoxelResolution::Size_32cm, true);
        voxelManager->setVoxel(IncrementCoordinates(0, 0, 32), VoxelResolution::Size_32cm, true);
    }
    
    // Simulate the complete face clicking pipeline
    Face performFaceClick(const Vector2i& screenPos) {
        // Step 1: Generate ray from mouse position
        Math::Ray mouseRay = cameraController->getMouseRay(screenPos);
        
        // Step 2: Detect face or ground
        // Convert Math::Ray to VisualFeedback::Ray
        VisualFeedback::Ray feedbackRay(WorldCoordinates(mouseRay.origin), mouseRay.direction);
        
        // Check each resolution for face hits
        Face bestHit;
        float closestDistance = std::numeric_limits<float>::max();
        
        for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
            VoxelResolution resolution = static_cast<VoxelResolution>(i);
            const VoxelGrid* grid = voxelManager->getGrid(resolution);
            if (!grid || grid->getVoxelCount() == 0) continue;
            
            Face hitFace = faceDetector->detectFaceOrGround(feedbackRay, *grid, resolution);
            if (hitFace.isValid() && !hitFace.isGroundPlane()) {
                // TODO: Calculate actual distance to face
                // For now, just take the first valid hit
                return hitFace;
            }
            
            if (hitFace.isValid() && hitFace.isGroundPlane()) {
                bestHit = hitFace;
            }
        }
        
        return bestHit;
    }
    
    IncrementCoordinates calculatePlacementPosition(const Face& face, VoxelResolution newVoxelRes) {
        if (face.isGroundPlane()) {
            // Place on ground at hit point
            WorldCoordinates hitPoint = face.getGroundPlaneHitPoint();
            return PlacementUtils::snapToValidIncrement(hitPoint);
        } else {
            // Calculate placement position based on face
            return faceDetector->calculatePlacementPosition(face);
        }
    }
    
    bool placeVoxelAtFace(const Face& face, VoxelResolution resolution) {
        if (!face.isValid()) {
            return false;
        }
        
        // Calculate placement position
        IncrementCoordinates placementPos = calculatePlacementPosition(face, resolution);
        
        // Validate placement
        Vector3f workspaceSize = voxelManager->getWorkspaceManager()->getSize();
        PlacementValidationResult validation = PlacementUtils::validatePlacement(
            placementPos, resolution, workspaceSize);
        
        if (validation != PlacementValidationResult::Valid) {
            return false;
        }
        
        // Create and execute place command
        auto command = std::make_unique<VoxelEditCommand>(
            voxelManager.get(), 
            placementPos, 
            resolution, 
            true
        );
        
        bool success = historyManager->executeCommand(std::move(command));
        
        return success;
    }
    
    bool removeVoxelAtFace(const Face& face) {
        if (!face.isValid() || face.isGroundPlane()) {
            return false;
        }
        
        IncrementCoordinates voxelPos = face.getVoxelPosition();
        
        // Create and execute remove command
        auto command = std::make_unique<VoxelEditCommand>(
            voxelManager.get(),
            voxelPos,
            face.getResolution(),
            false
        );
        
        historyManager->executeCommand(std::move(command));
        
        return true;
    }
    
    std::unique_ptr<EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelDataManager> voxelManager;
    std::unique_ptr<CameraController> cameraController;
    std::unique_ptr<FaceDetector> faceDetector;
    std::unique_ptr<HistoryManager> historyManager;
};

TEST_F(FaceClickPipelineTest, ClickOnGroundPlane_PlacesVoxel) {
    // Simulate clicking on empty ground
    Vector2i clickPos(400, 450); // Slightly below center
    
    // Perform click
    Face hitFace = performFaceClick(clickPos);
    
    EXPECT_TRUE(hitFace.isValid());
    EXPECT_TRUE(hitFace.isGroundPlane());
    
    // Place voxel
    bool placed = placeVoxelAtFace(hitFace, VoxelResolution::Size_32cm);
    EXPECT_TRUE(placed);
    
    // Verify voxel was placed
    IncrementCoordinates expectedPos = PlacementUtils::snapToValidIncrement(
        hitFace.getGroundPlaneHitPoint());
    EXPECT_TRUE(voxelManager->hasVoxel(expectedPos, VoxelResolution::Size_32cm));
}

TEST_F(FaceClickPipelineTest, ClickOnVoxelTop_PlacesAbove) {
    // Click on voxel at (32, 0, 0) which doesn't have a voxel above it
    // 32cm voxel at (32,0,0) has bottom at y=0.0, top at y=0.32
    Vector3f voxelWorldPos(0.32f, 0.30f, 0.0f); // Near top of 32cm voxel at (32,0,0)
    Vector2i screenPos = cameraController->worldToScreen(voxelWorldPos);
    
    // Perform click
    Face hitFace = performFaceClick(screenPos);
    
    EXPECT_TRUE(hitFace.isValid());
    EXPECT_FALSE(hitFace.isGroundPlane());
    
    // Debug: Print what we actually hit
    if (!hitFace.isGroundPlane()) {
        IncrementCoordinates hitPos = hitFace.getVoxelPosition();
        printf("Hit voxel at (%d, %d, %d)\n", hitPos.x(), hitPos.y(), hitPos.z());
    }
    
    // Should hit the voxel at (32, 0, 0)
    EXPECT_EQ(hitFace.getVoxelPosition(), IncrementCoordinates(32, 0, 0));
    
    // Place voxel on top
    bool placed = placeVoxelAtFace(hitFace, VoxelResolution::Size_32cm);
    EXPECT_TRUE(placed);
    
    // Verify voxel was placed above
    IncrementCoordinates expectedPos(32, 32, 0); // 32cm above (32,0,0)
    EXPECT_TRUE(voxelManager->hasVoxel(expectedPos, VoxelResolution::Size_32cm));
}

TEST_F(FaceClickPipelineTest, RightClickOnVoxel_RemovesIt) {
    // Get screen position of voxel
    Vector3f voxelWorldPos(0.32f, 0.0f, 0.0f); // Center of voxel at (32, 0, 0)
    Vector2i screenPos = cameraController->worldToScreen(voxelWorldPos);
    
    // Perform click detection
    Face hitFace = performFaceClick(screenPos);
    
    EXPECT_TRUE(hitFace.isValid());
    EXPECT_FALSE(hitFace.isGroundPlane());
    EXPECT_EQ(hitFace.getVoxelPosition(), IncrementCoordinates(32, 0, 0));
    
    // Remove voxel
    bool removed = removeVoxelAtFace(hitFace);
    EXPECT_TRUE(removed);
    
    // Verify voxel was removed
    EXPECT_FALSE(voxelManager->hasVoxel(IncrementCoordinates(32, 0, 0), VoxelResolution::Size_32cm));
}

TEST_F(FaceClickPipelineTest, PlaceSmallVoxelOnLargeFace) {
    // Place a large voxel first
    bool largeVoxelPlaced = voxelManager->setVoxel(IncrementCoordinates(100, 0, 100), VoxelResolution::Size_64cm, true);
    ASSERT_TRUE(largeVoxelPlaced) << "Failed to place 64cm voxel at (100, 0, 100)";
    
    // Verify it was placed
    ASSERT_TRUE(voxelManager->hasVoxel(IncrementCoordinates(100, 0, 100), VoxelResolution::Size_64cm));
    
    // Debug: Print all voxels
    printf("All voxels before click:\n");
    for (int i = 0; i < static_cast<int>(VoxelResolution::COUNT); ++i) {
        VoxelResolution res = static_cast<VoxelResolution>(i);
        auto voxels = voxelManager->getAllVoxels(res);
        if (!voxels.empty()) {
            printf("  Resolution %d: %zu voxels\n", i, voxels.size());
            for (const auto& voxel : voxels) {
                printf("    (%d, %d, %d)\n", voxel.incrementPos.x(), voxel.incrementPos.y(), voxel.incrementPos.z());
            }
        }
    }
    
    // Click on top face of large voxel
    Vector3f clickWorldPos(1.0f, 0.64f, 1.0f); // On top face of 64cm voxel at (100,0,100)
    Vector2i screenPos = cameraController->worldToScreen(clickWorldPos);
    
    Face hitFace = performFaceClick(screenPos);
    
    EXPECT_TRUE(hitFace.isValid());
    
    // Debug: Print what we actually hit
    if (hitFace.isValid()) {
        if (hitFace.isGroundPlane()) {
            printf("PlaceSmallVoxelOnLargeFace: Hit ground plane\n");
        } else {
            IncrementCoordinates hitPos = hitFace.getVoxelPosition();
            printf("PlaceSmallVoxelOnLargeFace: Hit voxel at (%d, %d, %d) with resolution %d\n", 
                   hitPos.x(), hitPos.y(), hitPos.z(), static_cast<int>(hitFace.getResolution()));
        }
    } else {
        printf("PlaceSmallVoxelOnLargeFace: No hit\n");
    }
    
    EXPECT_EQ(hitFace.getVoxelPosition(), IncrementCoordinates(100, 0, 100));
    EXPECT_EQ(hitFace.getResolution(), VoxelResolution::Size_64cm);
    
    // Place small voxel
    bool placed = placeVoxelAtFace(hitFace, VoxelResolution::Size_4cm);
    EXPECT_TRUE(placed);
    
    // Small voxel should be placed on the surface
    // Check that at least one 4cm voxel exists above the large voxel
    bool foundSmallVoxel = false;
    for (int x = 100; x <= 163; x += 4) {
        for (int z = 100; z <= 163; z += 4) {
            if (voxelManager->hasVoxel(IncrementCoordinates(x, 64, z), VoxelResolution::Size_4cm)) {
                foundSmallVoxel = true;
                break;
            }
        }
    }
    EXPECT_TRUE(foundSmallVoxel);
}

TEST_F(FaceClickPipelineTest, UndoRedoFaceClick) {
    // Click to place voxel
    Vector2i clickPos(500, 400);
    Face hitFace = performFaceClick(clickPos);
    
    EXPECT_TRUE(hitFace.isValid());
    
    IncrementCoordinates placementPos = calculatePlacementPosition(hitFace, VoxelResolution::Size_16cm);
    bool placed = placeVoxelAtFace(hitFace, VoxelResolution::Size_16cm);
    EXPECT_TRUE(placed);
    
    // Verify placed
    EXPECT_TRUE(voxelManager->hasVoxel(placementPos, VoxelResolution::Size_16cm));
    
    // Undo
    EXPECT_TRUE(historyManager->canUndo());
    historyManager->undo();
    
    // Verify removed
    EXPECT_FALSE(voxelManager->hasVoxel(placementPos, VoxelResolution::Size_16cm));
    
    // Redo
    EXPECT_TRUE(historyManager->canRedo());
    historyManager->redo();
    
    // Verify placed again
    EXPECT_TRUE(voxelManager->hasVoxel(placementPos, VoxelResolution::Size_16cm));
}

TEST_F(FaceClickPipelineTest, ClickOutsideWorkspace_NoPlacement) {
    // Set camera to look at edge of workspace
    cameraController->getCamera()->setTarget(WorldCoordinates(Vector3f(2.5f, 0, 2.5f)));
    
    // Click beyond workspace boundary
    Vector3f outsidePos(3.0f, 0.0f, 3.0f); // Outside 5m workspace
    Vector2i screenPos = cameraController->worldToScreen(outsidePos);
    
    Face hitFace = performFaceClick(screenPos);
    
    // Might hit ground plane but placement should fail validation
    if (hitFace.isValid()) {
        bool placed = placeVoxelAtFace(hitFace, VoxelResolution::Size_32cm);
        EXPECT_FALSE(placed); // Should fail due to workspace bounds
    }
}

TEST_F(FaceClickPipelineTest, MultipleVoxelPlacements) {
    // Test placing multiple voxels in sequence
    struct PlacementTest {
        Vector2i clickPos;
        VoxelResolution resolution;
        bool expectSuccess;
    };
    
    std::vector<PlacementTest> placements = {
        {Vector2i(400, 500), VoxelResolution::Size_8cm, true},
        {Vector2i(450, 500), VoxelResolution::Size_8cm, true},
        {Vector2i(500, 500), VoxelResolution::Size_8cm, true},
        {Vector2i(400, 450), VoxelResolution::Size_16cm, true},
        {Vector2i(450, 450), VoxelResolution::Size_16cm, true}
    };
    
    int successCount = 0;
    for (const auto& test : placements) {
        Face hitFace = performFaceClick(test.clickPos);
        if (hitFace.isValid()) {
            bool placed = placeVoxelAtFace(hitFace, test.resolution);
            if (placed) {
                successCount++;
            }
            EXPECT_EQ(placed, test.expectSuccess);
        }
    }
    
    EXPECT_GT(successCount, 0);
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 4 + successCount); // 4 initial + new ones
}

TEST_F(FaceClickPipelineTest, RayMissesEverything) {
    // Clear all voxels to ensure nothing to hit
    voxelManager->clearAll();
    
    // Point camera to the side where there's nothing
    cameraController->setViewPreset(ViewPreset::LEFT);
    cameraController->getCamera()->setTarget(WorldCoordinates(Vector3f(10.0f, 2.0f, 0.0f))); // Look at empty space
    
    // Click in center (ray goes into empty space)
    Vector2i clickPos(400, 300);
    Face hitFace = performFaceClick(clickPos);
    
    // The ray might hit ground plane at a distance, so check if it's a voxel hit
    if (hitFace.isValid()) {
        EXPECT_TRUE(hitFace.isGroundPlane()) << "Should only hit ground plane, not voxels";
    }
    
    // Verify no voxels were hit
    EXPECT_EQ(voxelManager->getTotalVoxelCount(), 0U);
}

TEST_F(FaceClickPipelineTest, NonAlignedVoxelPlacement) {
    // Clear existing voxels to avoid overlap issues
    voxelManager->clearAll();
    
    // Test placing voxels at arbitrary 1cm positions
    Vector3f clickPoint(0.13f, 0.0f, 0.27f); // Non-aligned position
    
    // Create a ray that hits this point on ground
    Vector3f rayOrigin(0.13f, 5.0f, 0.27f);
    Vector3f rayDir(0, -1, 0);
    VisualFeedback::Ray ray(WorldCoordinates(rayOrigin), rayDir);
    
    // Detect ground plane
    Face groundFace = faceDetector->detectGroundPlane(ray);
    ASSERT_TRUE(groundFace.isValid()) << "Ground plane detection failed";
    ASSERT_TRUE(groundFace.isGroundPlane()) << "Face is not ground plane";
    
    
    // Place voxel at non-aligned position
    bool placed = placeVoxelAtFace(groundFace, VoxelResolution::Size_32cm);
    EXPECT_TRUE(placed) << "Failed to place voxel";
    
    
    // Verify placement at exact 1cm increment
    IncrementCoordinates expectedPos(13, 0, 27); // Exact 1cm positions
    EXPECT_TRUE(voxelManager->hasVoxel(expectedPos, VoxelResolution::Size_32cm)) 
        << "Expected voxel at (13, 0, 27) but it was not found";
}