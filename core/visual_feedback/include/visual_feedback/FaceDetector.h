#pragma once

#include <vector>

#include "FeedbackTypes.h"
#include "voxel_data/VoxelGrid.h"

namespace VoxelEditor {
namespace VisualFeedback {

class FaceDetector {
public:
    FaceDetector();
    ~FaceDetector();
    
    // Face detection
    Face detectFace(const Ray& ray, const VoxelData::VoxelGrid& grid, 
                   VoxelData::VoxelResolution resolution);
    
    // Ground plane detection (Enhancement)
    Face detectGroundPlane(const Ray& ray);
    
    // Combined face/ground detection (Enhancement)
    Face detectFaceOrGround(const Ray& ray, const VoxelData::VoxelGrid& grid,
                           VoxelData::VoxelResolution resolution);
    
    std::vector<Face> detectFacesInRegion(const Math::BoundingBox& region, 
                                         const VoxelData::VoxelGrid& grid, 
                                         VoxelData::VoxelResolution resolution);
    
    // Placement validation
    bool isValidFaceForPlacement(const Face& face, const VoxelData::VoxelGrid& grid);
    Math::IncrementCoordinates calculatePlacementPosition(const Face& face);
    
    // Configuration
    void setMaxRayDistance(float distance) { m_maxRayDistance = distance; }
    float getMaxRayDistance() const { return m_maxRayDistance; }
    
    // Additional methods (for requirements tests)
    bool isFaceVisible(const Face& face) const;
    Rendering::Color getFaceHighlightColor(const Face& face) const;
    bool validateFace(const Face& face, const VoxelData::VoxelGrid& grid) const;
    bool hasActiveHighlight() const;
    
private:
    float m_maxRayDistance;
    bool m_hasActiveHighlight = false;
    
    // Ray-voxel intersection
    RaycastHit raycastVoxelGrid(const Ray& ray, const VoxelData::VoxelGrid& grid, 
                               VoxelData::VoxelResolution resolution);
    
    // Face creation
    Face createFaceFromHit(const RaycastHit& hit, VoxelData::VoxelResolution resolution);
    bool isValidFace(const Face& face, const VoxelData::VoxelGrid& grid) const;
    
    // Ray-box intersection
    bool rayIntersectsBox(const Ray& ray, const Math::BoundingBox& box, 
                         float& tMin, float& tMax) const;
    
    // Grid traversal
    struct GridTraversal {
        Math::IncrementCoordinates current;
        Math::Vector3i step;  // Step remains as Vector3i (just -1, 0, or 1)
        Math::Vector3f tMax;
        Math::Vector3f tDelta;
    };
    
    void initializeTraversal(const Ray& ray, const Math::WorldCoordinates& gridMin, 
                           float voxelSize, VoxelData::VoxelResolution resolution, GridTraversal& traversal) const;
    void stepTraversal(GridTraversal& traversal) const;
};

} // namespace VisualFeedback
} // namespace VoxelEditor