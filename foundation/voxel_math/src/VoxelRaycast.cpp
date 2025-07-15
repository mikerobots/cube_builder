#include "../include/voxel_math/VoxelRaycast.h"
#include "../include/voxel_math/VoxelGridMath.h"
#include "../include/voxel_math/FaceOperations.h"
#include "../../../core/voxel_data/VoxelGrid.h"
#include "../../math/CoordinateConverter.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace VoxelEditor {
namespace Math {

VoxelRaycast::RaycastResult VoxelRaycast::raycastVoxel(const Ray& ray,
                                                       const IncrementCoordinates& voxelPos,
                                                       VoxelData::VoxelResolution resolution) {
    RaycastResult result;
    
    // Create voxel bounds
    VoxelBounds voxelBounds(voxelPos, VoxelGridMath::getVoxelSizeMeters(resolution));
    
    // Test ray-box intersection
    float tMin, tMax;
    if (!voxelBounds.intersectsRay(ray, tMin, tMax)) {
        return result;  // No hit
    }
    
    // Use the entry point for hits from outside, exit point for hits from inside
    float hitT = (tMin > 0.0f) ? tMin : tMax;
    
    if (hitT < 0.0f) {
        return result;  // Hit is behind ray origin
    }
    
    // Calculate hit point
    WorldCoordinates hitPoint(ray.getPoint(hitT));
    
    // Determine which face was hit
    VoxelData::FaceDirection hitFace = calculateHitFace(ray, voxelBounds, hitT);
    
    // Get face normal
    Vector3f hitNormal = FaceOperations::getFaceNormal(hitFace);
    
    // Fill result
    result.hit = true;
    result.distance = hitT;
    result.voxelPos = voxelPos;
    result.hitFace = hitFace;
    result.hitPoint = hitPoint;
    result.hitNormal = hitNormal;
    
    return result;
}

VoxelRaycast::RaycastResult VoxelRaycast::raycastGrid(const Ray& ray,
                                                      const VoxelData::VoxelGrid& grid,
                                                      VoxelData::VoxelResolution resolution,
                                                      float maxDistance) {
    RaycastResult closestResult;
    float closestDistance = std::numeric_limits<float>::max();
    
    // Get all voxels in the grid
    auto allVoxels = grid.getAllVoxels();
    
    // Test each voxel
    for (const auto& voxelData : allVoxels) {
        // Only check voxels of the specified resolution
        if (voxelData.resolution != resolution) {
            continue;
        }
        
        RaycastResult voxelResult = raycastVoxel(ray, voxelData.incrementPos, resolution);
        
        if (voxelResult.hit && 
            voxelResult.distance < closestDistance && 
            voxelResult.distance <= maxDistance) {
            closestResult = voxelResult;
            closestDistance = voxelResult.distance;
        }
    }
    
    return closestResult;
}

std::vector<IncrementCoordinates> VoxelRaycast::getVoxelsAlongRay(const Ray& ray,
                                                                  VoxelData::VoxelResolution resolution,
                                                                  float maxDistance) {
    std::vector<IncrementCoordinates> voxels;
    
    // Initialize DDA traversal
    TraversalState state;
    initializeTraversal(ray, resolution, state);
    
    if (!state.valid) {
        return voxels;
    }
    
    int stepCount = 0;
    while (stepCount < MAX_TRAVERSAL_STEPS && getCurrentDistance(state) <= maxDistance) {
        voxels.push_back(state.current);
        
        stepTraversal(state);
        if (!state.valid) {
            break;
        }
        
        stepCount++;
    }
    
    return voxels;
}

bool VoxelRaycast::rayIntersectsGrid(const Ray& ray,
                                    const VoxelData::VoxelGrid& grid,
                                    VoxelData::VoxelResolution resolution,
                                    float maxDistance) {
    // Get all voxels in the grid
    auto allVoxels = grid.getAllVoxels();
    
    // Test each voxel
    for (const auto& voxelData : allVoxels) {
        // Only check voxels of the specified resolution
        if (voxelData.resolution != resolution) {
            continue;
        }
        
        RaycastResult voxelResult = raycastVoxel(ray, voxelData.incrementPos, resolution);
        
        if (voxelResult.hit && voxelResult.distance <= maxDistance) {
            return true;  // Early exit on first hit
        }
    }
    
    return false;
}

std::vector<VoxelRaycast::RaycastResult> VoxelRaycast::raycastAllHits(const Ray& ray,
                                                                      const VoxelData::VoxelGrid& grid,
                                                                      VoxelData::VoxelResolution resolution,
                                                                      float maxDistance,
                                                                      int maxHits) {
    std::vector<RaycastResult> hits;
    
    // Get all voxels in the grid
    auto allVoxels = grid.getAllVoxels();
    
    // Test each voxel
    for (const auto& voxelData : allVoxels) {
        // Only check voxels of the specified resolution
        if (voxelData.resolution != resolution) {
            continue;
        }
        
        RaycastResult voxelResult = raycastVoxel(ray, voxelData.incrementPos, resolution);
        
        if (voxelResult.hit && voxelResult.distance <= maxDistance) {
            hits.push_back(voxelResult);
            
            // Check if we've reached max hits
            if (maxHits > 0 && static_cast<int>(hits.size()) >= maxHits) {
                break;
            }
        }
    }
    
    // Sort by distance
    std::sort(hits.begin(), hits.end(), 
              [](const RaycastResult& a, const RaycastResult& b) {
                  return a.distance < b.distance;
              });
    
    return hits;
}

std::optional<VoxelRaycast::RaycastResult> VoxelRaycast::raycastWorkspace(const Ray& ray,
                                                                         const Vector3f& workspaceSize) {
    // Create workspace bounds (centered at origin)
    Vector3f workspaceMin(-workspaceSize.x * 0.5f, 0.0f, -workspaceSize.z * 0.5f);
    Vector3f workspaceMax(workspaceSize.x * 0.5f, workspaceSize.y, workspaceSize.z * 0.5f);
    
    float tMin, tMax;
    if (!rayBoxIntersection(ray, workspaceMin, workspaceMax, tMin, tMax)) {
        return std::nullopt;
    }
    
    // Use entry point if ray starts outside, otherwise use exit point
    float hitT = (tMin > 0.0f) ? tMin : tMax;
    
    if (hitT < 0.0f) {
        return std::nullopt;
    }
    
    WorldCoordinates hitPoint(ray.getPoint(hitT));
    
    // Determine which face of workspace was hit
    VoxelData::FaceDirection hitFace = VoxelData::FaceDirection::PosY;  // Default
    const Vector3f& hit = hitPoint.value();
    const float epsilon = 0.001f;
    
    if (std::abs(hit.x - workspaceMin.x) < epsilon) {
        hitFace = VoxelData::FaceDirection::NegX;
    } else if (std::abs(hit.x - workspaceMax.x) < epsilon) {
        hitFace = VoxelData::FaceDirection::PosX;
    } else if (std::abs(hit.y - workspaceMin.y) < epsilon) {
        hitFace = VoxelData::FaceDirection::NegY;
    } else if (std::abs(hit.y - workspaceMax.y) < epsilon) {
        hitFace = VoxelData::FaceDirection::PosY;
    } else if (std::abs(hit.z - workspaceMin.z) < epsilon) {
        hitFace = VoxelData::FaceDirection::NegZ;
    } else if (std::abs(hit.z - workspaceMax.z) < epsilon) {
        hitFace = VoxelData::FaceDirection::PosZ;
    }
    
    Vector3f hitNormal = FaceOperations::getFaceNormal(hitFace);
    
    RaycastResult result;
    result.hit = true;
    result.distance = hitT;
    result.voxelPos = CoordinateConverter::worldToIncrement(hitPoint);
    result.hitFace = hitFace;
    result.hitPoint = hitPoint;
    result.hitNormal = hitNormal;
    
    return result;
}

std::optional<std::pair<WorldCoordinates, WorldCoordinates>> 
VoxelRaycast::calculateRayVoxelIntersection(const Ray& ray, const VoxelBounds& voxelBounds) {
    float tMin, tMax;
    if (!voxelBounds.intersectsRay(ray, tMin, tMax)) {
        return std::nullopt;
    }
    
    WorldCoordinates entryPoint(ray.getPoint(tMin));
    WorldCoordinates exitPoint(ray.getPoint(tMax));
    
    return std::make_pair(entryPoint, exitPoint);
}

void VoxelRaycast::initializeTraversal(const Ray& ray, 
                                      VoxelData::VoxelResolution resolution,
                                      TraversalState& state) {
    // Get voxel size
    float voxelSize = VoxelGridMath::getVoxelSizeMeters(resolution);
    int voxelSizeCm = VoxelGridMath::getVoxelSizeCm(resolution);
    
    // Convert ray origin to increment coordinates and snap to voxel grid
    IncrementCoordinates rayIncrement = CoordinateConverter::worldToIncrement(WorldCoordinates(ray.origin));
    
    // Snap to voxel grid boundaries
    state.current = VoxelGridMath::snapToVoxelGrid(rayIncrement, resolution);
    
    // Calculate step direction
    state.step = Vector3i(
        ray.direction.x > 0 ? voxelSizeCm : -voxelSizeCm,
        ray.direction.y > 0 ? voxelSizeCm : -voxelSizeCm,
        ray.direction.z > 0 ? voxelSizeCm : -voxelSizeCm
    );
    
    // Calculate tMax and tDelta
    WorldCoordinates currentWorld = CoordinateConverter::incrementToWorld(state.current);
    
    // X axis
    if (std::abs(ray.direction.x) > EPSILON) {
        float nextX = currentWorld.x() + (ray.direction.x > 0 ? voxelSize : -voxelSize);
        state.tMax.x = (nextX - ray.origin.x) / ray.direction.x;
        state.tDelta.x = voxelSize / std::abs(ray.direction.x);
    } else {
        state.tMax.x = std::numeric_limits<float>::max();
        state.tDelta.x = std::numeric_limits<float>::max();
    }
    
    // Y axis
    if (std::abs(ray.direction.y) > EPSILON) {
        float nextY = currentWorld.y() + (ray.direction.y > 0 ? voxelSize : -voxelSize);
        state.tMax.y = (nextY - ray.origin.y) / ray.direction.y;
        state.tDelta.y = voxelSize / std::abs(ray.direction.y);
    } else {
        state.tMax.y = std::numeric_limits<float>::max();
        state.tDelta.y = std::numeric_limits<float>::max();
    }
    
    // Z axis
    if (std::abs(ray.direction.z) > EPSILON) {
        float nextZ = currentWorld.z() + (ray.direction.z > 0 ? voxelSize : -voxelSize);
        state.tMax.z = (nextZ - ray.origin.z) / ray.direction.z;
        state.tDelta.z = voxelSize / std::abs(ray.direction.z);
    } else {
        state.tMax.z = std::numeric_limits<float>::max();
        state.tDelta.z = std::numeric_limits<float>::max();
    }
    
    state.valid = true;
}

void VoxelRaycast::stepTraversal(TraversalState& state) {
    if (!state.valid) {
        return;
    }
    
    // Step to the next voxel along the direction with the smallest tMax
    if (state.tMax.x < state.tMax.y && state.tMax.x < state.tMax.z) {
        // Step in X direction
        state.current = IncrementCoordinates(
            state.current.x() + state.step.x,
            state.current.y(),
            state.current.z()
        );
        state.tMax.x += state.tDelta.x;
    } else if (state.tMax.y < state.tMax.z) {
        // Step in Y direction
        state.current = IncrementCoordinates(
            state.current.x(),
            state.current.y() + state.step.y,
            state.current.z()
        );
        state.tMax.y += state.tDelta.y;
    } else {
        // Step in Z direction
        state.current = IncrementCoordinates(
            state.current.x(),
            state.current.y(),
            state.current.z() + state.step.z
        );
        state.tMax.z += state.tDelta.z;
    }
}

VoxelData::FaceDirection VoxelRaycast::calculateHitFace(const Ray& ray,
                                                       const VoxelBounds& voxelBounds,
                                                       float t) {
    // Calculate hit point
    WorldCoordinates hitPoint(ray.getPoint(t));
    
    // Use FaceOperations to determine face from hit point
    return FaceOperations::determineFaceFromHit(hitPoint, voxelBounds);
}

float VoxelRaycast::getCurrentDistance(const TraversalState& state) {
    return std::min({state.tMax.x, state.tMax.y, state.tMax.z}) - 
           std::max({state.tDelta.x, state.tDelta.y, state.tDelta.z});
}

bool VoxelRaycast::isInWorkspace(const WorldCoordinates& pos, const Vector3f& workspaceSize) {
    const Vector3f& p = pos.value();
    float halfX = workspaceSize.x * 0.5f;
    float halfZ = workspaceSize.z * 0.5f;
    
    return p.x >= -halfX && p.x <= halfX &&
           p.y >= 0.0f && p.y <= workspaceSize.y &&
           p.z >= -halfZ && p.z <= halfZ;
}

VoxelData::FaceDirection VoxelRaycast::stepDirectionToFace(const Vector3i& stepDirection) {
    if (stepDirection.x != 0) {
        return stepDirection.x > 0 ? VoxelData::FaceDirection::PosX 
                                   : VoxelData::FaceDirection::NegX;
    } else if (stepDirection.y != 0) {
        return stepDirection.y > 0 ? VoxelData::FaceDirection::PosY 
                                   : VoxelData::FaceDirection::NegY;
    } else {
        return stepDirection.z > 0 ? VoxelData::FaceDirection::PosZ 
                                   : VoxelData::FaceDirection::NegZ;
    }
}

bool VoxelRaycast::rayBoxIntersection(const Ray& ray,
                                     const Vector3f& boxMin,
                                     const Vector3f& boxMax,
                                     float& tMin,
                                     float& tMax) {
    // Use the standard ray-box intersection algorithm
    Vector3f invDir(
        std::abs(ray.direction.x) > EPSILON ? 1.0f / ray.direction.x : 1.0f / EPSILON,
        std::abs(ray.direction.y) > EPSILON ? 1.0f / ray.direction.y : 1.0f / EPSILON,
        std::abs(ray.direction.z) > EPSILON ? 1.0f / ray.direction.z : 1.0f / EPSILON
    );
    
    Vector3f t1 = (boxMin - ray.origin) * invDir;
    Vector3f t2 = (boxMax - ray.origin) * invDir;
    
    Vector3f tMinVec(std::min(t1.x, t2.x), std::min(t1.y, t2.y), std::min(t1.z, t2.z));
    Vector3f tMaxVec(std::max(t1.x, t2.x), std::max(t1.y, t2.y), std::max(t1.z, t2.z));
    
    tMin = std::max({tMinVec.x, tMinVec.y, tMinVec.z, 0.0f});
    tMax = std::min({tMaxVec.x, tMaxVec.y, tMaxVec.z});
    
    return tMin <= tMax && tMax >= 0.0f;
}

} // namespace Math
} // namespace VoxelEditor