#include "SelectionSet.h"
#include <numeric>

namespace VoxelEditor {
namespace Selection {

SelectionSet::SelectionSet(const std::vector<VoxelId>& voxels) {
    addRange(voxels);
}

SelectionSet::SelectionSet(std::initializer_list<VoxelId> voxels) {
    for (const auto& voxel : voxels) {
        add(voxel);
    }
}

void SelectionSet::add(const VoxelId& voxel) {
    if (m_voxels.insert(voxel).second) {
        invalidateCache();
    }
}

void SelectionSet::remove(const VoxelId& voxel) {
    if (m_voxels.erase(voxel) > 0) {
        invalidateCache();
    }
}

bool SelectionSet::contains(const VoxelId& voxel) const {
    return m_voxels.find(voxel) != m_voxels.end();
}

void SelectionSet::clear() {
    m_voxels.clear();
    invalidateCache();
}

void SelectionSet::addRange(const std::vector<VoxelId>& voxels) {
    size_t oldSize = m_voxels.size();
    m_voxels.insert(voxels.begin(), voxels.end());
    if (m_voxels.size() != oldSize) {
        invalidateCache();
    }
}

void SelectionSet::removeRange(const std::vector<VoxelId>& voxels) {
    size_t oldSize = m_voxels.size();
    for (const auto& voxel : voxels) {
        m_voxels.erase(voxel);
    }
    if (m_voxels.size() != oldSize) {
        invalidateCache();
    }
}

void SelectionSet::addSet(const SelectionSet& other) {
    size_t oldSize = m_voxels.size();
    m_voxels.insert(other.begin(), other.end());
    if (m_voxels.size() != oldSize) {
        invalidateCache();
    }
}

void SelectionSet::removeSet(const SelectionSet& other) {
    size_t oldSize = m_voxels.size();
    for (const auto& voxel : other) {
        m_voxels.erase(voxel);
    }
    if (m_voxels.size() != oldSize) {
        invalidateCache();
    }
}

SelectionSet SelectionSet::unionWith(const SelectionSet& other) const {
    SelectionSet result(*this);
    result.unite(other);
    return result;
}

SelectionSet SelectionSet::intersectWith(const SelectionSet& other) const {
    SelectionSet result;
    for (const auto& voxel : m_voxels) {
        if (other.contains(voxel)) {
            result.add(voxel);
        }
    }
    return result;
}

SelectionSet SelectionSet::subtract(const SelectionSet& other) const {
    SelectionSet result(*this);
    result.subtractFrom(other);
    return result;
}

SelectionSet SelectionSet::symmetricDifference(const SelectionSet& other) const {
    SelectionSet result;
    
    // Add elements in this but not in other
    for (const auto& voxel : m_voxels) {
        if (!other.contains(voxel)) {
            result.add(voxel);
        }
    }
    
    // Add elements in other but not in this
    for (const auto& voxel : other) {
        if (!contains(voxel)) {
            result.add(voxel);
        }
    }
    
    return result;
}

void SelectionSet::unite(const SelectionSet& other) {
    addSet(other);
}

void SelectionSet::intersect(const SelectionSet& other) {
    std::unordered_set<VoxelId> result;
    for (const auto& voxel : m_voxels) {
        if (other.contains(voxel)) {
            result.insert(voxel);
        }
    }
    m_voxels = std::move(result);
    invalidateCache();
}

void SelectionSet::subtractFrom(const SelectionSet& other) {
    removeSet(other);
}

std::vector<VoxelId> SelectionSet::toVector() const {
    return std::vector<VoxelId>(m_voxels.begin(), m_voxels.end());
}

Math::BoundingBox SelectionSet::getBounds() const {
    if (!m_cachedBounds.has_value()) {
        updateCache();
    }
    return m_cachedBounds.value();
}

Math::Vector3f SelectionSet::getCenter() const {
    if (!m_cachedCenter.has_value()) {
        updateCache();
    }
    return m_cachedCenter.value();
}

SelectionStats SelectionSet::getStats() const {
    SelectionStats stats;
    stats.voxelCount = m_voxels.size();
    
    if (empty()) {
        return stats;
    }
    
    // Count by resolution and calculate volume
    for (const auto& voxel : m_voxels) {
        stats.countByResolution[voxel.resolution]++;
        float voxelSize = voxel.getVoxelSize();
        stats.totalVolume += voxelSize * voxelSize * voxelSize;
    }
    
    stats.bounds = getBounds();
    stats.center = getCenter();
    
    return stats;
}

SelectionSet SelectionSet::filter(const SelectionPredicate& predicate) const {
    SelectionSet result;
    for (const auto& voxel : m_voxels) {
        if (predicate(voxel)) {
            result.add(voxel);
        }
    }
    return result;
}

void SelectionSet::filterInPlace(const SelectionPredicate& predicate) {
    std::unordered_set<VoxelId> filtered;
    for (const auto& voxel : m_voxels) {
        if (predicate(voxel)) {
            filtered.insert(voxel);
        }
    }
    m_voxels = std::move(filtered);
    invalidateCache();
}

void SelectionSet::forEach(const SelectionVisitor& visitor) const {
    for (const auto& voxel : m_voxels) {
        visitor(voxel);
    }
}

bool SelectionSet::operator==(const SelectionSet& other) const {
    return m_voxels == other.m_voxels;
}

bool SelectionSet::operator!=(const SelectionSet& other) const {
    return !(*this == other);
}

void SelectionSet::invalidateCache() const {
    m_cachedBounds.reset();
    m_cachedCenter.reset();
}

void SelectionSet::updateCache() const {
    if (empty()) {
        m_cachedBounds = Math::BoundingBox();
        m_cachedCenter = Math::Vector3f::Zero();
        return;
    }
    
    // Calculate bounds and center
    Math::Vector3f minPos(std::numeric_limits<float>::max());
    Math::Vector3f maxPos(std::numeric_limits<float>::lowest());
    Math::Vector3f totalPos = Math::Vector3f::Zero();
    
    for (const auto& voxel : m_voxels) {
        Math::BoundingBox voxelBounds = voxel.getBounds();
        minPos = Math::Vector3f::min(minPos, voxelBounds.min);
        maxPos = Math::Vector3f::max(maxPos, voxelBounds.max);
        totalPos = totalPos + voxel.getWorldPosition();
    }
    
    m_cachedBounds = Math::BoundingBox(minPos, maxPos);
    m_cachedCenter = totalPos / static_cast<float>(m_voxels.size());
}

// Utility function implementations
SelectionSet makeBoxSelection(const Math::BoundingBox& box, VoxelData::VoxelResolution resolution) {
    SelectionSet result;
    
    float voxelSize = VoxelId(Math::Vector3i::Zero(), resolution).getVoxelSize();
    
    // Calculate voxel range
    Math::Vector3i minVoxel(
        static_cast<int>(std::floor(box.min.x / voxelSize)),
        static_cast<int>(std::floor(box.min.y / voxelSize)),
        static_cast<int>(std::floor(box.min.z / voxelSize))
    );
    
    Math::Vector3i maxVoxel(
        static_cast<int>(std::ceil(box.max.x / voxelSize)),
        static_cast<int>(std::ceil(box.max.y / voxelSize)),
        static_cast<int>(std::ceil(box.max.z / voxelSize))
    );
    
    // Add all voxels in range
    for (int x = minVoxel.x; x <= maxVoxel.x; ++x) {
        for (int y = minVoxel.y; y <= maxVoxel.y; ++y) {
            for (int z = minVoxel.z; z <= maxVoxel.z; ++z) {
                VoxelId voxel(Math::Vector3i(x, y, z), resolution);
                if (box.intersects(voxel.getBounds())) {
                    result.add(voxel);
                }
            }
        }
    }
    
    return result;
}

SelectionSet makeSphereSelection(const Math::Vector3f& center, float radius, VoxelData::VoxelResolution resolution) {
    SelectionSet result;
    
    float voxelSize = VoxelId(Math::Vector3i::Zero(), resolution).getVoxelSize();
    float radiusSq = radius * radius;
    
    // Calculate bounding box of sphere
    Math::Vector3f radiusVec(radius, radius, radius);
    Math::BoundingBox sphereBounds(center - radiusVec, center + radiusVec);
    
    // Get voxel range
    Math::Vector3i minVoxel(
        static_cast<int>(std::floor(sphereBounds.min.x / voxelSize)),
        static_cast<int>(std::floor(sphereBounds.min.y / voxelSize)),
        static_cast<int>(std::floor(sphereBounds.min.z / voxelSize))
    );
    
    Math::Vector3i maxVoxel(
        static_cast<int>(std::ceil(sphereBounds.max.x / voxelSize)),
        static_cast<int>(std::ceil(sphereBounds.max.y / voxelSize)),
        static_cast<int>(std::ceil(sphereBounds.max.z / voxelSize))
    );
    
    // Add voxels within sphere
    for (int x = minVoxel.x; x <= maxVoxel.x; ++x) {
        for (int y = minVoxel.y; y <= maxVoxel.y; ++y) {
            for (int z = minVoxel.z; z <= maxVoxel.z; ++z) {
                VoxelId voxel(Math::Vector3i(x, y, z), resolution);
                Math::Vector3f voxelCenter = voxel.getWorldPosition();
                float distSq = (voxelCenter - center).lengthSquared();
                if (distSq <= radiusSq) {
                    result.add(voxel);
                }
            }
        }
    }
    
    return result;
}

SelectionSet makeCylinderSelection(const Math::Vector3f& base, const Math::Vector3f& direction, 
                                  float radius, float height, VoxelData::VoxelResolution resolution) {
    SelectionSet result;
    
    float voxelSize = VoxelId(Math::Vector3i::Zero(), resolution).getVoxelSize();
    Math::Vector3f normalizedDir = direction.normalized();
    Math::Vector3f top = base + normalizedDir * height;
    
    // Calculate bounding box of cylinder
    Math::Vector3f radiusVec(radius, radius, radius);
    Math::BoundingBox cylinderBounds(
        Math::Vector3f::min(base, top) - radiusVec,
        Math::Vector3f::max(base, top) + radiusVec
    );
    
    // Get voxel range
    Math::Vector3i minVoxel(
        static_cast<int>(std::floor(cylinderBounds.min.x / voxelSize)),
        static_cast<int>(std::floor(cylinderBounds.min.y / voxelSize)),
        static_cast<int>(std::floor(cylinderBounds.min.z / voxelSize))
    );
    
    Math::Vector3i maxVoxel(
        static_cast<int>(std::ceil(cylinderBounds.max.x / voxelSize)),
        static_cast<int>(std::ceil(cylinderBounds.max.y / voxelSize)),
        static_cast<int>(std::ceil(cylinderBounds.max.z / voxelSize))
    );
    
    // Add voxels within cylinder
    for (int x = minVoxel.x; x <= maxVoxel.x; ++x) {
        for (int y = minVoxel.y; y <= maxVoxel.y; ++y) {
            for (int z = minVoxel.z; z <= maxVoxel.z; ++z) {
                VoxelId voxel(Math::Vector3i(x, y, z), resolution);
                Math::Vector3f voxelCenter = voxel.getWorldPosition();
                
                // Project point onto cylinder axis
                Math::Vector3f toPoint = voxelCenter - base;
                float projLength = toPoint.dot(normalizedDir);
                
                // Check if within height range
                if (projLength >= 0 && projLength <= height) {
                    // Calculate distance from axis
                    Math::Vector3f projPoint = base + normalizedDir * projLength;
                    float distFromAxis = (voxelCenter - projPoint).length();
                    
                    if (distFromAxis <= radius) {
                        result.add(voxel);
                    }
                }
            }
        }
    }
    
    return result;
}

}
}