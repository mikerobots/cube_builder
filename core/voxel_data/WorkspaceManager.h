#pragma once

#include <functional>
#include <algorithm>

#include "VoxelTypes.h"
#include "../../foundation/math/Vector3f.h"
#include "../../foundation/events/EventDispatcher.h"
#include "../../foundation/events/CommonEvents.h"

namespace VoxelEditor {
namespace VoxelData {

// Forward declaration for memory estimation
class OctreeNode;

class WorkspaceManager {
public:
    using SizeChangeCallback = std::function<bool(const Math::Vector3f&, const Math::Vector3f&)>;
    
    explicit WorkspaceManager(Events::EventDispatcher* eventDispatcher = nullptr)
        : m_size(WorkspaceConstraints::DEFAULT_SIZE, 
                WorkspaceConstraints::DEFAULT_SIZE, 
                WorkspaceConstraints::DEFAULT_SIZE)
        , m_eventDispatcher(eventDispatcher) {}
    
    // Workspace size management
    bool setSize(const Math::Vector3f& newSize) {
        if (!WorkspaceConstraints::isValidSize(newSize)) {
            return false;
        }
        
        Math::Vector3f oldSize = m_size;
        
        // Check with callback if resize is allowed
        if (m_sizeChangeCallback && !m_sizeChangeCallback(oldSize, newSize)) {
            return false;
        }
        
        m_size = newSize;
        
        // Dispatch event
        if (m_eventDispatcher) {
            Events::WorkspaceResizedEvent event(oldSize, newSize);
            m_eventDispatcher->dispatch(event);
        }
        
        return true;
    }
    
    bool setSize(float size) {
        return setSize(Math::Vector3f(size, size, size));
    }
    
    const Math::Vector3f& getSize() const {
        return m_size;
    }
    
    float getVolume() const {
        return m_size.x * m_size.y * m_size.z;
    }
    
    float getMinDimension() const {
        return std::min({m_size.x, m_size.y, m_size.z});
    }
    
    float getMaxDimension() const {
        return std::max({m_size.x, m_size.y, m_size.z});
    }
    
    // Check if workspace is cubic
    bool isCubic() const {
        return (m_size.x == m_size.y) && (m_size.y == m_size.z);
    }
    
    // Get workspace bounds (centered at origin)
    Math::Vector3f getMinBounds() const {
        return Math::Vector3f(-m_size.x * 0.5f, 0.0f, -m_size.z * 0.5f);
    }
    
    Math::Vector3f getMaxBounds() const {
        return Math::Vector3f(m_size.x * 0.5f, m_size.y, m_size.z * 0.5f);
    }
    
    Math::Vector3f getCenter() const {
        return Math::Vector3f(0.0f, m_size.y * 0.5f, 0.0f);
    }
    
    // Position validation (centered workspace, Y >= 0)
    bool isPositionValid(const Math::Vector3f& position) const {
        float halfX = m_size.x * 0.5f;
        float halfZ = m_size.z * 0.5f;
        return position.x >= -halfX && position.x <= halfX &&
               position.y >= 0 && position.y <= m_size.y &&
               position.z >= -halfZ && position.z <= halfZ;
    }
    
    bool isVoxelPositionValid(const VoxelPosition& voxelPos) const {
        Math::Vector3f worldPos = voxelPos.toWorldSpace(m_size);
        return isPositionValid(worldPos);
    }
    
    // Grid position validation (for specific resolution)
    bool isGridPositionValid(const Math::Vector3i& gridPos, VoxelResolution resolution) const {
        Math::Vector3i maxDims = getMaxGridDimensions(resolution);
        return gridPos.x >= 0 && gridPos.x < maxDims.x &&
               gridPos.y >= 0 && gridPos.y < maxDims.y &&
               gridPos.z >= 0 && gridPos.z < maxDims.z;
    }
    
    // Clamp position to workspace bounds
    Math::Vector3f clampPosition(const Math::Vector3f& position) const {
        float halfX = m_size.x * 0.5f;
        float halfZ = m_size.z * 0.5f;
        return Math::Vector3f(
            std::clamp(position.x, -halfX, halfX),
            std::clamp(position.y, 0.0f, m_size.y),
            std::clamp(position.z, -halfZ, halfZ)
        );
    }
    
    // Get maximum voxel counts for each resolution
    int getMaxVoxelCount(VoxelResolution resolution) const {
        Math::Vector3i maxDims = getMaxGridDimensions(resolution);
        return maxDims.x * maxDims.y * maxDims.z;
    }
    
    Math::Vector3i getMaxGridDimensions(VoxelResolution resolution) const {
        return calculateMaxGridDimensions(resolution, m_size);
    }
    
    // Memory estimation
    size_t estimateMemoryUsage(VoxelResolution resolution, float fillRatio = 0.1f) const {
        // Base memory for workspace management
        size_t baseMemory = sizeof(WorkspaceManager) + sizeof(Math::Vector3f);
        
        // Estimate voxel storage
        int maxVoxels = getMaxVoxelCount(resolution);
        size_t estimatedVoxels = static_cast<size_t>(maxVoxels * fillRatio);
        
        // Rough estimate: each voxel might need 2-3 octree nodes on average
        // Using approximate node size without exact type dependency
        return estimatedVoxels * 2 * 64; // Rough estimate of 64 bytes per node
    }
    
    // Constraint validation
    static bool isValidSize(const Math::Vector3f& size) {
        return WorkspaceConstraints::isValidSize(size);
    }
    
    static bool isValidSize(float size) {
        return WorkspaceConstraints::isValidSize(size);
    }
    
    static Math::Vector3f clampSize(const Math::Vector3f& size) {
        return WorkspaceConstraints::clampSize(size);
    }
    
    static float getMinSize() { return WorkspaceConstraints::MIN_SIZE; }
    static float getMaxSize() { return WorkspaceConstraints::MAX_SIZE; }
    static float getDefaultSize() { return WorkspaceConstraints::DEFAULT_SIZE; }
    
    // Callback management
    void setSizeChangeCallback(SizeChangeCallback callback) {
        m_sizeChangeCallback = callback;
    }
    
    void clearSizeChangeCallback() {
        m_sizeChangeCallback = nullptr;
    }
    
    // Event dispatcher
    void setEventDispatcher(Events::EventDispatcher* eventDispatcher) {
        m_eventDispatcher = eventDispatcher;
    }
    
    // Reset to defaults
    void reset() {
        Math::Vector3f oldSize = m_size;
        m_size = Math::Vector3f(WorkspaceConstraints::DEFAULT_SIZE, 
                              WorkspaceConstraints::DEFAULT_SIZE, 
                              WorkspaceConstraints::DEFAULT_SIZE);
        
        if (m_eventDispatcher) {
            Events::WorkspaceResizedEvent event(oldSize, m_size);
            m_eventDispatcher->dispatch(event);
        }
    }
    
    // Utility methods for common sizes
    bool setToMinimumSize() {
        return setSize(WorkspaceConstraints::MIN_SIZE);
    }
    
    bool setToMaximumSize() {
        return setSize(WorkspaceConstraints::MAX_SIZE);
    }
    
    bool setToDefaultSize() {
        return setSize(WorkspaceConstraints::DEFAULT_SIZE);
    }
    
    // Check if workspace is at limits
    bool isAtMinimumSize() const {
        return m_size.x <= WorkspaceConstraints::MIN_SIZE &&
               m_size.y <= WorkspaceConstraints::MIN_SIZE &&
               m_size.z <= WorkspaceConstraints::MIN_SIZE;
    }
    
    bool isAtMaximumSize() const {
        return m_size.x >= WorkspaceConstraints::MAX_SIZE &&
               m_size.y >= WorkspaceConstraints::MAX_SIZE &&
               m_size.z >= WorkspaceConstraints::MAX_SIZE;
    }
    
    bool isDefaultSize() const {
        return m_size.x == WorkspaceConstraints::DEFAULT_SIZE &&
               m_size.y == WorkspaceConstraints::DEFAULT_SIZE &&
               m_size.z == WorkspaceConstraints::DEFAULT_SIZE;
    }
    
private:
    Math::Vector3f m_size;
    Events::EventDispatcher* m_eventDispatcher;
    SizeChangeCallback m_sizeChangeCallback;
};

}
}