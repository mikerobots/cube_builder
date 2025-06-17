#include "../include/visual_feedback/FaceDetector.h"
#include "../../foundation/logging/Logger.h"
#include <algorithm>
#include <limits>
#include <cmath>

namespace VoxelEditor {
namespace VisualFeedback {

FaceDetector::FaceDetector()
    : m_maxRayDistance(1000.0f) {
}

FaceDetector::~FaceDetector() = default;

// Enhancement: Ground plane detection
Face FaceDetector::detectGroundPlane(const Ray& ray) {
    // Check if ray intersects the Y=0 plane
    if (std::abs(ray.direction.y) < 0.0001f) {
        // Ray is parallel to ground plane
        return Face();
    }
    
    // Calculate intersection point
    float t = -ray.origin.y / ray.direction.y;
    
    if (t < 0) {
        // Intersection is behind the ray origin
        return Face();
    }
    
    Math::Vector3f hitPoint = ray.pointAt(t);
    
    // Check if hit point is within max distance
    if (t > m_maxRayDistance) {
        return Face();
    }
    
    // Create a ground plane face
    return Face::GroundPlane(hitPoint);
}

// Enhancement: Combined face/ground detection
Face FaceDetector::detectFaceOrGround(const Ray& ray, const VoxelData::VoxelGrid& grid,
                                     VoxelData::VoxelResolution resolution) {
    // First try to detect a voxel face
    Face voxelFace = detectFace(ray, grid, resolution);
    
    if (voxelFace.isValid()) {
        return voxelFace;
    }
    
    // If no voxel face hit, check ground plane
    return detectGroundPlane(ray);
}

Face FaceDetector::detectFace(const Ray& ray, const VoxelData::VoxelGrid& grid, 
                             VoxelData::VoxelResolution resolution) {
    RaycastHit hit = raycastVoxelGrid(ray, grid, resolution);
    
    // Log occasionally for debugging
    static int detectCount = 0;
    if (detectCount++ % 30 == 0) {
        Logging::Logger::getInstance().debugfc("FaceDetector",
            "detectFace: Ray origin=(%.2f,%.2f,%.2f) dir=(%.3f,%.3f,%.3f) hit=%s",
            ray.origin.x, ray.origin.y, ray.origin.z,
            ray.direction.x, ray.direction.y, ray.direction.z,
            hit.hit ? "true" : "false");
    }
    
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
    // Enhancement: Handle ground plane faces
    if (face.isGroundPlane()) {
        // For ground plane, snap to nearest grid position at Y=0
        Math::Vector3f hitPoint = face.getGroundPlaneHitPoint();
        
        // Snap to 1cm increments
        Math::Vector3i gridPos(
            static_cast<int>(std::round(hitPoint.x / 0.01f)),
            0,  // Always Y=0 for ground plane
            static_cast<int>(std::round(hitPoint.z / 0.01f))
        );
        
        return gridPos;
    }
    
    // Original voxel face logic
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
    
    // Track the closest hit
    struct HitInfo {
        Math::Vector3i voxel;
        float distance;
        Math::Vector3f normal;
        Face face;
        bool valid = false;
    } closestHit;
    
    // Track if we start inside a voxel for special handling
    Math::Vector3i startVoxel = traversal.current;
    bool startedInsideVoxel = false;
    
    // Check if we start inside a voxel
    if (traversal.current.x >= 0 && traversal.current.y >= 0 && traversal.current.z >= 0 &&
        traversal.current.x < grid.getGridDimensions().x &&
        traversal.current.y < grid.getGridDimensions().y &&
        traversal.current.z < grid.getGridDimensions().z) {
        startedInsideVoxel = grid.getVoxel(traversal.current);
    }
    
    // Track the previous voxel for exit detection
    Math::Vector3i previousVoxel = traversal.current;
    bool wasInsideStartVoxel = startedInsideVoxel;
    
    while (currentDistance < maxDistance) {
        // Check current voxel
        if (traversal.current.x >= 0 && traversal.current.y >= 0 && traversal.current.z >= 0) {
            bool isCurrentVoxel = grid.getVoxel(traversal.current);
            
            // Special case: ray started inside a voxel and we just exited it
            if (startedInsideVoxel && wasInsideStartVoxel && 
                (traversal.current.x != startVoxel.x || 
                 traversal.current.y != startVoxel.y || 
                 traversal.current.z != startVoxel.z)) {
                
                
                // We just exited the starting voxel - determine which face
                FaceDirection exitFace;
                if (traversal.current.x != previousVoxel.x) {
                    exitFace = (traversal.current.x > previousVoxel.x) ? 
                        FaceDirection::PositiveX : FaceDirection::NegativeX;
                } else if (traversal.current.y != previousVoxel.y) {
                    exitFace = (traversal.current.y > previousVoxel.y) ? 
                        FaceDirection::PositiveY : FaceDirection::NegativeY;
                } else {
                    exitFace = (traversal.current.z > previousVoxel.z) ? 
                        FaceDirection::PositiveZ : FaceDirection::NegativeZ;
                }
                
                // Create face for the exit
                closestHit.valid = true;
                closestHit.voxel = previousVoxel;
                closestHit.distance = currentDistance * voxelSize;
                closestHit.face = Face(previousVoxel, resolution, exitFace);
                
                // Set normal based on exit face
                switch (exitFace) {
                    case FaceDirection::PositiveX: closestHit.normal = Math::Vector3f(1, 0, 0); break;
                    case FaceDirection::NegativeX: closestHit.normal = Math::Vector3f(-1, 0, 0); break;
                    case FaceDirection::PositiveY: closestHit.normal = Math::Vector3f(0, 1, 0); break;
                    case FaceDirection::NegativeY: closestHit.normal = Math::Vector3f(0, -1, 0); break;
                    case FaceDirection::PositiveZ: closestHit.normal = Math::Vector3f(0, 0, 1); break;
                    case FaceDirection::NegativeZ: closestHit.normal = Math::Vector3f(0, 0, -1); break;
                }
                
                // Mark that we've handled the starting voxel
                wasInsideStartVoxel = false;
                
                // Return the exit face immediately, even if we hit another voxel
                break;
            }
            
            // Normal case: ray hits a voxel from outside
            if (isCurrentVoxel && !wasInsideStartVoxel) {
                // Calculate actual distance to this voxel
                Math::Vector3f voxelCenter(
                    traversal.current.x + 0.5f,
                    traversal.current.y + 0.5f,
                    traversal.current.z + 0.5f
                );
                
                // Calculate exact intersection with voxel box
                Math::BoundingBox voxelBox(
                    Math::Vector3f(traversal.current.x, traversal.current.y, traversal.current.z),
                    Math::Vector3f(traversal.current.x + 1, traversal.current.y + 1, traversal.current.z + 1)
                );
                
                float tMin, tMax;
                if (rayIntersectsBox(voxelRay, voxelBox, tMin, tMax)) {
                    float hitDistance = tMin * voxelSize;
                    
                    // Only update if this is closer than any previous hit
                    if (!closestHit.valid || hitDistance < closestHit.distance) {
                        closestHit.valid = true;
                        closestHit.voxel = traversal.current;
                        closestHit.distance = hitDistance;
                        
                        // Calculate hit point to determine which face was hit
                        Math::Vector3f hitPoint = voxelRay.origin + voxelRay.direction * tMin;
                        
                        // Determine which face based on hit point relative to voxel center
                        Math::Vector3f voxelMin(traversal.current.x, traversal.current.y, traversal.current.z);
                        Math::Vector3f voxelMax = voxelMin + Math::Vector3f(1, 1, 1);
                        
                        
                        // Check which face the hit point is closest to
                        const float epsilon = 0.001f;
                        
                        // For rays hitting exactly on edges/corners, we need to determine
                        // which face was actually entered based on ray direction
                        float xDist = std::min(std::abs(hitPoint.x - voxelMin.x), 
                                             std::abs(hitPoint.x - voxelMax.x));
                        float yDist = std::min(std::abs(hitPoint.y - voxelMin.y), 
                                             std::abs(hitPoint.y - voxelMax.y));
                        float zDist = std::min(std::abs(hitPoint.z - voxelMin.z), 
                                             std::abs(hitPoint.z - voxelMax.z));
                        
                        // Find which axis has the smallest distance (most likely the hit face)
                        if (zDist <= xDist && zDist <= yDist && zDist < epsilon) {
                            // Hit on Z face
                            if (std::abs(hitPoint.z - voxelMin.z) < epsilon) {
                                closestHit.normal = Math::Vector3f(0, 0, -1);
                                closestHit.face = Face(traversal.current, resolution, FaceDirection::NegativeZ);
                            } else {
                                closestHit.normal = Math::Vector3f(0, 0, 1);
                                closestHit.face = Face(traversal.current, resolution, FaceDirection::PositiveZ);
                            }
                        } else if (yDist <= xDist && yDist < epsilon) {
                            // Hit on Y face
                            if (std::abs(hitPoint.y - voxelMin.y) < epsilon) {
                                closestHit.normal = Math::Vector3f(0, -1, 0);
                                closestHit.face = Face(traversal.current, resolution, FaceDirection::NegativeY);
                            } else {
                                closestHit.normal = Math::Vector3f(0, 1, 0);
                                closestHit.face = Face(traversal.current, resolution, FaceDirection::PositiveY);
                            }
                        } else if (xDist < epsilon) {
                            // Hit on X face
                            if (std::abs(hitPoint.x - voxelMin.x) < epsilon) {
                                closestHit.normal = Math::Vector3f(-1, 0, 0);
                                closestHit.face = Face(traversal.current, resolution, FaceDirection::NegativeX);
                            } else {
                                closestHit.normal = Math::Vector3f(1, 0, 0);
                                closestHit.face = Face(traversal.current, resolution, FaceDirection::PositiveX);
                            }
                        }
                        
                        // Log which voxel was hit
                        Logging::Logger::getInstance().debugfc("FaceDetector",
                            "Found voxel at (%d,%d,%d) distance=%.3f",
                            traversal.current.x, traversal.current.y, traversal.current.z,
                            closestHit.distance);
                    }
                }
            }
        }
        
        // Update tracking variables
        previousVoxel = traversal.current;
        
        // Step to next voxel
        stepTraversal(traversal);
        currentDistance = std::min({traversal.tMax.x, traversal.tMax.y, traversal.tMax.z});
    }
    
    // Return the closest hit if we found one
    if (closestHit.valid) {
        result.hit = true;
        result.distance = closestHit.distance;
        result.position = ray.pointAt(result.distance);
        result.normal = closestHit.normal;
        result.face = closestHit.face;
        
        Logging::Logger::getInstance().debugfc("FaceDetector",
            "Returning closest hit: voxel (%d,%d,%d) at distance %.3f",
            closestHit.voxel.x, closestHit.voxel.y, closestHit.voxel.z,
            closestHit.distance);
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
    // Starting voxel - ray is already in voxel space, so voxelSize should be 1.0
    // Add small epsilon to handle edge cases where ray origin is exactly on voxel boundary
    const float nudge = 0.0001f;
    traversal.current = Math::Vector3i(
        static_cast<int>(std::floor(ray.origin.x - gridMin.x + nudge)),
        static_cast<int>(std::floor(ray.origin.y - gridMin.y + nudge)),
        static_cast<int>(std::floor(ray.origin.z - gridMin.z + nudge))
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
        float nextX = gridMin.x + (traversal.current.x + (ray.direction.x > 0 ? 1 : 0));
        traversal.tMax.x = (nextX - ray.origin.x) / ray.direction.x;
        traversal.tDelta.x = 1.0f / std::abs(ray.direction.x);
    } else {
        traversal.tMax.x = std::numeric_limits<float>::max();
        traversal.tDelta.x = std::numeric_limits<float>::max();
    }
    
    if (std::abs(ray.direction.y) > epsilon) {
        float nextY = gridMin.y + (traversal.current.y + (ray.direction.y > 0 ? 1 : 0));
        traversal.tMax.y = (nextY - ray.origin.y) / ray.direction.y;
        traversal.tDelta.y = 1.0f / std::abs(ray.direction.y);
    } else {
        traversal.tMax.y = std::numeric_limits<float>::max();
        traversal.tDelta.y = std::numeric_limits<float>::max();
    }
    
    if (std::abs(ray.direction.z) > epsilon) {
        float nextZ = gridMin.z + (traversal.current.z + (ray.direction.z > 0 ? 1 : 0));
        traversal.tMax.z = (nextZ - ray.origin.z) / ray.direction.z;
        traversal.tDelta.z = 1.0f / std::abs(ray.direction.z);
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