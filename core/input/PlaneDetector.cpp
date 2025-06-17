#include "PlaneDetector.h"
#include "../voxel_data/VoxelDataManager.h"
#include "../voxel_data/VoxelTypes.h"
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
        
        // Convert world position to grid coordinates
        Math::Vector3i gridPos = worldToGrid(worldPos, resolution);
        
        // Check a range of Y values
        for (int dy = -1; dy <= static_cast<int>(maxHeight / voxelSize); dy++) {
            Math::Vector3i checkPos(gridPos.x, dy, gridPos.z);
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

void PlaneDetector::updatePlanePersistence(const Math::Vector3i& previewPosition, 
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

bool PlaneDetector::previewOverlapsCurrentPlane(const Math::Vector3i& previewPosition, 
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

std::vector<Math::Vector3i> PlaneDetector::getVoxelsAtHeight(float height, float tolerance) const {
    std::vector<Math::Vector3i> voxelsAtHeight;
    
    if (!m_voxelManager) {
        return voxelsAtHeight;
    }
    
    // Search in a reasonable area
    float searchRadius = DEFAULT_SEARCH_RADIUS;
    
    for (auto resolution : getAllResolutions()) {
        float voxelSize = VoxelData::getVoxelSize(resolution);
        
        // Calculate which Y grid level we need to check for this resolution
        int targetYGrid = static_cast<int>(std::round(height / voxelSize)) - 1; // -1 because height is top face
        
        // Only check voxels that could have their top face at the target height
        if (targetYGrid < 0) {
            continue; // Can't have voxels with negative Y
        }
        
        int searchRangeGrid = static_cast<int>(std::ceil(searchRadius / voxelSize));
        
        for (int x = -searchRangeGrid; x <= searchRangeGrid; x++) {
            for (int z = -searchRangeGrid; z <= searchRangeGrid; z++) {
                Math::Vector3i pos(x, targetYGrid, z);
                
                if (m_voxelManager->getVoxel(pos, resolution)) {
                    float voxelTopHeight = calculateVoxelTopHeight(pos, resolution);
                    
                    if (std::abs(voxelTopHeight - height) <= tolerance) {
                        voxelsAtHeight.push_back(pos);
                    }
                }
            }
        }
    }
    
    return voxelsAtHeight;
}

float PlaneDetector::calculateVoxelTopHeight(const Math::Vector3i& voxelPos, VoxelData::VoxelResolution resolution) const {
    float voxelSize = VoxelData::getVoxelSize(resolution);
    return (voxelPos.y + 1) * voxelSize; // +1 because we want the top face
}


void PlaneDetector::reset() {
    m_currentPlane = std::nullopt;
    m_planePersistenceActive = false;
    m_persistenceTimeout = 0.0f;
}

// Private helper methods

std::vector<Math::Vector3i> PlaneDetector::searchVoxelsInCylinder(const Math::Vector3f& centerPos, 
                                                                 float radius, 
                                                                 float minHeight, 
                                                                 float maxHeight) const {
    std::vector<Math::Vector3i> voxels;
    
    if (!m_voxelManager) {
        return voxels;
    }
    
    // Search in all resolutions with proper grid coordinates
    for (auto resolution : getAllResolutions()) {
        float voxelSize = VoxelData::getVoxelSize(resolution);
        
        // Convert world coordinates to grid coordinates for this resolution
        int centerXGrid = static_cast<int>(std::floor(centerPos.x / voxelSize));
        int centerZGrid = static_cast<int>(std::floor(centerPos.z / voxelSize));
        int minYGrid = static_cast<int>(std::floor(minHeight / voxelSize));
        int maxYGrid = static_cast<int>(std::ceil(maxHeight / voxelSize));
        int radiusGrid = static_cast<int>(std::ceil(radius / voxelSize)) + 1; // Add 1 for safety
        
        for (int y = minYGrid; y <= maxYGrid; y++) {
            for (int x = centerXGrid - radiusGrid; x <= centerXGrid + radiusGrid; x++) {
                for (int z = centerZGrid - radiusGrid; z <= centerZGrid + radiusGrid; z++) {
                    // Check if within cylinder in world space
                    Math::Vector3f voxelWorldPos = gridToWorld(Math::Vector3i(x, y, z), resolution);
                    float dx = voxelWorldPos.x + voxelSize/2.0f - centerPos.x; // Check from voxel center
                    float dz = voxelWorldPos.z + voxelSize/2.0f - centerPos.z;
                    float distanceSquared = dx * dx + dz * dz;
                    
                    if (distanceSquared <= radius * radius) {
                        Math::Vector3i gridPos(x, y, z);
                        if (m_voxelManager->getVoxel(gridPos, resolution)) {
                            voxels.push_back(gridPos);
                        }
                    }
                }
            }
        }
    }
    
    return voxels;
}

std::optional<Math::Vector3i> PlaneDetector::findHighestVoxel(const std::vector<Math::Vector3i>& voxels) const {
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

bool PlaneDetector::wouldLargerVoxelOverlapSmaller(const Math::Vector3i& position,
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

Math::Vector3i PlaneDetector::worldToGrid(const Math::Vector3f& worldPos, VoxelData::VoxelResolution resolution) const {
    float voxelSize = VoxelData::getVoxelSize(resolution);
    return Math::Vector3i(
        static_cast<int>(std::floor(worldPos.x / voxelSize)),
        static_cast<int>(std::floor(worldPos.y / voxelSize)),
        static_cast<int>(std::floor(worldPos.z / voxelSize))
    );
}

Math::Vector3f PlaneDetector::gridToWorld(const Math::Vector3i& gridPos, VoxelData::VoxelResolution resolution) const {
    float voxelSize = VoxelData::getVoxelSize(resolution);
    return Math::Vector3f(
        gridPos.x * voxelSize,
        gridPos.y * voxelSize,
        gridPos.z * voxelSize
    );
}

}
}