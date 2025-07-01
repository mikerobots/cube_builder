#include <gtest/gtest.h>
#include <memory>
#include "voxel_data/VoxelDataManager.h"
#include "voxel_data/VoxelTypes.h"
#include "math/Vector3f.h"
#include "math/Vector3i.h"
#include "math/CoordinateConverter.h"
#include "events/EventDispatcher.h"
#include "logging/Logger.h"
#include "input/PlacementValidation.h"
#include <iostream>

namespace VoxelEditor {
namespace Tests {

class Debug192cmTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup logging to console
        auto& logger = Logging::Logger::getInstance();
        logger.setLevel(Logging::LogLevel::Debug);
        logger.clearOutputs();
        logger.addOutput(std::make_unique<Logging::ConsoleOutput>());
        
        // Create event dispatcher and voxel manager
        eventDispatcher = std::make_unique<Events::EventDispatcher>();
        voxelManager = std::make_unique<VoxelData::VoxelDataManager>(eventDispatcher.get());
    }
    
    void TearDown() override {
        voxelManager.reset();
        eventDispatcher.reset();
    }
    
    std::unique_ptr<Events::EventDispatcher> eventDispatcher;
    std::unique_ptr<VoxelData::VoxelDataManager> voxelManager;
};

TEST_F(Debug192cmTest, Debug192cmPlacement) {
    // Check default workspace size (should be 5x5x5)
    Math::Vector3f workspaceSize = voxelManager->getWorkspaceSize();
    std::cout << "Default workspace size: " << workspaceSize.x << "x" << workspaceSize.y << "x" << workspaceSize.z << " meters" << std::endl;
    
    // Calculate bounds just like PlacementValidation.cpp does
    int halfX_cm = static_cast<int>(workspaceSize.x * Math::CoordinateConverter::METERS_TO_CM * 0.5f);
    int halfZ_cm = static_cast<int>(workspaceSize.z * Math::CoordinateConverter::METERS_TO_CM * 0.5f);
    int height_cm = static_cast<int>(workspaceSize.y * Math::CoordinateConverter::METERS_TO_CM);
    
    std::cout << "Calculated bounds:" << std::endl;
    std::cout << "  X: -" << halfX_cm << " to +" << halfX_cm << " cm" << std::endl;
    std::cout << "  Y: 0 to " << height_cm << " cm" << std::endl;
    std::cout << "  Z: -" << halfZ_cm << " to +" << halfZ_cm << " cm" << std::endl;
    
    // Test placement at 192cm (should be valid)
    Math::IncrementCoordinates testPos(192, 0, 0);
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_1cm;
    
    std::cout << "\nTesting placement at (192, 0, 0) with 1cm resolution:" << std::endl;
    
    // Check using PlacementValidation directly
    Input::PlacementValidationResult result = Input::PlacementUtils::validatePlacement(testPos, resolution, workspaceSize);
    
    std::cout << "PlacementValidation result: ";
    switch (result) {
        case Input::PlacementValidationResult::Valid:
            std::cout << "Valid" << std::endl;
            break;
        case Input::PlacementValidationResult::InvalidOutOfBounds:
            std::cout << "InvalidOutOfBounds" << std::endl;
            break;
        case Input::PlacementValidationResult::InvalidYBelowZero:
            std::cout << "InvalidYBelowZero" << std::endl;
            break;
        case Input::PlacementValidationResult::InvalidOverlap:
            std::cout << "InvalidOverlap" << std::endl;
            break;
        case Input::PlacementValidationResult::InvalidPosition:
            std::cout << "InvalidPosition" << std::endl;
            break;
    }
    
    // Manual bounds check calculation to verify the logic
    float voxelSize = VoxelData::getVoxelSize(resolution);
    int voxelSize_cm = static_cast<int>(voxelSize * Math::CoordinateConverter::METERS_TO_CM);
    
    std::cout << "\nManual bounds check for position (192, 0, 0):" << std::endl;
    std::cout << "  Voxel size: " << voxelSize_cm << " cm" << std::endl;
    
    // This is the exact logic from PlacementValidation.cpp line 49-53
    bool xOutOfBounds = (testPos.x() < -halfX_cm || testPos.x() + voxelSize_cm > halfX_cm);
    bool yOutOfBounds = (testPos.y() < 0 || testPos.y() + voxelSize_cm > height_cm);
    bool zOutOfBounds = (testPos.z() < -halfZ_cm || testPos.z() + voxelSize_cm > halfZ_cm);
    
    std::cout << "  X bounds: " << testPos.x() << " < -" << halfX_cm << " = " << (testPos.x() < -halfX_cm) << std::endl;
    std::cout << "  X bounds: " << testPos.x() << " + " << voxelSize_cm << " > " << halfX_cm << " = " << (testPos.x() + voxelSize_cm > halfX_cm) << std::endl;
    std::cout << "  X out of bounds: " << xOutOfBounds << std::endl;
    
    std::cout << "  Y bounds: " << testPos.y() << " < 0 = " << (testPos.y() < 0) << std::endl;
    std::cout << "  Y bounds: " << testPos.y() << " + " << voxelSize_cm << " > " << height_cm << " = " << (testPos.y() + voxelSize_cm > height_cm) << std::endl;
    std::cout << "  Y out of bounds: " << yOutOfBounds << std::endl;
    
    std::cout << "  Z bounds: " << testPos.z() << " < -" << halfZ_cm << " = " << (testPos.z() < -halfZ_cm) << std::endl;
    std::cout << "  Z bounds: " << testPos.z() << " + " << voxelSize_cm << " > " << halfZ_cm << " = " << (testPos.z() + voxelSize_cm > halfZ_cm) << std::endl;
    std::cout << "  Z out of bounds: " << zOutOfBounds << std::endl;
    
    bool isOutOfBounds = xOutOfBounds || yOutOfBounds || zOutOfBounds;
    std::cout << "  Overall out of bounds: " << isOutOfBounds << std::endl;
    
    // Test using CoordinateConverter (this uses a different method)
    bool coordValid = Math::CoordinateConverter::isValidIncrementCoordinate(testPos, workspaceSize);
    std::cout << "  CoordinateConverter validation: " << (coordValid ? "Valid" : "Invalid") << std::endl;
    
    // Test using VoxelDataManager directly
    bool managerValid = voxelManager->isValidIncrementPosition(testPos);
    std::cout << "  VoxelDataManager validation: " << (managerValid ? "Valid" : "Invalid") << std::endl;
    
    // Try actual placement
    bool placementSuccess = voxelManager->setVoxel(testPos, resolution, true);
    std::cout << "  Actual placement result: " << (placementSuccess ? "Success" : "Failed") << std::endl;
    
    // For debugging - let's also test edge cases
    std::cout << "\nTesting boundary cases:" << std::endl;
    
    // Test exactly at boundary
    Math::IncrementCoordinates boundaryPos(250, 0, 0);
    Input::PlacementValidationResult boundaryResult = Input::PlacementUtils::validatePlacement(boundaryPos, resolution, workspaceSize);
    std::cout << "  Position (250, 0, 0): " << (boundaryResult == Input::PlacementValidationResult::Valid ? "Valid" : "Invalid") << std::endl;
    
    // Test just over boundary
    Math::IncrementCoordinates overBoundaryPos(251, 0, 0);
    Input::PlacementValidationResult overBoundaryResult = Input::PlacementUtils::validatePlacement(overBoundaryPos, resolution, workspaceSize);
    std::cout << "  Position (251, 0, 0): " << (overBoundaryResult == Input::PlacementValidationResult::Valid ? "Valid" : "Invalid") << std::endl;
    
    // The placement at 192cm should succeed for a 5x5x5 workspace
    // Since 192 + 1 = 193 < 250, it should be valid
    EXPECT_EQ(result, Input::PlacementValidationResult::Valid) << "192cm should be valid in 5x5x5 workspace";
}

}
}