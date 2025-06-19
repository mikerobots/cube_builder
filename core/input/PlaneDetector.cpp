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
    
    // Use much smaller search area and adaptive resolution checking
    float searchRadius = std::min(DEFAULT_SEARCH_RADIUS, 2.0f); // Max 2m search
    int searchRadiusIncrement = static_cast<int>(searchRadius * 100);
    
    // Use coarser grid for initial search
    int step = 8; // 8cm steps instead of 1cm
    
    for (int x = -searchRadiusIncrement; x <= searchRadiusIncrement; x += step) {
        for (int z = -searchRadiusIncrement; z <= searchRadiusIncrement; z += step) {
            // Calculate distance for early culling
            if (x * x + z * z > searchRadiusIncrement * searchRadiusIncrement) {
                continue;
            }
            
            // Only check resolutions that could realistically have their top at this height
            for (auto resolution : getAllResolutions()) {
                float voxelSize = VoxelData::getVoxelSize(resolution);
                
                // Calculate the Y position where a voxel would need to start to have its top at target height
                int targetY = static_cast<int>((height - voxelSize) * 100);
                
                // Only check a small range around this position
                for (int yOffset = -4; yOffset <= 4; yOffset += 2) { // ±4cm range, 2cm steps
                    Math::IncrementCoordinates pos(x, targetY + yOffset, z);
                    
                    if (m_voxelManager->getVoxel(pos, resolution)) {
                        float voxelTopHeight = calculateVoxelTopHeight(pos, resolution);
                        
                        if (std::abs(voxelTopHeight - height) <= tolerance) {
                            voxelsAtHeight.push_back(pos);
                        }
                    }
                }
            }
        }
    }
    
    return voxelsAtHeight;
}

float PlaneDetector::calculateVoxelTopHeight(const Math::IncrementCoordinates& voxelPos, VoxelData::VoxelResolution resolution) const {
    // Convert increment position to world position (this gives us the bottom-left corner)
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
    
    // Convert to increment coordinates (1cm granularity)
    Math::IncrementCoordinates centerIncrement = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(centerPos));
    int minYIncrement = static_cast<int>(minHeight * 100); // Convert to cm
    int maxYIncrement = static_cast<int>(maxHeight * 100); // Convert to cm
    int radiusIncrement = static_cast<int>(radius * 100) + 1; // Convert to cm, add 1 for safety
    
    // Use a more efficient search strategy - check larger voxels first since they're more likely to be found
    auto resolutions = getAllResolutions();
    std::reverse(resolutions.begin(), resolutions.end()); // Start with largest resolution
    
    // Use a set to avoid duplicate positions
    std::set<Math::IncrementCoordinates> uniqueVoxels;
    
    for (auto resolution : resolutions) {
        float voxelSize = VoxelData::getVoxelSize(resolution);
        int voxelSizeIncrements = static_cast<int>(voxelSize * 100);
        
        // Search at intervals appropriate for this voxel size
        int stepSize = std::max(1, voxelSizeIncrements / 2);
        
        for (int y = minYIncrement; y <= maxYIncrement; y += stepSize) {
            for (int x = centerIncrement.x() - radiusIncrement; x <= centerIncrement.x() + radiusIncrement; x += stepSize) {
                for (int z = centerIncrement.z() - radiusIncrement; z <= centerIncrement.z() + radiusIncrement; z += stepSize) {
                    // Quick distance check before coordinate conversion
                    int dx = x - centerIncrement.x();
                    int dz = z - centerIncrement.z();
                    if (dx * dx + dz * dz > radiusIncrement * radiusIncrement) {
                        continue; // Outside cylinder
                    }
                    
                    Math::IncrementCoordinates checkPos(x, y, z);
                    
                    if (m_voxelManager->getVoxel(checkPos, resolution)) {
                        uniqueVoxels.insert(checkPos);
                        
                        // Also check the actual voxel bounds more precisely
                        for (int dy = 0; dy < voxelSizeIncrements; dy += stepSize) {
                            for (int dx = 0; dx < voxelSizeIncrements; dx += stepSize) {
                                for (int dz = 0; dz < voxelSizeIncrements; dz += stepSize) {
                                    Math::IncrementCoordinates boundPos(x + dx, y + dy, z + dz);
                                    
                                    // Check distance for this position too
                                    int bdx = boundPos.x() - centerIncrement.x();
                                    int bdz = boundPos.z() - centerIncrement.z();
                                    if (bdx * bdx + bdz * bdz <= radiusIncrement * radiusIncrement) {
                                        if (m_voxelManager->getVoxel(boundPos, resolution)) {
                                            uniqueVoxels.insert(boundPos);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
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
    
    // Use adaptive height search based on existing voxels
    int maxHeightIncrements = static_cast<int>(MAX_VOXEL_SEARCH_HEIGHT * 100);
    
    // Check all resolutions to find the highest voxel at this exact position
    auto resolutions = getAllResolutions();
    
    for (auto resolution : resolutions) {
        float voxelSize = VoxelData::getVoxelSize(resolution);
        int voxelSizeIncrements = static_cast<int>(voxelSize * 100);
        
        // Calculate which grid cell this position would be in for this resolution
        int gridX = (x / voxelSizeIncrements) * voxelSizeIncrements;
        int gridZ = (z / voxelSizeIncrements) * voxelSizeIncrements;
        
        // Search vertically for voxels at this grid position
        for (int y = 0; y <= maxHeightIncrements; y += voxelSizeIncrements) {
            Math::IncrementCoordinates checkPos(gridX, y, gridZ);
            
            // Check if there's a voxel placed at this exact grid-aligned position
            if (m_voxelManager->getVoxel(checkPos, resolution)) {
                float topHeight = calculateVoxelTopHeight(checkPos, resolution);
                if (topHeight > highestTopHeight) {
                    highestTopHeight = topHeight;
                    highestVoxel = VoxelInfo(checkPos, resolution);
                }
            }
        }
    }
    
    return highestVoxel;
}

std::optional<PlaneDetector::VoxelInfo> PlaneDetector::findHighestVoxelInRadius(const Math::IncrementCoordinates& centerPos, float searchRadius) {
    std::optional<VoxelInfo> bestVoxel;
    float highestHeight = -1.0f;
    
    // Use much smaller search radius to avoid finding distant voxels
    int radiusIncrements = std::min(static_cast<int>(searchRadius * 100), 16); // Max 16cm search instead of 64cm
    
    // Grid search pattern with smaller steps for accuracy  
    for (int dx = -radiusIncrements; dx <= radiusIncrements; dx += 2) { // 2cm steps instead of 4cm
        for (int dz = -radiusIncrements; dz <= radiusIncrements; dz += 2) {
            // Check if within circular radius
            if (dx * dx + dz * dz > radiusIncrements * radiusIncrements) {
                continue;
            }
            
            auto voxel = findHighestVoxelAtPosition(centerPos.x() + dx, centerPos.z() + dz);
            if (voxel.has_value()) {
                float topHeight = calculateVoxelTopHeight(voxel->position, voxel->resolution);
                if (topHeight > highestHeight) {
                    highestHeight = topHeight;
                    bestVoxel = voxel;
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