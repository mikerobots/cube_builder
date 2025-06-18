#include <iostream>
#include "core/visual_feedback/include/visual_feedback/FaceDetector.h"
#include "core/voxel_data/VoxelGrid.h"
#include "foundation/logging/Logger.h"

using namespace VoxelEditor;

int main() {
    // Set up logging
    Logging::Logger::getInstance().setLevel(Logging::LogLevel::Debug);
    
    // Create test setup similar to the failing test
    Math::Vector3f workspaceSize(10.0f, 10.0f, 10.0f);
    VoxelData::VoxelResolution resolution = VoxelData::VoxelResolution::Size_32cm;
    auto testGrid = std::make_unique<VoxelData::VoxelGrid>(resolution, workspaceSize);
    auto detector = std::make_unique<VisualFeedback::FaceDetector>();
    
    // Add test voxel
    testGrid->setVoxel(Math::Vector3i(5, 5, 5), true);
    
    // Calculate voxel position and create ray like the test does
    Math::Vector3f voxelWorldPos = testGrid->gridToWorld(Math::Vector3i(5, 5, 5));
    float voxelSize = VoxelData::getVoxelSize(resolution);
    Math::Vector3f voxelCenter = voxelWorldPos + Math::Vector3f(voxelSize * 0.5f, voxelSize * 0.5f, voxelSize * 0.5f);
    
    std::cout << "Workspace size: " << workspaceSize.x << ", " << workspaceSize.y << ", " << workspaceSize.z << std::endl;
    std::cout << "Voxel size: " << voxelSize << std::endl;
    std::cout << "Grid dimensions: " << testGrid->getGridDimensions().x << ", " << testGrid->getGridDimensions().y << ", " << testGrid->getGridDimensions().z << std::endl;
    std::cout << "Voxel world pos: " << voxelWorldPos.x << ", " << voxelWorldPos.y << ", " << voxelWorldPos.z << std::endl;
    std::cout << "Voxel center: " << voxelCenter.x << ", " << voxelCenter.y << ", " << voxelCenter.z << std::endl;
    
    // Create ray from in front of the voxel
    Math::Vector3f rayOrigin = Math::Vector3f(voxelCenter.x, voxelCenter.y, voxelCenter.z - 2.0f);
    VisualFeedback::Ray ray(rayOrigin, Math::Vector3f(0, 0, 1));
    
    std::cout << "Ray origin: " << rayOrigin.x << ", " << rayOrigin.y << ", " << rayOrigin.z << std::endl;
    std::cout << "Ray direction: " << ray.direction.x << ", " << ray.direction.y << ", " << ray.direction.z << std::endl;
    
    // Test ray detection
    VisualFeedback::Face face = detector->detectFace(ray, *testGrid, resolution);
    
    std::cout << "Face valid: " << (face.isValid() ? "true" : "false") << std::endl;
    if (face.isValid()) {
        Math::Vector3i pos = face.getVoxelPosition().value();
        std::cout << "Face position: " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
        std::cout << "Face direction: " << static_cast<int>(face.getDirection()) << std::endl;
    }
    
    return 0;
}