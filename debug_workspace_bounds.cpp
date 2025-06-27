#include <iostream>
#include <memory>
#include "core/voxel_data/VoxelDataManager.h"
#include "core/voxel_data/VoxelTypes.h"
#include "foundation/math/Vector3f.h"
#include "foundation/math/Vector3i.h"
#include "foundation/math/CoordinateConverter.h"
#include "foundation/events/EventDispatcher.h"
#include "foundation/logging/Logger.h"
#include "core/input/PlacementValidation.h"

using namespace VoxelEditor;

int main() {
    // Set up logging
    auto& logger = Logging::Logger::getInstance();
    logger.setLevel(Logging::LogLevel::Debug);
    logger.clearOutputs();
    logger.addOutput(std::make_unique<Logging::ConsoleOutput>());
    
    // Create event dispatcher and voxel manager
    auto eventDispatcher = std::make_unique<Events::EventDispatcher>();
    auto voxelManager = std::make_unique<VoxelData::VoxelDataManager>(eventDispatcher.get());
    
    // Check default workspace size
    Math::Vector3f workspaceSize = voxelManager->getWorkspaceSize();
    std::cout << "Default workspace size: " << workspaceSize.x << "x" << workspaceSize.y << "x" << workspaceSize.z << " meters" << std::endl;
    
    // Calculate bounds
    int halfX_cm = static_cast<int>(workspaceSize.x * 100.0f * 0.5f);
    int halfZ_cm = static_cast<int>(workspaceSize.z * 100.0f * 0.5f);
    int height_cm = static_cast<int>(workspaceSize.y * 100.0f);
    
    std::cout << "Calculated bounds:" << std::endl;
    std::cout << "  X: -" << halfX_cm << " to +" << halfX_cm << " cm" << std::endl;
    std::cout << "  Y: 0 to " << height_cm << " cm" << std::endl;
    std::cout << "  Z: -" << halfZ_cm << " to +" << halfZ_cm << " cm" << std::endl;
    
    // Test placement at 192cm
    Math::IncrementCoordinates testPos(192, 0, 0);
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_1cm;
    
    std::cout << "\nTesting placement at (192, 0, 0) with 1cm resolution:" << std::endl;
    
    // Check using PlacementValidation
    Input::PlacementValidationResult result = Input::PlacementUtils::validatePlacement(testPos, resolution, workspaceSize);
    
    std::cout << "PlacementValidation result: ";
    switch (result) {
        case Input::PlacementValidationResult::Valid:
            std::cout << "Valid";
            break;
        case Input::PlacementValidationResult::InvalidOutOfBounds:
            std::cout << "InvalidOutOfBounds";
            break;
        case Input::PlacementValidationResult::InvalidYBelowZero:
            std::cout << "InvalidYBelowZero";
            break;
        case Input::PlacementValidationResult::InvalidOverlap:
            std::cout << "InvalidOverlap";
            break;
        case Input::PlacementValidationResult::InvalidPosition:
            std::cout << "InvalidPosition";
            break;
    }
    std::cout << std::endl;
    
    // Check bounds calculation manually
    float voxelSize = VoxelData::getVoxelSize(resolution);
    int voxelSize_cm = static_cast<int>(voxelSize * 100.0f);
    
    std::cout << "\nManual bounds check for position (192, 0, 0):" << std::endl;
    std::cout << "  Voxel size: " << voxelSize_cm << " cm" << std::endl;
    std::cout << "  X check: " << testPos.x() << " < -" << halfX_cm << " ? " << (testPos.x() < -halfX_cm) << std::endl;
    std::cout << "  X check: " << testPos.x() << " + " << voxelSize_cm << " > " << halfX_cm << " ? " << (testPos.x() + voxelSize_cm > halfX_cm) << std::endl;
    std::cout << "  Y check: " << testPos.y() << " < 0 ? " << (testPos.y() < 0) << std::endl;
    std::cout << "  Y check: " << testPos.y() << " + " << voxelSize_cm << " > " << height_cm << " ? " << (testPos.y() + voxelSize_cm > height_cm) << std::endl;
    std::cout << "  Z check: " << testPos.z() << " < -" << halfZ_cm << " ? " << (testPos.z() < -halfZ_cm) << std::endl;
    std::cout << "  Z check: " << testPos.z() << " + " << voxelSize_cm << " > " << halfZ_cm << " ? " << (testPos.z() + voxelSize_cm > halfZ_cm) << std::endl;
    
    // Also test CoordinateConverter validation
    bool coordValid = Math::CoordinateConverter::isValidIncrementCoordinate(testPos, workspaceSize);
    std::cout << "\nCoordinateConverter validation: " << (coordValid ? "Valid" : "Invalid") << std::endl;
    
    return 0;
}