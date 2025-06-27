#include <iostream>
#include <memory>
#include "visual_feedback/FaceDetector.h"
#include "voxel_data/VoxelDataManager.h"
#include "math/CoordinateTypes.h"
#include "math/CoordinateConverter.h"

using namespace VoxelEditor;

int main() {
    // Create face detector and voxel data manager
    auto faceDetector = std::make_unique<VisualFeedback::FaceDetector>();
    auto voxelDataManager = std::make_unique<VoxelData::VoxelDataManager>();
    
    // Set workspace size to 5m x 5m x 5m
    voxelDataManager->resizeWorkspace(Math::Vector3f(5.0f, 5.0f, 5.0f));
    
    auto workspaceSize = voxelDataManager->getWorkspaceSize();
    float halfX = workspaceSize.x * 0.5f;
    
    // Place a voxel near the positive X boundary
    auto resolution = VoxelData::VoxelResolution::Size_16cm;
    voxelDataManager->setActiveResolution(resolution);
    
    Math::IncrementCoordinates voxelPos = Math::CoordinateConverter::worldToIncrement(
        Math::WorldCoordinates(halfX - 0.16f, 0.0f, 0.0f)
    );
    
    std::cout << "Workspace size: " << workspaceSize.x << " x " << workspaceSize.y << " x " << workspaceSize.z << std::endl;
    std::cout << "Half X: " << halfX << std::endl;
    std::cout << "Voxel world position: " << (halfX - 0.16f) << ", 0, 0" << std::endl;
    std::cout << "Voxel increment position: " << voxelPos.x() << ", " << voxelPos.y() << ", " << voxelPos.z() << std::endl;
    
    bool placed = voxelDataManager->setVoxel(voxelPos, resolution, true);
    std::cout << "Voxel placed: " << (placed ? "true" : "false") << std::endl;
    
    // Get the grid
    auto* grid = voxelDataManager->getGrid(resolution);
    if (!grid) {
        std::cerr << "Failed to get grid!" << std::endl;
        return 1;
    }
    
    // Create ray from outside +X boundary
    VisualFeedback::Ray ray(
        Math::WorldCoordinates(halfX + 1.0f, 0.5f, 0.0f),
        Math::Vector3f(-1.0f, 0.0f, 0.0f)
    );
    
    std::cout << "\nRay origin: " << ray.origin.x() << ", " << ray.origin.y() << ", " << ray.origin.z() << std::endl;
    std::cout << "Ray direction: " << ray.direction.x << ", " << ray.direction.y << ", " << ray.direction.z << std::endl;
    
    // Test ray-box intersection with workspace
    Math::Vector3f gridMin(-workspaceSize.x * 0.5f, 0.0f, -workspaceSize.z * 0.5f);
    Math::Vector3f gridMax(workspaceSize.x * 0.5f, workspaceSize.y, workspaceSize.z * 0.5f);
    Math::BoundingBox workspaceBounds(gridMin, gridMax);
    
    std::cout << "\nWorkspace bounds:" << std::endl;
    std::cout << "  Min: " << gridMin.x << ", " << gridMin.y << ", " << gridMin.z << std::endl;
    std::cout << "  Max: " << gridMax.x << ", " << gridMax.y << ", " << gridMax.z << std::endl;
    
    // Detect face
    auto result = faceDetector->detectFace(ray, *grid, resolution);
    
    std::cout << "\nFace detection result: " << (result.isValid() ? "Valid" : "Invalid") << std::endl;
    
    if (result.isValid()) {
        auto detectedPos = result.getVoxelPosition();
        std::cout << "Detected voxel at: " << detectedPos.x() << ", " << detectedPos.y() << ", " << detectedPos.z() << std::endl;
    }
    
    return 0;
}