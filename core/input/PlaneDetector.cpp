#include "PlaneDetector.h"
#include "../voxel_data/VoxelDataManager.h"
#include "../voxel_data/VoxelTypes.h"
#include "../../foundation/math/CoordinateConverter.h"
#include <algorithm>
#include <cmath>

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
    
    // Search in a cylinder from ground to reasonable height
    float maxHeight = MAX_VOXEL_SEARCH_HEIGHT;
    
    // Store voxel candidates with their resolutions for proper height calculation
    std::vector<VoxelInfo> voxelCandidates;
    
    // Check all resolutions
    for (auto resolution : getAllResolutions()) {
        float voxelSize = VoxelData::getVoxelSize(resolution);
        
        // Convert world position to increment coordinates
        Math::IncrementCoordinates incrementPos = Math::CoordinateConverter::worldToIncrement(Math::WorldCoordinates(worldPos));
        
        // Check a range of Y values (in increment coordinates, each unit is 1cm)
        int maxHeightIncrements = static_cast<int>(maxHeight * 100); // Convert meters to cm
        for (int dy = -1; dy <= maxHeightIncrements; dy++) {
            Math::IncrementCoordinates checkPos(incrementPos.x(), dy, incrementPos.z());
            if (m_voxelManager->getVoxel(checkPos, resolution)) {
                voxelCandidates.emplace_back(checkPos, resolution);
            }
        }
    }
    
    // Find the highest voxel by calculating actual top heights
    if (!voxelCandidates.empty()) {
        auto highestIt = voxelCandidates.begin();
        float highestHeight = calculateVoxelTopHeight(highestIt->position, highestIt->resolution);
        
        for (auto it = voxelCandidates.begin(); it != voxelCandidates.end(); ++it) {
            float height = calculateVoxelTopHeight(it->position, it->resolution);
            if (height > highestHeight) {
                highestHeight = height;
                highestIt = it;
            }
        }
        
        return *highestIt;
    }
    
    // If no voxel directly under cursor, search in a wider area
    auto voxelsInCylinder = searchVoxelsInCylinder(worldPos, searchRadius, 0.0f, maxHeight);
    if (!voxelsInCylinder.empty()) {
        auto highestVoxel = findHighestVoxel(voxelsInCylinder);
        if (highestVoxel.has_value()) {
            // Need to determine resolution for this voxel
            for (auto resolution : getAllResolutions()) {
                if (m_voxelManager->getVoxel(highestVoxel.value(), resolution)) {
                    return VoxelInfo(highestVoxel.value(), resolution);
                }
            }
        }
    }
    
    return std::nullopt;
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
    
    // Search in a reasonable area using increment coordinates
    float searchRadius = DEFAULT_SEARCH_RADIUS;
    int searchRadiusIncrement = static_cast<int>(searchRadius * 100); // Convert to cm
    
    // For each position in the search area, check all resolutions
    for (int x = -searchRadiusIncrement; x <= searchRadiusIncrement; x++) {
        for (int z = -searchRadiusIncrement; z <= searchRadiusIncrement; z++) {
            // Try different Y positions around the target height
            for (auto resolution : getAllResolutions()) {
                float voxelSize = VoxelData::getVoxelSize(resolution);
                
                // Calculate approximate Y increment position for this resolution
                // Since voxels can be of different sizes, we need to check a range
                int baseYIncrement = static_cast<int>(height * 100); // Convert height to cm
                int voxelSizeIncrement = static_cast<int>(voxelSize * 100); // Convert voxel size to cm
                
                // Check a range of Y positions where voxels of this resolution might have their top at target height
                for (int yOffset = -voxelSizeIncrement; yOffset <= 0; yOffset++) {
                    Math::IncrementCoordinates pos(x, baseYIncrement + yOffset, z);
                    
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
    
    // Search in all resolutions at each increment position
    for (int y = minYIncrement; y <= maxYIncrement; y++) {
        for (int x = centerIncrement.x() - radiusIncrement; x <= centerIncrement.x() + radiusIncrement; x++) {
            for (int z = centerIncrement.z() - radiusIncrement; z <= centerIncrement.z() + radiusIncrement; z++) {
                Math::IncrementCoordinates checkPos(x, y, z);
                
                // Convert to world position for distance check
                Math::WorldCoordinates worldCoords = Math::CoordinateConverter::incrementToWorld(checkPos);
                Math::Vector3f voxelWorldPos = worldCoords.value();
                
                // Check if within cylinder (distance from center in XZ plane)
                float dx = voxelWorldPos.x - centerPos.x;
                float dz = voxelWorldPos.z - centerPos.z;
                float distanceSquared = dx * dx + dz * dz;
                
                if (distanceSquared <= radius * radius) {
                    // Check if there's a voxel at this position in any resolution
                    for (auto resolution : getAllResolutions()) {
                        if (m_voxelManager->getVoxel(checkPos, resolution)) {
                            voxels.push_back(checkPos);
                            break; // Found one, don't need to check other resolutions
                        }
                    }
                }
            }
        }
    }
    
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