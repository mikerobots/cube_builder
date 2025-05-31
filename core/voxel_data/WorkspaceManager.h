#pragma once

#include "VoxelTypes.h"
#include "../../foundation/math/Vector3f.h"
#include "../../foundation/events/EventDispatcher.h"
#include "../../foundation/events/CommonEvents.h"
#include <functional>

// Forward declaration for memory estimation
namespace VoxelEditor { namespace VoxelData { class OctreeNode; }}

namespace VoxelEditor {
namespace VoxelData {

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
    
    // Convenience getters
    float getWidth() const { return m_size.x; }
    float getHeight() const { return m_size.y; }
    float getDepth() const { return m_size.z; }
    
    // Volume calculation
    float getVolume() const {
        return m_size.x * m_size.y * m_size.z;
    }
    
    // Bounds calculation
    Math::Vector3f getMinBounds() const {
        return -m_size * 0.5f;
    }
    
    Math::Vector3f getMaxBounds() const {
        return m_size * 0.5f;
    }
    
    void getBounds(Math::Vector3f& minBounds, Math::Vector3f& maxBounds) const {
        minBounds = getMinBounds();
        maxBounds = getMaxBounds();
    }
    
    // Position validation
    bool isPositionInBounds(const Math::Vector3f& worldPos) const {
        Math::Vector3f halfSize = m_size * 0.5f;
        return worldPos.x >= -halfSize.x && worldPos.x <= halfSize.x &&
               worldPos.y >= -halfSize.y && worldPos.y <= halfSize.y &&
               worldPos.z >= -halfSize.z && worldPos.z <= halfSize.z;
    }
    
    bool isVoxelPositionValid(const VoxelPosition& voxelPos) const {
        return ::VoxelEditor::VoxelData::isPositionInBounds(voxelPos.gridPos, voxelPos.resolution, m_size);
    }
    
    // Clamping utilities
    Math::Vector3f clampPosition(const Math::Vector3f& worldPos) const {
        Math::Vector3f halfSize = m_size * 0.5f;
        return Math::Vector3f(
            std::clamp(worldPos.x, -halfSize.x, halfSize.x),
            std::clamp(worldPos.y, -halfSize.y, halfSize.y),
            std::clamp(worldPos.z, -halfSize.z, halfSize.z)
        );
    }
    
    // Distance calculations
    float getDistanceToEdge(const Math::Vector3f& worldPos) const {
        Math::Vector3f halfSize = m_size * 0.5f;
        Math::Vector3f distances = halfSize - Math::Vector3f(
            std::abs(worldPos.x),
            std::abs(worldPos.y),
            std::abs(worldPos.z)
        );
        
        return std::min({distances.x, distances.y, distances.z});
    }
    
    bool isNearEdge(const Math::Vector3f& worldPos, float threshold = 0.1f) const {
        return getDistanceToEdge(worldPos) < threshold;
    }
    
    // Grid information for different resolutions
    Math::Vector3i getMaxGridDimensions(VoxelResolution resolution) const {
        return calculateMaxGridDimensions(resolution, m_size);
    }
    
    size_t getMaxVoxelCount(VoxelResolution resolution) const {
        Math::Vector3i dims = getMaxGridDimensions(resolution);
        return static_cast<size_t>(dims.x) * static_cast<size_t>(dims.y) * static_cast<size_t>(dims.z);
    }
    
    float getVoxelDensity(VoxelResolution resolution, size_t actualVoxelCount) const {
        size_t maxVoxels = getMaxVoxelCount(resolution);
        if (maxVoxels == 0) return 0.0f;
        return static_cast<float>(actualVoxelCount) / static_cast<float>(maxVoxels);
    }
    
    // Memory estimation (rough estimate)
    size_t estimateMemoryUsage(VoxelResolution resolution, float fillRatio = 0.1f) const {
        size_t maxVoxels = getMaxVoxelCount(resolution);
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