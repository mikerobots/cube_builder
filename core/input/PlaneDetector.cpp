#include "PlaneDetector.h"
#include "../voxel_data/VoxelDataManager.h"
#include "../voxel_data/VoxelTypes.h"
#include "../../foundation/math/CoordinateConverter.h"
#include <algorithm>
#include <cmath>
#include <set>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace VoxelEditor {
namespace Input {

PlaneDetector::PlaneDetector(VoxelData::VoxelDataManager* voxelManager)
    : m_voxelManager(voxelManager)
    , m_currentPlane(std::nullopt)
    , m_planePersistenceActive(false)
    , m_persistenceTimeout(0.0f) {
}

PlaneDetectionResult PlaneDetector::detectPlane(const PlaneDetectionContext& context) {
    if (!m_voxelManager) {
        return PlaneDetectionResult::NotFound();
    }
    
    // First, try to find highest voxel under cursor
    auto highestVoxelInfo = findHighestVoxelUnderCursor(context.worldPosition);
    
    if (highestVoxelInfo.has_value()) {
        // Found a voxel - create plane from its top surface face
        const VoxelInfo& voxelInfo = highestVoxelInfo.value();
        
        // Calculate the top height of this voxel
        float topHeight = calculateVoxelTopHeight(voxelInfo.position, voxelInfo.resolution);
        
        PlacementPlane plane(topHeight, voxelInfo.position, voxelInfo.resolution);
        PlaneDetectionResult result = PlaneDetectionResult::Found(plane);
        
        // Find all voxels on this plane
        result.voxelsOnPlane = getVoxelsAtHeight(topHeight);
        
        return result;
    } else {
        // No voxel found - use ground plane
        return PlaneDetectionResult::Found(PlacementPlane::GroundPlane());
    }
}

std::optional<PlaneDetector::VoxelInfo> PlaneDetector::findHighestVoxelUnderCursor(const Math::Vector3f& worldPos, float searchRadius) {
    if (!m_voxelManager) {
        return std::nullopt;
    }
    
    // Convert world position to increment coordinates
    Math::IncrementCoordinates basePos = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(worldPos));
    
    // First, try direct vertical search at cursor position
    auto directHit = findHighestVoxelAtPosition(basePos.x(), basePos.z());
    if (directHit.has_value()) {
        return directHit;
    }
    
    // If no direct hit, do a limited spiral search
    return findHighestVoxelInRadius(basePos, searchRadius);
}

void PlaneDetector::updatePlanePersistence(const Math::IncrementCoordinates& previewPosition, 
                                          VoxelData::VoxelResolution previewResolution,
                                          float deltaTime) {
    if (!m_currentPlane.has_value()) {
        return;
    }
    
    // Check if preview overlaps any voxels on current plane
    bool overlaps = previewOverlapsCurrentPlane(previewPosition, previewResolution);
    
    if (overlaps) {
        // Preview overlaps - maintain persistence
        m_planePersistenceActive = true;
        m_persistenceTimeout = 0.0f;
    } else {
        // Preview doesn't overlap - start or continue timeout
        m_planePersistenceActive = true; // Ensure we're tracking timeout
        m_persistenceTimeout += deltaTime;
        
        if (m_persistenceTimeout > PERSISTENCE_TIMEOUT_SECONDS) {
            // Timeout reached - clear plane
            clearCurrentPlane();
            m_planePersistenceActive = false;
            m_persistenceTimeout = 0.0f;
        }
    }
}

bool PlaneDetector::previewOverlapsCurrentPlane(const Math::IncrementCoordinates& previewPosition, 
                                               VoxelData::VoxelResolution previewResolution) const {
    if (!m_currentPlane.has_value() || !m_voxelManager) {
        return false;
    }
    
    // Simply check if placing the preview voxel would overlap with any existing voxel
    // The wouldOverlap method already handles checking all voxels efficiently
    return m_voxelManager->wouldOverlap(previewPosition, previewResolution);
}

bool PlaneDetector::shouldTransitionToNewPlane(const PlaneDetectionResult& newPlaneResult) const {
    if (!newPlaneResult.found) {
        return false;
    }
    
    if (!m_currentPlane.has_value()) {
        return true; // No current plane, so transition to new one
    }
    
    const PlacementPlane& currentPlane = m_currentPlane.value();
    const PlacementPlane& newPlane = newPlaneResult.plane;
    
    // Transition if the new plane is significantly higher
    float heightDifference = newPlane.height - currentPlane.height;
    return heightDifference > 0.01f; // 1cm threshold
}

std::vector<Math::IncrementCoordinates> PlaneDetector::getVoxelsAtHeight(float height, float tolerance) const {
    std::vector<Math::IncrementCoordinates> voxelsAtHeight;
    
    if (!m_voxelManager) {
        return voxelsAtHeight;
    }
    
    // OPTIMIZED APPROACH: Iterate through existing voxels instead of searching empty space
    // This is O(n) where n is the number of actual voxels, not O(space * resolutions)
    
    // Use a set to avoid duplicates when checking multiple resolutions
    std::set<Math::IncrementCoordinates> uniqueVoxelsAtHeight;
    
    // Only check resolutions that actually have voxels
    for (auto resolution : getAllResolutions()) {
        // Skip resolutions with no voxels
        if (m_voxelManager->getVoxelCount(resolution) == 0) {
            continue;
        }
        
        // Get all voxels of this resolution
        auto voxelsOfResolution = m_voxelManager->getAllVoxels(resolution);
        
        // Check each voxel's top height
        for (const auto& voxelPos : voxelsOfResolution) {
            float voxelTopHeight = calculateVoxelTopHeight(voxelPos.incrementPos, voxelPos.resolution);
            
            // Check if this voxel's top is at the target height (within tolerance)
            if (std::abs(voxelTopHeight - height) <= tolerance) {
                uniqueVoxelsAtHeight.insert(voxelPos.incrementPos);
            }
        }
    }
    
    // Convert set to vector
    voxelsAtHeight.assign(uniqueVoxelsAtHeight.begin(), uniqueVoxelsAtHeight.end());
    
    return voxelsAtHeight;
}

float PlaneDetector::calculateVoxelTopHeight(const Math::IncrementCoordinates& voxelPos, VoxelData::VoxelResolution resolution) const {
    // Convert increment position to world position (this gives us the bottom-center)
    Math::WorldCoordinates worldCoords = Math::CoordinateConverter::incrementToWorld(voxelPos);
    Math::Vector3f worldPos = worldCoords.value();
    
    // Add voxel size to get the top face height
    float voxelSize = VoxelData::getVoxelSize(resolution);
    return worldPos.y + voxelSize;
}


void PlaneDetector::reset() {
    m_currentPlane = std::nullopt;
    m_planePersistenceActive = false;
    m_persistenceTimeout = 0.0f;
}

// Private helper methods

std::vector<Math::IncrementCoordinates> PlaneDetector::searchVoxelsInCylinder(const Math::Vector3f& centerPos, 
                                                                 float radius, 
                                                                 float minHeight, 
                                                                 float maxHeight) const {
    std::vector<Math::IncrementCoordinates> voxels;
    
    if (!m_voxelManager) {
        return voxels;
    }
    
    // OPTIMIZED APPROACH: Iterate through existing voxels and filter by cylinder bounds
    // This is O(n) where n is the number of actual voxels, not O(space)
    
    // Convert to increment coordinates for comparison
    Math::IncrementCoordinates centerIncrement = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(centerPos));
    int radiusIncrementSquared = static_cast<int>(radius * 100) * static_cast<int>(radius * 100);
    
    // Use a set to avoid duplicate positions
    std::set<Math::IncrementCoordinates> uniqueVoxels;
    
    // Only check resolutions that actually have voxels
    for (auto resolution : getAllResolutions()) {
        // Skip resolutions with no voxels
        if (m_voxelManager->getVoxelCount(resolution) == 0) {
            continue;
        }
        
        // Get all voxels of this resolution
        auto voxelsOfResolution = m_voxelManager->getAllVoxels(resolution);
        
        // Filter voxels that are within the cylinder
        for (const auto& voxelPos : voxelsOfResolution) {
            const Math::IncrementCoordinates& pos = voxelPos.incrementPos;
            
            // Convert to world coordinates for height check
            Math::WorldCoordinates worldCoords = Math::CoordinateConverter::incrementToWorld(pos);
            float worldY = worldCoords.value().y;
            
            // Check height bounds
            if (worldY < minHeight || worldY > maxHeight) {
                continue;
            }
            
            // Check cylinder radius (using squared distance to avoid sqrt)
            int dx = pos.x() - centerIncrement.x();
            int dz = pos.z() - centerIncrement.z();
            int distanceSquared = dx * dx + dz * dz;
            
            if (distanceSquared <= radiusIncrementSquared) {
                uniqueVoxels.insert(pos);
            }
        }
    }
    
    // Convert set to vector
    voxels.assign(uniqueVoxels.begin(), uniqueVoxels.end());
    return voxels;
}

std::optional<Math::IncrementCoordinates> PlaneDetector::findHighestVoxel(const std::vector<Math::IncrementCoordinates>& voxels) const {
    if (voxels.empty()) {
        return std::nullopt;
    }
    
    if (!m_voxelManager) {
        return std::nullopt;
    }
    
    // Find the voxel with highest top face
    auto highestVoxel = voxels[0];
    float highestHeight = 0.0f;
    
    for (const auto& voxel : voxels) {
        // Determine resolution of this voxel
        for (auto resolution : getAllResolutions()) {
            if (m_voxelManager->getVoxel(voxel, resolution)) {
                float voxelTopHeight = calculateVoxelTopHeight(voxel, resolution);
                if (voxelTopHeight > highestHeight) {
                    highestHeight = voxelTopHeight;
                    highestVoxel = voxel;
                }
                break; // Found the voxel, move to next
            }
        }
    }
    
    return highestVoxel;
}

bool PlaneDetector::wouldLargerVoxelOverlapSmaller(const Math::IncrementCoordinates& position,
                                                  VoxelData::VoxelResolution largerResolution) const {
    if (!m_voxelManager) {
        return false;
    }
    
    // Check if placing a larger voxel would overlap with smaller voxels
    return m_voxelManager->wouldOverlap(position, largerResolution);
}

std::optional<PlaneDetector::VoxelInfo> PlaneDetector::findHighestVoxelAtPosition(int x, int z) {
    std::optional<VoxelInfo> highestVoxel;
    float highestTopHeight = -1.0f;
    
    if (!m_voxelManager) {
        return highestVoxel;
    }
    
    // OPTIMIZED APPROACH: Check existing voxels instead of searching empty space
    // Only check resolutions that actually have voxels
    for (auto resolution : getAllResolutions()) {
        // Skip resolutions with no voxels
        if (m_voxelManager->getVoxelCount(resolution) == 0) {
            continue;
        }
        
        float voxelSize = VoxelData::getVoxelSize(resolution);
        int voxelSizeIncrements = static_cast<int>(voxelSize * 100);
        
        // Get all voxels of this resolution
        auto voxelsOfResolution = m_voxelManager->getAllVoxels(resolution);
        
        // Check each voxel to see if it contains the query point (x,z)
        for (const auto& voxelPos : voxelsOfResolution) {
            const Math::IncrementCoordinates& pos = voxelPos.incrementPos;
            
            // Check if this voxel contains the query point (x,z)
            // A voxel at position (vx,vy,vz) with size S contains point (x,z) if:
            // vx <= x < vx+S and vz <= z < vz+S
            if (pos.x() <= x && x < pos.x() + voxelSizeIncrements &&
                pos.z() <= z && z < pos.z() + voxelSizeIncrements) {
                
                // This voxel contains the point, check if it's the highest
                float topHeight = calculateVoxelTopHeight(pos, resolution);
                if (topHeight > highestTopHeight) {
                    highestTopHeight = topHeight;
                    highestVoxel = VoxelInfo(pos, resolution);
                }
            }
        }
    }
    
    return highestVoxel;
}

std::optional<PlaneDetector::VoxelInfo> PlaneDetector::findHighestVoxelInRadius(const Math::IncrementCoordinates& centerPos, float searchRadius) {
    std::optional<VoxelInfo> bestVoxel;
    float highestHeight = -1.0f;
    
    if (!m_voxelManager) {
        return bestVoxel;
    }
    
    // OPTIMIZED APPROACH: Check existing voxels within radius instead of grid search
    int radiusIncrements = static_cast<int>(searchRadius * 100);
    int radiusSquared = radiusIncrements * radiusIncrements;
    
    // Only check resolutions that actually have voxels
    for (auto resolution : getAllResolutions()) {
        // Skip resolutions with no voxels
        if (m_voxelManager->getVoxelCount(resolution) == 0) {
            continue;
        }
        
        float voxelSize = VoxelData::getVoxelSize(resolution);
        int voxelSizeIncrements = static_cast<int>(voxelSize * 100);
        
        // Get all voxels of this resolution
        auto voxelsOfResolution = m_voxelManager->getAllVoxels(resolution);
        
        // Check each voxel to see if it's within the search radius
        for (const auto& voxelPos : voxelsOfResolution) {
            const Math::IncrementCoordinates& pos = voxelPos.incrementPos;
            
            // Check if any part of this voxel is within the search radius
            // Calculate closest point on voxel to center
            int closestX = std::max(pos.x(), std::min(centerPos.x(), pos.x() + voxelSizeIncrements - 1));
            int closestZ = std::max(pos.z(), std::min(centerPos.z(), pos.z() + voxelSizeIncrements - 1));
            
            int dx = closestX - centerPos.x();
            int dz = closestZ - centerPos.z();
            int distSquared = dx * dx + dz * dz;
            
            if (distSquared <= radiusSquared) {
                // This voxel is within search radius, check if it's the highest
                float topHeight = calculateVoxelTopHeight(pos, resolution);
                if (topHeight > highestHeight) {
                    highestHeight = topHeight;
                    bestVoxel = VoxelInfo(pos, resolution);
                }
            }
        }
    }
    
    return bestVoxel;
}

std::vector<VoxelData::VoxelResolution> PlaneDetector::getAllResolutions() const {
    return {
        VoxelData::VoxelResolution::Size_1cm,
        VoxelData::VoxelResolution::Size_2cm,
        VoxelData::VoxelResolution::Size_4cm,
        VoxelData::VoxelResolution::Size_8cm,
        VoxelData::VoxelResolution::Size_16cm,
        VoxelData::VoxelResolution::Size_32cm,
        VoxelData::VoxelResolution::Size_64cm,
        VoxelData::VoxelResolution::Size_128cm,
        VoxelData::VoxelResolution::Size_256cm,
        VoxelData::VoxelResolution::Size_512cm
    };
}

// Note: worldToGrid and gridToWorld methods removed - use CoordinateConverter instead

}
}