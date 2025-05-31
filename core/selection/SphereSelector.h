#pragma once

#include "SelectionTypes.h"
#include "SelectionSet.h"
#include "../../foundation/math/Ray.h"
#include "../../foundation/math/Quaternion.h"
#include "../voxel_data/VoxelDataManager.h"

namespace VoxelEditor {
namespace Selection {

class SphereSelector {
public:
    explicit SphereSelector(VoxelData::VoxelDataManager* voxelManager = nullptr);
    ~SphereSelector() = default;
    
    // Set the voxel manager
    void setVoxelManager(VoxelData::VoxelDataManager* manager) { m_voxelManager = manager; }
    
    // Sphere selection from center and radius
    SelectionSet selectFromSphere(const Math::Vector3f& center,
                                float radius,
                                VoxelData::VoxelResolution resolution,
                                bool checkExistence = true);
    
    // Sphere selection from ray (select sphere at intersection point)
    SelectionSet selectFromRay(const Math::Ray& ray,
                             float radius,
                             float maxDistance,
                             VoxelData::VoxelResolution resolution);
    
    // Ellipsoid selection
    SelectionSet selectEllipsoid(const Math::Vector3f& center,
                               const Math::Vector3f& radii,
                               const Math::Quaternion& rotation,
                               VoxelData::VoxelResolution resolution,
                               bool checkExistence = true);
    
    // Hemisphere selection
    SelectionSet selectHemisphere(const Math::Vector3f& center,
                                float radius,
                                const Math::Vector3f& normal,
                                VoxelData::VoxelResolution resolution,
                                bool checkExistence = true);
    
    // Configuration
    void setSelectionMode(SelectionMode mode) { m_selectionMode = mode; }
    SelectionMode getSelectionMode() const { return m_selectionMode; }
    
    void setIncludePartial(bool include) { m_includePartial = include; }
    bool getIncludePartial() const { return m_includePartial; }
    
    void setFalloff(bool enabled, float start = 0.8f) { 
        m_useFalloff = enabled; 
        m_falloffStart = start;
    }
    bool getFalloff() const { return m_useFalloff; }
    float getFalloffStart() const { return m_falloffStart; }
    
private:
    VoxelData::VoxelDataManager* m_voxelManager;
    SelectionMode m_selectionMode;
    bool m_includePartial;
    bool m_useFalloff;
    float m_falloffStart;
    
    // Helper methods
    bool isVoxelInSphere(const VoxelId& voxel, const Math::Vector3f& center, float radius) const;
    bool isVoxelInEllipsoid(const VoxelId& voxel, const Math::Vector3f& center, 
                           const Math::Vector3f& radii, const Math::Quaternion& rotation) const;
    bool isVoxelInHemisphere(const VoxelId& voxel, const Math::Vector3f& center, 
                            float radius, const Math::Vector3f& normal) const;
    float getVoxelWeight(const VoxelId& voxel, const Math::Vector3f& center, float radius) const;
    bool voxelExists(const VoxelId& voxel) const;
};

}
}