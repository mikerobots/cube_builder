#include "../include/visual_feedback/FaceDetector.h"
#include "../../foundation/logging/Logger.h"
#include <algorithm>
#include <limits>
#include <cmath>
#include <vector>

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
    float t = -ray.origin.y() / ray.direction.y;
    
    if (t < 0) {
        // Intersection is behind the ray origin
        return Face();
    }
    
    Math::WorldCoordinates hitPoint = ray.pointAt(t);
    
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
    int voxelSize_cm = static_cast<int>(voxelSize * 100.0f);
    
    // Convert region bounds to increment coordinates (centimeters)
    Math::IncrementCoordinates minIncrement = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(region.min));
    Math::IncrementCoordinates maxIncrement = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(region.max));
    
    // Snap to voxel grid boundaries for this resolution
    Math::IncrementCoordinates minVoxel = Math::CoordinateConverter::snapToVoxelResolution(minIncrement, resolution);
    Math::IncrementCoordinates maxVoxel = Math::CoordinateConverter::snapToVoxelResolution(maxIncrement, resolution);
    
    // Check all voxels in region at this resolution
    for (int z = minVoxel.z(); z <= maxVoxel.z(); z += voxelSize_cm) {
        for (int y = minVoxel.y(); y <= maxVoxel.y(); y += voxelSize_cm) {
            for (int x = minVoxel.x(); x <= maxVoxel.x(); x += voxelSize_cm) {
                Math::IncrementCoordinates voxelPos(x, y, z);
                
                // Check if this voxel exists
                if (grid.isValidIncrementPosition(voxelPos) && grid.getVoxel(voxelPos)) {
                    // Check all 6 faces
                    for (int dir = 0; dir < 6; ++dir) {
                        Face face(voxelPos, resolution, static_cast<FaceDirection>(dir));
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
    Math::IncrementCoordinates adjacentPos = calculatePlacementPosition(face);
    return !grid.getVoxel(adjacentPos);
}

Math::IncrementCoordinates FaceDetector::calculatePlacementPosition(const Face& face) {
    // Enhancement: Handle ground plane faces
    if (face.isGroundPlane()) {
        // For ground plane, use coordinate converter to snap to nearest increment position
        Math::WorldCoordinates hitPoint = face.getGroundPlaneHitPoint();
        
        // Convert to increment coordinates (which snaps to 1cm increments)
        Math::IncrementCoordinates incrementPos = Math::CoordinateConverter::worldToIncrement(hitPoint);
        
        // Ensure Y=0 for ground plane placement
        return Math::IncrementCoordinates(incrementPos.x(), 0, incrementPos.z());
    }
    
    // Original voxel face logic
    Math::IncrementCoordinates pos = face.getVoxelPosition();
    Math::Vector3i offset(0, 0, 0);
    
    // Calculate the voxel size in centimeters for proper offset
    int voxelSize_cm = static_cast<int>(VoxelData::getVoxelSize(face.getResolution()) * 100.0f);
    
    switch (face.getDirection()) {
        case FaceDirection::PositiveX: offset.x = voxelSize_cm; break;
        case FaceDirection::NegativeX: offset.x = -voxelSize_cm; break;
        case FaceDirection::PositiveY: offset.y = voxelSize_cm; break;
        case FaceDirection::NegativeY: offset.y = -voxelSize_cm; break;
        case FaceDirection::PositiveZ: offset.z = voxelSize_cm; break;
        case FaceDirection::NegativeZ: offset.z = -voxelSize_cm; break;
    }
    
    return Math::IncrementCoordinates(pos.value() + offset);
}

RaycastHit FaceDetector::raycastVoxelGrid(const Ray& ray, const VoxelData::VoxelGrid& grid, 
                                         VoxelData::VoxelResolution resolution) {
    RaycastHit result;
    result.hit = false;
    
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    // Get workspace bounds in world coordinates (centered)
    Math::Vector3f workspaceSize = grid.getWorkspaceSize();
    Math::Vector3f gridMin(-workspaceSize.x * 0.5f, 0.0f, -workspaceSize.z * 0.5f);
    Math::Vector3f gridMax(workspaceSize.x * 0.5f, workspaceSize.y, workspaceSize.z * 0.5f);
    
    // Check if ray intersects workspace bounds
    float tMin, tMax;
    Math::BoundingBox workspaceBounds(gridMin, gridMax);
    
    if (!rayIntersectsBox(ray, workspaceBounds, tMin, tMax)) {
        return result;
    }
    
    // Initialize traversal at workspace entry point, not ray origin
    GridTraversal traversal;
    
    // Calculate the ray position at workspace entry (tMin)
    Math::WorldCoordinates entryPoint = ray.pointAt(tMin);
    
    // Create a ray from the entry point
    VoxelEditor::VisualFeedback::Ray entryRay(entryPoint, ray.direction);
    initializeTraversal(entryRay, Math::WorldCoordinates(gridMin), voxelSize, resolution, traversal);
    
    
    // Traverse grid
    float maxDistance = m_maxRayDistance;
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
    Math::IncrementCoordinates startVoxel = traversal.current;
    bool startedInsideVoxel = false;
    
    // Check if we start inside a voxel
    if (grid.isValidIncrementPosition(traversal.current)) {
        startedInsideVoxel = grid.getVoxel(traversal.current);
    }
    
    // Track the previous voxel for exit detection
    Math::IncrementCoordinates previousVoxel = traversal.current;
    bool wasInsideStartVoxel = startedInsideVoxel;
    
    int stepCount = 0;
    while (currentDistance < maxDistance && stepCount < 50) { // Add step limit to prevent infinite loops
        // Check current voxel
        if (grid.isValidIncrementPosition(traversal.current)) {
            bool isCurrentVoxel = grid.getVoxel(traversal.current);
            
            if (isCurrentVoxel) {
                Logging::Logger::getInstance().debugfc("FaceDetector",
                    "Found voxel at grid position (%d, %d, %d)",
                    traversal.current.x(), traversal.current.y(), traversal.current.z());
            }
            
            // Special case: ray started inside a voxel and we just exited it
            if (startedInsideVoxel && wasInsideStartVoxel && 
                (traversal.current.x() != startVoxel.x() || 
                 traversal.current.y() != startVoxel.y() || 
                 traversal.current.z() != startVoxel.z())) {
                
                
                // We just exited the starting voxel - determine which face
                FaceDirection exitFace;
                if (traversal.current.x() != previousVoxel.x()) {
                    exitFace = (traversal.current.x() > previousVoxel.x()) ? 
                        FaceDirection::PositiveX : FaceDirection::NegativeX;
                } else if (traversal.current.y() != previousVoxel.y()) {
                    exitFace = (traversal.current.y() > previousVoxel.y()) ? 
                        FaceDirection::PositiveY : FaceDirection::NegativeY;
                } else {
                    exitFace = (traversal.current.z() > previousVoxel.z()) ? 
                        FaceDirection::PositiveZ : FaceDirection::NegativeZ;
                }
                
                // Create face for the exit
                closestHit.valid = true;
                closestHit.voxel = previousVoxel.value();
                closestHit.distance = currentDistance;
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
                // Convert voxel increment position to world coordinates
                Math::WorldCoordinates voxelWorldPos = Math::CoordinateConverter::incrementToWorld(traversal.current);
                Math::Vector3f voxelWorldMin = voxelWorldPos.value();
                Math::Vector3f voxelWorldMax = voxelWorldMin + Math::Vector3f(voxelSize, voxelSize, voxelSize);
                
                // Calculate exact intersection with voxel box in world space
                Math::BoundingBox voxelBox(voxelWorldMin, voxelWorldMax);
                
                float tMin, tMax;
                if (rayIntersectsBox(ray, voxelBox, tMin, tMax)) {
                    float hitDistance = tMin;
                    
                    // Only update if this is closer than any previous hit
                    if (!closestHit.valid || hitDistance < closestHit.distance) {
                        closestHit.valid = true;
                        closestHit.voxel = traversal.current.value();
                        closestHit.distance = hitDistance;
                        
                        // Calculate hit point to determine which face was hit
                        Math::Vector3f hitPoint = ray.origin.value() + ray.direction * tMin;
                        
                        // We already have voxelWorldMin and voxelWorldMax from above
                        
                        
                        // Determine which face was hit by checking which face the ray enters first
                        // Calculate t values for intersection with each face
                        const float epsilon = 0.0001f;
                        
                        // Calculate t values for intersection with each face
                        float tMinX = (std::abs(ray.direction.x) > epsilon) ? (voxelWorldMin.x - ray.origin.x()) / ray.direction.x : std::numeric_limits<float>::max();
                        float tMaxX = (std::abs(ray.direction.x) > epsilon) ? (voxelWorldMax.x - ray.origin.x()) / ray.direction.x : std::numeric_limits<float>::max();
                        float tMinY = (std::abs(ray.direction.y) > epsilon) ? (voxelWorldMin.y - ray.origin.y()) / ray.direction.y : std::numeric_limits<float>::max();
                        float tMaxY = (std::abs(ray.direction.y) > epsilon) ? (voxelWorldMax.y - ray.origin.y()) / ray.direction.y : std::numeric_limits<float>::max();
                        float tMinZ = (std::abs(ray.direction.z) > epsilon) ? (voxelWorldMin.z - ray.origin.z()) / ray.direction.z : std::numeric_limits<float>::max();
                        float tMaxZ = (std::abs(ray.direction.z) > epsilon) ? (voxelWorldMax.z - ray.origin.z()) / ray.direction.z : std::numeric_limits<float>::max();
                        
                        // For each axis, check which face plane (min or max) we hit first
                        std::vector<std::pair<float, FaceDirection>> candidates;
                        
                        // X-axis faces
                        if (std::abs(ray.direction.x) > epsilon) {
                            if (ray.direction.x > 0 && tMinX >= tMin - epsilon && tMinX <= tMax + epsilon) {
                                candidates.push_back({tMinX, FaceDirection::NegativeX});
                            } else if (ray.direction.x < 0 && tMaxX >= tMin - epsilon && tMaxX <= tMax + epsilon) {
                                candidates.push_back({tMaxX, FaceDirection::PositiveX});
                            }
                        }
                        
                        // Y-axis faces
                        if (std::abs(ray.direction.y) > epsilon) {
                            if (ray.direction.y > 0 && tMinY >= tMin - epsilon && tMinY <= tMax + epsilon) {
                                candidates.push_back({tMinY, FaceDirection::NegativeY});
                            } else if (ray.direction.y < 0 && tMaxY >= tMin - epsilon && tMaxY <= tMax + epsilon) {
                                candidates.push_back({tMaxY, FaceDirection::PositiveY});
                            }
                        }
                        
                        // Z-axis faces
                        if (std::abs(ray.direction.z) > epsilon) {
                            if (ray.direction.z > 0 && tMinZ >= tMin - epsilon && tMinZ <= tMax + epsilon) {
                                candidates.push_back({tMinZ, FaceDirection::NegativeZ});
                            } else if (ray.direction.z < 0 && tMaxZ >= tMin - epsilon && tMaxZ <= tMax + epsilon) {
                                candidates.push_back({tMaxZ, FaceDirection::PositiveZ});
                            }
                        }
                        
                        // Find the candidate with the smallest t value (entry face)
                        if (!candidates.empty()) {
                            auto entryFace = *std::min_element(candidates.begin(), candidates.end());
                            FaceDirection hitFaceDir = entryFace.second;
                            
                            closestHit.face = Face(traversal.current, resolution, hitFaceDir);
                            
                            // Set normal based on face direction
                            switch (hitFaceDir) {
                                case FaceDirection::PositiveX: closestHit.normal = Math::Vector3f(1, 0, 0); break;
                                case FaceDirection::NegativeX: closestHit.normal = Math::Vector3f(-1, 0, 0); break;
                                case FaceDirection::PositiveY: closestHit.normal = Math::Vector3f(0, 1, 0); break;
                                case FaceDirection::NegativeY: closestHit.normal = Math::Vector3f(0, -1, 0); break;
                                case FaceDirection::PositiveZ: closestHit.normal = Math::Vector3f(0, 0, 1); break;
                                case FaceDirection::NegativeZ: closestHit.normal = Math::Vector3f(0, 0, -1); break;
                            }
                        } else {
                            // Fallback: determine face based on which component of the ray direction is dominant
                            // and which face plane we're closest to
                            float absX = std::abs(ray.direction.x);
                            float absY = std::abs(ray.direction.y);
                            float absZ = std::abs(ray.direction.z);
                            
                            if (absX >= absY && absX >= absZ) {
                                // X is dominant direction
                                if (ray.direction.x > 0) {
                                    closestHit.face = Face(traversal.current, resolution, FaceDirection::NegativeX);
                                    closestHit.normal = Math::Vector3f(-1, 0, 0);
                                } else {
                                    closestHit.face = Face(traversal.current, resolution, FaceDirection::PositiveX);
                                    closestHit.normal = Math::Vector3f(1, 0, 0);
                                }
                            } else if (absY >= absX && absY >= absZ) {
                                // Y is dominant direction
                                if (ray.direction.y > 0) {
                                    closestHit.face = Face(traversal.current, resolution, FaceDirection::NegativeY);
                                    closestHit.normal = Math::Vector3f(0, -1, 0);
                                } else {
                                    closestHit.face = Face(traversal.current, resolution, FaceDirection::PositiveY);
                                    closestHit.normal = Math::Vector3f(0, 1, 0);
                                }
                            } else {
                                // Z is dominant direction
                                if (ray.direction.z > 0) {
                                    closestHit.face = Face(traversal.current, resolution, FaceDirection::NegativeZ);
                                    closestHit.normal = Math::Vector3f(0, 0, -1);
                                } else {
                                    closestHit.face = Face(traversal.current, resolution, FaceDirection::PositiveZ);
                                    closestHit.normal = Math::Vector3f(0, 0, 1);
                                }
                            }
                        }
                        
                        // Log which voxel was hit and the face detection details
                        Logging::Logger::getInstance().debugfc("FaceDetector",
                            "Found voxel at (%d,%d,%d) distance=%.3f, faceDir=%d",
                            traversal.current.x(), traversal.current.y(), traversal.current.z(),
                            closestHit.distance,
                            static_cast<int>(closestHit.face.getDirection()));
                    }
                }
            }
        }
        
        // Update tracking variables
        previousVoxel = traversal.current;
        
        // Step to next voxel
        stepTraversal(traversal);
        currentDistance = std::min({traversal.tMax.x, traversal.tMax.y, traversal.tMax.z});
        stepCount++;
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

bool FaceDetector::isValidFace(const Face& face, const VoxelData::VoxelGrid& grid) const {
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
    
    Math::Vector3f t1 = (box.min - ray.origin.value()) * invDir;
    Math::Vector3f t2 = (box.max - ray.origin.value()) * invDir;
    
    Math::Vector3f tMinVec(std::min(t1.x, t2.x), std::min(t1.y, t2.y), std::min(t1.z, t2.z));
    Math::Vector3f tMaxVec(std::max(t1.x, t2.x), std::max(t1.y, t2.y), std::max(t1.z, t2.z));
    
    tMin = std::max({tMinVec.x, tMinVec.y, tMinVec.z, 0.0f});
    tMax = std::min({tMaxVec.x, tMaxVec.y, tMaxVec.z});
    
    return tMin <= tMax && tMax >= 0.0f;
}

void FaceDetector::initializeTraversal(const Ray& ray, const Math::WorldCoordinates& gridMin, 
                                      float voxelSize, VoxelData::VoxelResolution resolution, GridTraversal& traversal) const {
    // Convert ray origin to increment coordinates
    Math::IncrementCoordinates rayIncrement = Math::CoordinateConverter::worldToIncrement(ray.origin);
    
    // Snap to the voxel grid for this resolution
    traversal.current = Math::CoordinateConverter::snapToVoxelResolution(rayIncrement, resolution);
    
    // Step direction in increment coordinates (voxel size in cm)
    int voxelSize_cm = static_cast<int>(voxelSize * 100.0f);
    traversal.step = Math::Vector3i(
        ray.direction.x > 0 ? voxelSize_cm : -voxelSize_cm,
        ray.direction.y > 0 ? voxelSize_cm : -voxelSize_cm,
        ray.direction.z > 0 ? voxelSize_cm : -voxelSize_cm
    );
    
    // Calculate tMax and tDelta
    const float epsilon = 0.0001f;
    
    // Convert current position to world coordinates for tMax calculation
    Math::WorldCoordinates currentWorld = Math::CoordinateConverter::incrementToWorld(traversal.current);
    
    if (std::abs(ray.direction.x) > epsilon) {
        float nextX = currentWorld.x() + (ray.direction.x > 0 ? voxelSize : -voxelSize * 0.5f);
        traversal.tMax.x = (nextX - ray.origin.x()) / ray.direction.x;
        traversal.tDelta.x = voxelSize / std::abs(ray.direction.x);
    } else {
        traversal.tMax.x = std::numeric_limits<float>::max();
        traversal.tDelta.x = std::numeric_limits<float>::max();
    }
    
    if (std::abs(ray.direction.y) > epsilon) {
        float nextY = currentWorld.y() + (ray.direction.y > 0 ? voxelSize : -voxelSize * 0.5f);
        traversal.tMax.y = (nextY - ray.origin.y()) / ray.direction.y;
        traversal.tDelta.y = voxelSize / std::abs(ray.direction.y);
    } else {
        traversal.tMax.y = std::numeric_limits<float>::max();
        traversal.tDelta.y = std::numeric_limits<float>::max();
    }
    
    if (std::abs(ray.direction.z) > epsilon) {
        float nextZ = currentWorld.z() + (ray.direction.z > 0 ? voxelSize : -voxelSize * 0.5f);
        traversal.tMax.z = (nextZ - ray.origin.z()) / ray.direction.z;
        traversal.tDelta.z = voxelSize / std::abs(ray.direction.z);
    } else {
        traversal.tMax.z = std::numeric_limits<float>::max();
        traversal.tDelta.z = std::numeric_limits<float>::max();
    }
}

void FaceDetector::stepTraversal(GridTraversal& traversal) const {
    if (traversal.tMax.x < traversal.tMax.y) {
        if (traversal.tMax.x < traversal.tMax.z) {
            traversal.current = Math::IncrementCoordinates(traversal.current.x() + traversal.step.x, traversal.current.y(), traversal.current.z());
            traversal.tMax.x += traversal.tDelta.x;
        } else {
            traversal.current = Math::IncrementCoordinates(traversal.current.x(), traversal.current.y(), traversal.current.z() + traversal.step.z);
            traversal.tMax.z += traversal.tDelta.z;
        }
    } else {
        if (traversal.tMax.y < traversal.tMax.z) {
            traversal.current = Math::IncrementCoordinates(traversal.current.x(), traversal.current.y() + traversal.step.y, traversal.current.z());
            traversal.tMax.y += traversal.tDelta.y;
        } else {
            traversal.current = Math::IncrementCoordinates(traversal.current.x(), traversal.current.y(), traversal.current.z() + traversal.step.z);
            traversal.tMax.z += traversal.tDelta.z;
        }
    }
}

// Additional methods for requirements tests
bool FaceDetector::isFaceVisible(const Face& face) const {
    // For simplicity, assume all faces are visible
    return face.isValid();
}

Rendering::Color FaceDetector::getFaceHighlightColor(const Face& face) const {
    // Return yellow highlight color
    return Rendering::Color(1.0f, 1.0f, 0.0f, 1.0f);
}

bool FaceDetector::validateFace(const Face& face, const VoxelData::VoxelGrid& grid) const {
    return isValidFace(face, grid);
}

bool FaceDetector::hasActiveHighlight() const {
    return m_hasActiveHighlight;
}

} // namespace VisualFeedback
} // namespace VoxelEditor