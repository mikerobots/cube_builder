#include <iostream>
#include "core/visual_feedback/include/visual_feedback/FaceDetector.h"
#include "foundation/math/Vector3f.h"
#include "foundation/math/CoordinateConverter.h"
#include "core/voxel_data/include/voxel_data/VoxelTypes.h"

using namespace VoxelEditor;

int main() {
    // Test the coordinate conversions
    Math::IncrementCoordinates voxelPos(0, 0, 0);
    Math::WorldCoordinates worldPos = Math::CoordinateConverter::incrementToWorld(voxelPos);
    
    std::cout << "Voxel increment position: (0, 0, 0)" << std::endl;
    std::cout << "Voxel world position: (" << worldPos.value().x << ", " << worldPos.value().y << ", " << worldPos.value().z << ")" << std::endl;
    
    // Test face placement calculation
    VisualFeedback::Face topFace(voxelPos, VoxelData::VoxelResolution::Size_64cm, VisualFeedback::FaceDirection::PositiveY);
    VisualFeedback::FaceDetector detector;
    Math::IncrementCoordinates placementPos = detector.calculatePlacementPosition(topFace);
    
    std::cout << "Top face placement position: (" << placementPos.x() << ", " << placementPos.y() << ", " << placementPos.z() << ")" << std::endl;
    
    // Test bottom face too
    VisualFeedback::Face bottomFace(voxelPos, VoxelData::VoxelResolution::Size_64cm, VisualFeedback::FaceDirection::NegativeY);
    Math::IncrementCoordinates bottomPlacement = detector.calculatePlacementPosition(bottomFace);
    
    std::cout << "Bottom face placement position: (" << bottomPlacement.x() << ", " << bottomPlacement.y() << ", " << bottomPlacement.z() << ")" << std::endl;
    
    return 0;
}