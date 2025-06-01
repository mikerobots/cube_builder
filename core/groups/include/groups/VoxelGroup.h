#pragma once

#include "GroupTypes.h"
#include <unordered_set>
#include <memory>
#include <mutex>

namespace VoxelEditor {
namespace Groups {

class VoxelGroup {
public:
    VoxelGroup(GroupId id, const std::string& name);
    ~VoxelGroup() = default;
    
    // Basic properties
    GroupId getId() const { return m_id; }
    const std::string& getName() const { return m_metadata.name; }
    void setName(const std::string& name);
    
    const GroupMetadata& getMetadata() const { return m_metadata; }
    void setMetadata(const GroupMetadata& metadata);
    
    // Color management
    const Rendering::Color& getColor() const { return m_metadata.color; }
    void setColor(const Rendering::Color& color);
    
    // Visibility
    bool isVisible() const { return m_metadata.visible; }
    void setVisible(bool visible);
    
    float getOpacity() const { return m_metadata.opacity; }
    void setOpacity(float opacity);
    
    // Locking
    bool isLocked() const { return m_metadata.locked; }
    void setLocked(bool locked);
    
    // Voxel membership
    bool addVoxel(const VoxelId& voxel);
    bool removeVoxel(const VoxelId& voxel);
    bool containsVoxel(const VoxelId& voxel) const;
    void clearVoxels();
    
    const std::unordered_set<VoxelId>& getVoxels() const { return m_voxels; }
    std::vector<VoxelId> getVoxelList() const;
    size_t getVoxelCount() const { return m_voxels.size(); }
    bool isEmpty() const { return m_voxels.empty(); }
    
    // Bounding box
    Math::BoundingBox getBoundingBox() const;
    void invalidateBounds() { m_boundsValid = false; }
    
    // Pivot point
    Math::Vector3f getPivot() const { return m_metadata.pivot; }
    void setPivot(const Math::Vector3f& pivot);
    Math::Vector3f getCenter() const;
    
    // Transform operations
    void translate(const Math::Vector3f& offset);
    void rotate(const Math::Vector3f& eulerAngles, const Math::Vector3f& pivot);
    void scale(float factor, const Math::Vector3f& pivot);
    
    // Utility
    GroupInfo getInfo() const;
    void updateModified() { m_metadata.updateModified(); }
    
    // Thread safety
    std::mutex& getMutex() const { return m_mutex; }
    
private:
    GroupId m_id;
    GroupMetadata m_metadata;
    std::unordered_set<VoxelId> m_voxels;
    
    // Cached bounding box
    mutable Math::BoundingBox m_bounds;
    mutable bool m_boundsValid = false;
    mutable std::mutex m_mutex;
    
    void updateBounds() const;
    float getVoxelSize(VoxelData::VoxelResolution resolution) const;
};

} // namespace Groups
} // namespace VoxelEditor