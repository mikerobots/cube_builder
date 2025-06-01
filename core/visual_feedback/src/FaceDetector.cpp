#include "../include/visual_feedback/FaceDetector.h"
#include <algorithm>
#include <limits>
#include <cmath>

namespace VoxelEditor {
namespace VisualFeedback {

FaceDetector::FaceDetector()
    : m_maxRayDistance(1000.0f) {
}

FaceDetector::~FaceDetector() = default;

Face FaceDetector::detectFace(const Ray& ray, const VoxelData::VoxelGrid& grid, 
                             VoxelData::VoxelResolution resolution) {
    RaycastHit hit = raycastVoxelGrid(ray, grid, resolution);
    
    if (hit.hit) {
        return createFaceFromHit(hit, resolution);
    }
    
    return Face(); // Invalid face
}

std::vector<Face> FaceDetector::detectFacesInRegion(const Math::BoundingBox& region, 
                                                   const VoxelData::VoxelGrid& grid, 
                                                   VoxelData::VoxelResolution resolution) {
    std::vector<Face> faces;
    
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    // Convert region to voxel coordinates
    Math::Vector3i minVoxel(
        static_cast<int>(std::floor(region.min.x / voxelSize)),
        static_cast<int>(std::floor(region.min.y / voxelSize)),
        static_cast<int>(std::floor(region.min.z / voxelSize))
    );
    
    Math::Vector3i maxVoxel(
        static_cast<int>(std::ceil(region.max.x / voxelSize)),
        static_cast<int>(std::ceil(region.max.y / voxelSize)),
        static_cast<int>(std::ceil(region.max.z / voxelSize))
    );
    
    // Check all voxels in region
    for (int z = minVoxel.z; z <= maxVoxel.z; ++z) {
        for (int y = minVoxel.y; y <= maxVoxel.y; ++y) {
            for (int x = minVoxel.x; x <= maxVoxel.x; ++x) {
                Math::Vector3i pos(x, y, z);
                
                if (grid.getVoxel(pos)) {
                    // Check all 6 faces
                    for (int dir = 0; dir < 6; ++dir) {
                        Face face(pos, resolution, static_cast<FaceDirection>(dir));
                        if (isValidFaceForPlacement(face, grid)) {
                            faces.push_back(face);
                        }
                    }
                }
            }
        }
    }
    
    return faces;
}

bool FaceDetector::isValidFaceForPlacement(const Face& face, const VoxelData::VoxelGrid& grid) {
    // Check if the voxel on the face normal side is empty
    Math::Vector3i adjacentPos = calculatePlacementPosition(face);
    return !grid.getVoxel(adjacentPos);
}

Math::Vector3i FaceDetector::calculatePlacementPosition(const Face& face) {
    Math::Vector3i pos = face.getVoxelPosition();
    
    switch (face.getDirection()) {
        case FaceDirection::PositiveX: pos.x += 1; break;
        case FaceDirection::NegativeX: pos.x -= 1; break;
        case FaceDirection::PositiveY: pos.y += 1; break;
        case FaceDirection::NegativeY: pos.y -= 1; break;
        case FaceDirection::PositiveZ: pos.z += 1; break;
        case FaceDirection::NegativeZ: pos.z -= 1; break;
    }
    
    return pos;
}

RaycastHit FaceDetector::raycastVoxelGrid(const Ray& ray, const VoxelData::VoxelGrid& grid, 
                                         VoxelData::VoxelResolution resolution) {
    RaycastHit result;
    result.hit = false;
    
    float voxelSize = VoxelData::getVoxelSize(resolution);
    Math::Vector3f gridMin(0, 0, 0);
    Math::Vector3f gridMax = grid.getWorkspaceSize();
    
    // Convert to voxel space
    Ray voxelRay;
    voxelRay.origin = ray.origin / voxelSize;
    voxelRay.direction = ray.direction;
    
    // Check if ray intersects grid bounds
    float tMin, tMax;
    Math::BoundingBox gridBounds(gridMin / voxelSize, gridMax / voxelSize);
    if (!rayIntersectsBox(voxelRay, gridBounds, tMin, tMax)) {
        return result;
    }
    
    // Initialize traversal
    GridTraversal traversal;
    initializeTraversal(voxelRay, gridMin / voxelSize, 1.0f, traversal);
    
    // Traverse grid
    float maxDistance = m_maxRayDistance / voxelSize;
    float currentDistance = 0.0f;
    
    while (currentDistance < maxDistance) {
        // Check current voxel
        if (traversal.current.x >= 0 && traversal.current.y >= 0 && traversal.current.z >= 0) {
            if (grid.getVoxel(traversal.current)) {
                // Found hit
                result.hit = true;
                result.distance = currentDistance * voxelSize;
                result.position = ray.pointAt(result.distance);
                
                // Determine which face was hit
                float minT = std::min({traversal.tMax.x, traversal.tMax.y, traversal.tMax.z});
                if (traversal.tMax.x == minT) {
                    result.normal = traversal.step.x > 0 ? 
                        Math::Vector3f(-1, 0, 0) : Math::Vector3f(1, 0, 0);
                    result.face = Face(traversal.current, resolution, 
                        traversal.step.x > 0 ? FaceDirection::NegativeX : FaceDirection::PositiveX);
                } else if (traversal.tMax.y == minT) {
                    result.normal = traversal.step.y > 0 ? 
                        Math::Vector3f(0, -1, 0) : Math::Vector3f(0, 1, 0);
                    result.face = Face(traversal.current, resolution,
                        traversal.step.y > 0 ? FaceDirection::NegativeY : FaceDirection::PositiveY);
                } else {
                    result.normal = traversal.step.z > 0 ? 
                        Math::Vector3f(0, 0, -1) : Math::Vector3f(0, 0, 1);
                    result.face = Face(traversal.current, resolution,
                        traversal.step.z > 0 ? FaceDirection::NegativeZ : FaceDirection::PositiveZ);
                }
                
                return result;
            }
        }
        
        // Step to next voxel
        stepTraversal(traversal);
        currentDistance = std::min({traversal.tMax.x, traversal.tMax.y, traversal.tMax.z});
    }
    
    return result;
}

Face FaceDetector::createFaceFromHit(const RaycastHit& hit, VoxelData::VoxelResolution resolution) {
    return hit.face;
}

bool FaceDetector::isValidFace(const Face& face, const VoxelData::VoxelGrid& grid) {
    // Check if the face's voxel exists
    return grid.getVoxel(face.getVoxelPosition());
}

bool FaceDetector::rayIntersectsBox(const Ray& ray, const Math::BoundingBox& box, 
                                   float& tMin, float& tMax) const {
    Math::Vector3f invDir(
        1.0f / (std::abs(ray.direction.x) < 0.0001f ? 0.0001f : ray.direction.x),
        1.0f / (std::abs(ray.direction.y) < 0.0001f ? 0.0001f : ray.direction.y),
        1.0f / (std::abs(ray.direction.z) < 0.0001f ? 0.0001f : ray.direction.z)
    );
    
    Math::Vector3f t1 = (box.min - ray.origin) * invDir;
    Math::Vector3f t2 = (box.max - ray.origin) * invDir;
    
    Math::Vector3f tMinVec(std::min(t1.x, t2.x), std::min(t1.y, t2.y), std::min(t1.z, t2.z));
    Math::Vector3f tMaxVec(std::max(t1.x, t2.x), std::max(t1.y, t2.y), std::max(t1.z, t2.z));
    
    tMin = std::max({tMinVec.x, tMinVec.y, tMinVec.z, 0.0f});
    tMax = std::min({tMaxVec.x, tMaxVec.y, tMaxVec.z});
    
    return tMin <= tMax && tMax >= 0.0f;
}

void FaceDetector::initializeTraversal(const Ray& ray, const Math::Vector3f& gridMin, 
                                      float voxelSize, GridTraversal& traversal) const {
    // Starting voxel
    traversal.current = Math::Vector3i(
        static_cast<int>(std::floor((ray.origin.x - gridMin.x) / voxelSize)),
        static_cast<int>(std::floor((ray.origin.y - gridMin.y) / voxelSize)),
        static_cast<int>(std::floor((ray.origin.z - gridMin.z) / voxelSize))
    );
    
    // Step direction
    traversal.step = Math::Vector3i(
        ray.direction.x > 0 ? 1 : -1,
        ray.direction.y > 0 ? 1 : -1,
        ray.direction.z > 0 ? 1 : -1
    );
    
    // Calculate tMax and tDelta
    const float epsilon = 0.0001f;
    
    if (std::abs(ray.direction.x) > epsilon) {
        float nextX = gridMin.x + (traversal.current.x + (ray.direction.x > 0 ? 1 : 0)) * voxelSize;
        traversal.tMax.x = (nextX - ray.origin.x) / ray.direction.x;
        traversal.tDelta.x = voxelSize / std::abs(ray.direction.x);
    } else {
        traversal.tMax.x = std::numeric_limits<float>::max();
        traversal.tDelta.x = std::numeric_limits<float>::max();
    }
    
    if (std::abs(ray.direction.y) > epsilon) {
        float nextY = gridMin.y + (traversal.current.y + (ray.direction.y > 0 ? 1 : 0)) * voxelSize;
        traversal.tMax.y = (nextY - ray.origin.y) / ray.direction.y;
        traversal.tDelta.y = voxelSize / std::abs(ray.direction.y);
    } else {
        traversal.tMax.y = std::numeric_limits<float>::max();
        traversal.tDelta.y = std::numeric_limits<float>::max();
    }
    
    if (std::abs(ray.direction.z) > epsilon) {
        float nextZ = gridMin.z + (traversal.current.z + (ray.direction.z > 0 ? 1 : 0)) * voxelSize;
        traversal.tMax.z = (nextZ - ray.origin.z) / ray.direction.z;
        traversal.tDelta.z = voxelSize / std::abs(ray.direction.z);
    } else {
        traversal.tMax.z = std::numeric_limits<float>::max();
        traversal.tDelta.z = std::numeric_limits<float>::max();
    }
}

void FaceDetector::stepTraversal(GridTraversal& traversal) const {
    if (traversal.tMax.x < traversal.tMax.y) {
        if (traversal.tMax.x < traversal.tMax.z) {
            traversal.current.x += traversal.step.x;
            traversal.tMax.x += traversal.tDelta.x;
        } else {
            traversal.current.z += traversal.step.z;
            traversal.tMax.z += traversal.tDelta.z;
        }
    } else {
        if (traversal.tMax.y < traversal.tMax.z) {
            traversal.current.y += traversal.step.y;
            traversal.tMax.y += traversal.tDelta.y;
        } else {
            traversal.current.z += traversal.step.z;
            traversal.tMax.z += traversal.tDelta.z;
        }
    }
}

} // namespace VisualFeedback
} // namespace VoxelEditor