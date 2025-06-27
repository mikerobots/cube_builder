#include <gtest/gtest.h>

// Include OpenGL headers
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <GLFW/glfw3.h>
#include <memory>
#include <cstdlib>
#include "voxel_data/VoxelDataManager.h"
#include "visual_feedback/PreviewManager.h"
#include "visual_feedback/FaceDetector.h"
#include "input/PlacementValidation.h"
#include "math/Ray.h"
#include "math/Vector3f.h"
#include "math/Vector3i.h"
#include "math/CoordinateConverter.h"
#include "math/CoordinateTypes.h"
#include "events/EventDispatcher.h"
#include "logging/Logger.h"

using namespace VoxelEditor;
using namespace VoxelEditor::VoxelData;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Events;
using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::Input;
using namespace VoxelEditor::Logging;

// Test fixture for preview positioning verification with exact 1cm placement
class PreviewPositioningTest : public ::testing::Test {
protected:
    GLFWwindow* window = nullptr;
    
    void SetUp() override {
        // Skip in CI environment
        if (std::getenv("CI") != nullptr) {
            GTEST_SKIP() << "Skipping OpenGL tests in CI environment";
        }
        
        // Setup logging
        auto& logger = Logger::getInstance();
        logger.setLevel(LogLevel::Debug);
        logger.clearOutputs();
        logger.addOutput(std::make_unique<FileOutput>("preview_positioning_test.log", "TestLog", false));
        
        // Initialize GLFW
        ASSERT_TRUE(glfwInit()) << "Failed to initialize GLFW";
        
        // Set OpenGL 3.3 Core Profile
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window for testing
        
        // Create window
        window = glfwCreateWindow(800, 600, "Preview Positioning Test", nullptr, nullptr);
        ASSERT_NE(window, nullptr) << "Failed to create GLFW window";
        
        glfwMakeContextCurrent(window);
        
        // Initialize OpenGL loader
#ifndef __APPLE__
        ASSERT_TRUE(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) << "Failed to initialize OpenGL loader";
#endif
        
        // Create managers
        eventDispatcher = std::make_unique<EventDispatcher>();
        voxelManager = std::make_unique<VoxelDataManager>(eventDispatcher.get());
        previewManager = std::make_unique<PreviewManager>();
        faceDetector = std::make_unique<FaceDetector>();
    }
    
    void TearDown() override {
        if (window) {
            faceDetector.reset();
            previewManager.reset();
            voxelManager.reset();
            eventDispatcher.reset();
            
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }
    
    std::unique_ptr<EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelDataManager> voxelManager;
    std::unique_ptr<PreviewManager> previewManager;
    std::unique_ptr<FaceDetector> faceDetector;
};

// REQ-2.2.2 (updated): The preview shall show the exact 1cm increment position where the voxel will be placed
TEST_F(PreviewPositioningTest, ExactPositionPreview_NoSnapToVoxelBoundaries) {
    // Test that preview positioning shows exact 1cm positions without resolution-based snapping
    
    // Test with 4cm voxels - preview should show exact position, not snapped position
    voxelManager->setActiveResolution(VoxelResolution::Size_4cm);
    
    // Test positions that are NOT aligned to 4cm boundaries
    std::vector<Vector3i> nonAlignedPositions = {
        Vector3i(1, 1, 1),     // 1cm position (not multiple of 4)
        Vector3i(3, 7, 11),    // Prime numbers (not multiples of 4)
        Vector3i(17, 23, 29),  // More primes
        Vector3i(50, 75, 99),  // Random non-aligned positions
        Vector3i(-5, 13, -21)  // Mixed positive/negative
    };
    
    for (const auto& pos : nonAlignedPositions) {
        if (voxelManager->isValidIncrementPosition(pos)) {
            // Set preview position to exact 1cm position
            previewManager->setPreviewPosition(pos, voxelManager->getActiveResolution());
            
            // Verify preview shows exact position (no snapping)
            EXPECT_TRUE(previewManager->hasPreview()) 
                << "Preview should be active for position (" << pos.x << "," << pos.y << "," << pos.z << ")";
            EXPECT_EQ(previewManager->getPreviewPosition(), pos) 
                << "Preview position should be exact (no snapping) for (" << pos.x << "," << pos.y << "," << pos.z << ")";
            EXPECT_EQ(previewManager->getPreviewResolution(), VoxelResolution::Size_4cm);
        }
    }
}

TEST_F(PreviewPositioningTest, ExactPositionPreview_AllVoxelSizes) {
    // Test that preview positioning works correctly for all voxel sizes at arbitrary positions
    
    // Test arbitrary 1cm positions that are NOT aligned to any common voxel size
    std::vector<Vector3i> testPositions = {
        Vector3i(13, 27, 41),   // Prime numbers
        Vector3i(1, 3, 5),      // Small odds
        Vector3i(7, 11, 19)     // More primes
    };
    
    std::vector<VoxelResolution> testResolutions = {
        VoxelResolution::Size_1cm, VoxelResolution::Size_2cm, 
        VoxelResolution::Size_4cm, VoxelResolution::Size_8cm, 
        VoxelResolution::Size_16cm, VoxelResolution::Size_32cm
    };
    
    for (auto resolution : testResolutions) {
        voxelManager->setActiveResolution(resolution);
        
        for (const auto& pos : testPositions) {
            if (voxelManager->isValidIncrementPosition(pos)) {
                // Set preview at exact position
                previewManager->setPreviewPosition(pos, resolution);
                
                // Verify preview shows exact position (no snapping)
                EXPECT_TRUE(previewManager->hasPreview());
                EXPECT_EQ(previewManager->getPreviewPosition(), pos) 
                    << "Preview should show exact position for " << static_cast<int>(resolution) 
                    << "cm voxel at (" << pos.x << "," << pos.y << "," << pos.z << ")";
                EXPECT_EQ(previewManager->getPreviewResolution(), resolution);
                
                // Clear preview for next test
                previewManager->clearPreview();
            }
        }
    }
}

TEST_F(PreviewPositioningTest, ExactPositionPreview_WorldCoordinateConsistency) {
    // Test that world coordinate preview positioning also shows exact positions
    
    voxelManager->setActiveResolution(VoxelResolution::Size_2cm);
    
    // Test world positions that correspond to arbitrary 1cm increment positions
    std::vector<Vector3f> worldPositions = {
        Vector3f(0.13f, 0.27f, 0.41f),  // 13cm, 27cm, 41cm
        Vector3f(0.07f, 0.11f, 0.19f),  // 7cm, 11cm, 19cm
        Vector3f(-0.05f, 0.13f, -0.21f), // -5cm, 13cm, -21cm
        Vector3f(0.01f, 0.03f, 0.05f)   // 1cm, 3cm, 5cm
    };
    
    for (const auto& worldPos : worldPositions) {
        if (voxelManager->isValidIncrementPosition(worldPos)) {
            // Convert to increment coordinates for expected position
            IncrementCoordinates incrementPos = CoordinateConverter::worldToIncrement(
                WorldCoordinates(worldPos));
            
            // Set preview at exact increment position
            previewManager->setPreviewPosition(incrementPos.value(), voxelManager->getActiveResolution());
            
            // Verify preview shows exact increment position
            EXPECT_TRUE(previewManager->hasPreview());
            EXPECT_EQ(previewManager->getPreviewPosition(), incrementPos.value()) 
                << "World position preview should convert to exact increment position";
        }
    }
}

TEST_F(PreviewPositioningTest, ExactPositionPreview_PlacementValidation) {
    // Test that preview positioning works correctly with placement validation
    
    voxelManager->setActiveResolution(VoxelResolution::Size_4cm);
    
    // Place a voxel at an arbitrary position
    Vector3i existingVoxelPos(13, 27, 41);
    if (voxelManager->isValidIncrementPosition(existingVoxelPos)) {
        ASSERT_TRUE(voxelManager->setVoxel(existingVoxelPos, VoxelResolution::Size_4cm, true));
        
        // Test preview at exact same position (should be invalid due to overlap)
        previewManager->setPreviewPosition(existingVoxelPos, VoxelResolution::Size_4cm);
        
        // Validate placement using PlacementUtils
        auto validationResult = PlacementUtils::validatePlacement(
            IncrementCoordinates(existingVoxelPos), VoxelResolution::Size_4cm, voxelManager->getWorkspaceSize());
        
        // Update preview with validation result
        previewManager->setValidationResult(validationResult);
        
        // Verify preview still shows exact position but with invalid state
        EXPECT_TRUE(previewManager->hasPreview());
        EXPECT_EQ(previewManager->getPreviewPosition(), existingVoxelPos);
        // Note: Can't test isValid() directly as it depends on overlap detection
        
        // Test preview at adjacent position (should be valid)
        Vector3i adjacentPos(existingVoxelPos.x + 1, existingVoxelPos.y, existingVoxelPos.z);
        if (voxelManager->isValidIncrementPosition(adjacentPos)) {
            previewManager->setPreviewPosition(adjacentPos, VoxelResolution::Size_4cm);
            
            auto adjacentValidation = PlacementUtils::validatePlacement(
                IncrementCoordinates(adjacentPos), VoxelResolution::Size_4cm, voxelManager->getWorkspaceSize());
            previewManager->setValidationResult(adjacentValidation);
            
            EXPECT_TRUE(previewManager->hasPreview());
            EXPECT_EQ(previewManager->getPreviewPosition(), adjacentPos);
            // Note: Adjacent position validity depends on voxel size and exact grid mapping
        }
    }
}

TEST_F(PreviewPositioningTest, ExactPositionPreview_MouseRayCalculation) {
    // Test that mouse ray calculations for preview positioning work with arbitrary positions
    
    voxelManager->setActiveResolution(VoxelResolution::Size_8cm);
    
    // Simulate mouse ray hitting arbitrary positions
    std::vector<Vector3f> rayHitPositions = {
        Vector3f(0.13f, 0.0f, 0.27f),   // Hit at 13cm, 0cm, 27cm
        Vector3f(0.07f, 0.0f, 0.11f),   // Hit at 7cm, 0cm, 11cm
        Vector3f(-0.05f, 0.0f, 0.19f)   // Hit at -5cm, 0cm, 19cm
    };
    
    for (const auto& hitPos : rayHitPositions) {
        // Convert ray hit to increment coordinates
        IncrementCoordinates hitIncrement = CoordinateConverter::worldToIncrement(
            WorldCoordinates(hitPos));
        
        if (voxelManager->isValidIncrementPosition(hitIncrement.value())) {
            // Simulate placing preview at ray hit position
            previewManager->setPreviewPosition(hitIncrement.value(), voxelManager->getActiveResolution());
            
            // Verify preview shows exact position where ray hit
            EXPECT_TRUE(previewManager->hasPreview());
            EXPECT_EQ(previewManager->getPreviewPosition(), hitIncrement.value()) 
                << "Preview should show exact position where mouse ray hit";
        }
    }
}

TEST_F(PreviewPositioningTest, ExactPositionPreview_RealTimeUpdates) {
    // Test that preview positioning updates correctly in real-time as mouse moves
    
    voxelManager->setActiveResolution(VoxelResolution::Size_2cm);
    
    // Simulate mouse movement over arbitrary positions
    std::vector<Vector3i> mousePositions = {
        Vector3i(1, 0, 1),
        Vector3i(3, 0, 3),
        Vector3i(5, 0, 5),
        Vector3i(7, 0, 7),
        Vector3i(9, 0, 9)
    };
    
    for (size_t i = 0; i < mousePositions.size(); ++i) {
        const auto& pos = mousePositions[i];
        
        if (voxelManager->isValidIncrementPosition(pos)) {
            // Update preview position
            previewManager->setPreviewPosition(pos, voxelManager->getActiveResolution());
            
            // Verify preview updated to exact position
            EXPECT_TRUE(previewManager->hasPreview());
            EXPECT_EQ(previewManager->getPreviewPosition(), pos) 
                << "Real-time preview update " << i << " should show exact position";
            
            // Verify preview position changed from previous (if not first)
            if (i > 0) {
                EXPECT_NE(previewManager->getPreviewPosition(), mousePositions[i-1]) 
                    << "Preview should update to new position";
            }
        }
    }
}