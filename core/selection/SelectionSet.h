#pragma once

#include "SelectionTypes.h"
#include <unordered_set>
#include <vector>
#include <algorithm>

// Forward declarations
namespace VoxelEditor {
namespace FileIO {
    class BinaryWriter;
    class BinaryReader;
}
}

namespace VoxelEditor {
namespace Selection {

class SelectionSet {
public:
    SelectionSet() = default;
    SelectionSet(const std::vector<VoxelId>& voxels);
    SelectionSet(std::initializer_list<VoxelId> voxels);
    ~SelectionSet() = default;
    
    // Basic operations
    void add(const VoxelId& voxel);
    void remove(const VoxelId& voxel);
    bool contains(const VoxelId& voxel) const;
    void clear();
    
    // Bulk operations
    void addRange(const std::vector<VoxelId>& voxels);
    void removeRange(const std::vector<VoxelId>& voxels);
    void addSet(const SelectionSet& other);
    void removeSet(const SelectionSet& other);
    
    // Set operations
    SelectionSet unionWith(const SelectionSet& other) const;
    SelectionSet intersectWith(const SelectionSet& other) const;
    SelectionSet subtract(const SelectionSet& other) const;
    SelectionSet symmetricDifference(const SelectionSet& other) const;
    
    // In-place set operations
    void unite(const SelectionSet& other);
    void intersect(const SelectionSet& other);
    void subtractFrom(const SelectionSet& other);
    
    // Queries
    size_t size() const { return m_voxels.size(); }
    bool empty() const { return m_voxels.empty(); }
    std::vector<VoxelId> toVector() const;
    Math::BoundingBox getBounds() const;
    Math::Vector3f getCenter() const;
    SelectionStats getStats() const;
    
    // Filtering
    SelectionSet filter(const SelectionPredicate& predicate) const;
    void filterInPlace(const SelectionPredicate& predicate);
    
    // Iteration
    using iterator = std::unordered_set<VoxelId>::iterator;
    using const_iterator = std::unordered_set<VoxelId>::const_iterator;
    
    iterator begin() { return m_voxels.begin(); }
    iterator end() { return m_voxels.end(); }
    const_iterator begin() const { return m_voxels.begin(); }
    const_iterator end() const { return m_voxels.end(); }
    const_iterator cbegin() const { return m_voxels.cbegin(); }
    const_iterator cend() const { return m_voxels.cend(); }
    
    // Visitor pattern
    void forEach(const SelectionVisitor& visitor) const;
    
    // Comparison
    bool operator==(const SelectionSet& other) const;
    bool operator!=(const SelectionSet& other) const;
    
    // Serialization
    void serialize(FileIO::BinaryWriter& writer) const;
    void deserialize(FileIO::BinaryReader& reader);
    
private:
    std::unordered_set<VoxelId> m_voxels;
    mutable std::optional<Math::BoundingBox> m_cachedBounds;
    mutable std::optional<Math::Vector3f> m_cachedCenter;
    
    void invalidateCache() const;
    void updateCache() const;
};

// Utility functions
SelectionSet makeBoxSelection(const Math::BoundingBox& box, VoxelData::VoxelResolution resolution);
SelectionSet makeSphereSelection(const Math::Vector3f& center, float radius, VoxelData::VoxelResolution resolution);
SelectionSet makeCylinderSelection(const Math::Vector3f& base, const Math::Vector3f& direction, 
                                  float radius, float height, VoxelData::VoxelResolution resolution);

}
}