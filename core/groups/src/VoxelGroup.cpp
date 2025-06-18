#include "../include/groups/VoxelGroup.h"
#include <algorithm>
#include <limits>

namespace VoxelEditor {
namespace Groups {

VoxelGroup::VoxelGroup(GroupId id, const std::string& name)
    : m_id(id) {
    m_metadata.name = name;
    m_metadata.color = GroupColorPalette::getColorForIndex(id % 10);
}

void VoxelGroup::setName(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metadata.name = name;
    m_metadata.updateModified();
}

void VoxelGroup::setMetadata(const GroupMetadata& metadata) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metadata = metadata;
    m_metadata.updateModified();
}

void VoxelGroup::setColor(const Rendering::Color& color) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metadata.color = color;
    m_metadata.updateModified();
}

void VoxelGroup::setVisible(bool visible) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metadata.visible = visible;
    m_metadata.updateModified();
}

void VoxelGroup::setOpacity(float opacity) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metadata.opacity = std::max(0.0f, std::min(1.0f, opacity));
    m_metadata.updateModified();
}

void VoxelGroup::setLocked(bool locked) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metadata.locked = locked;
    m_metadata.updateModified();
}

bool VoxelGroup::addVoxel(const VoxelId& voxel) {
    std::lock_guard<std::mutex> lock(m_mutex);
    bool inserted = m_voxels.insert(voxel).second;
    if (inserted) {
        m_boundsValid = false;
        m_metadata.updateModified();
    }
    return inserted;
}

bool VoxelGroup::removeVoxel(const VoxelId& voxel) {
    std::lock_guard<std::mutex> lock(m_mutex);
    bool removed = m_voxels.erase(voxel) > 0;
    if (removed) {
        m_boundsValid = false;
        m_metadata.updateModified();
    }
    return removed;
}

bool VoxelGroup::containsVoxel(const VoxelId& voxel) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_voxels.find(voxel) != m_voxels.end();
}

void VoxelGroup::clearVoxels() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_voxels.clear();
    m_boundsValid = false;
    m_metadata.updateModified();
}

std::vector<VoxelId> VoxelGroup::getVoxelList() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::vector<VoxelId>(m_voxels.begin(), m_voxels.end());
}

Math::BoundingBox VoxelGroup::getBoundingBox() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_boundsValid) {
        updateBounds();
    }
    return m_bounds;
}

void VoxelGroup::setPivot(const Math::Vector3f& pivot) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metadata.pivot = pivot;
    m_metadata.updateModified();
}

Math::Vector3f VoxelGroup::getCenter() const {
    return getBoundingBox().getCenter();
}

void VoxelGroup::translate(const Math::Vector3f& offset) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Create new voxel set with translated positions
    std::unordered_set<VoxelId> newVoxels;
    for (const auto& voxel : m_voxels) {
        VoxelId newVoxel = voxel;
        
        // Use VoxelId's coordinate conversion method instead of direct multiplication
        // This ensures we use the centered coordinate system properly
        Math::WorldCoordinates worldCoords = voxel.getWorldPosition();
        Math::Vector3f worldPos = worldCoords.value();
        worldPos = worldPos + offset;
        
        // Convert back to increment coordinates using the centralized converter
        Math::IncrementCoordinates newIncrementCoords = Math::CoordinateConverter::worldToIncrement(
            Math::WorldCoordinates(worldPos));
        
        newVoxel.position = newIncrementCoords;
        
        newVoxels.insert(newVoxel);
    }
    
    m_voxels = std::move(newVoxels);
    m_boundsValid = false;
    m_metadata.updateModified();
}

void VoxelGroup::rotate(const Math::Vector3f& eulerAngles, const Math::Vector3f& pivot) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // TODO: Implement rotation
    // This is a complex operation that requires:
    // 1. Converting voxel positions to world coordinates
    // 2. Applying rotation matrix around pivot
    // 3. Converting back to voxel coordinates
    // 4. Handling potential precision issues
    
    m_metadata.updateModified();
}

void VoxelGroup::scale(float factor, const Math::Vector3f& pivot) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // TODO: Implement scaling
    // Similar to rotation, this requires careful coordinate transformation
    
    m_metadata.updateModified();
}

GroupInfo VoxelGroup::getInfo() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    GroupInfo info(m_id, m_metadata);
    info.voxelCount = m_voxels.size();
    if (!m_boundsValid) {
        updateBounds();
    }
    info.bounds = m_bounds;
    return info;
}

void VoxelGroup::updateBounds() const {
    if (m_voxels.empty()) {
        m_bounds = Math::BoundingBox();
        m_boundsValid = true;
        return;
    }
    
    Math::Vector3f minBound(std::numeric_limits<float>::max());
    Math::Vector3f maxBound(std::numeric_limits<float>::lowest());
    
    for (const auto& voxel : m_voxels) {
        // Use VoxelId's getBounds() method which properly handles centered coordinate system
        Math::BoundingBox voxelBounds = voxel.getBounds();
        
        minBound.x = std::min(minBound.x, voxelBounds.min.x);
        minBound.y = std::min(minBound.y, voxelBounds.min.y);
        minBound.z = std::min(minBound.z, voxelBounds.min.z);
        
        maxBound.x = std::max(maxBound.x, voxelBounds.max.x);
        maxBound.y = std::max(maxBound.y, voxelBounds.max.y);
        maxBound.z = std::max(maxBound.z, voxelBounds.max.z);
    }
    
    m_bounds = Math::BoundingBox(minBound, maxBound);
    m_boundsValid = true;
}

float VoxelGroup::getVoxelSize(VoxelData::VoxelResolution resolution) const {
    return VoxelData::getVoxelSize(resolution);
}

} // namespace Groups
} // namespace VoxelEditor