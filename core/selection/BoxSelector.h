#pragma once

#include "SelectionTypes.h"
#include "SelectionSet.h"
#include "../../foundation/math/Ray.h"
#include "../../foundation/math/Vector4f.h"
#include "../voxel_data/VoxelDataManager.h"

namespace VoxelEditor {
namespace Selection {

class BoxSelector {
public:
    explicit BoxSelector(VoxelData::VoxelDataManager* voxelManager = nullptr);
    ~BoxSelector() = default;
    
    // Set the voxel manager
    void setVoxelManager(VoxelData::VoxelDataManager* manager) { m_voxelManager = manager; }
    
    // Box selection from screen coordinates
    SelectionSet selectFromScreen(const Math::Vector2i& screenStart, 
                                const Math::Vector2i& screenEnd,
                                const Math::Matrix4f& viewMatrix,
                                const Math::Matrix4f& projMatrix,
                                const Math::Vector2i& viewportSize,
                                VoxelData::VoxelResolution resolution);
    
    // Box selection from world coordinates
    SelectionSet selectFromWorld(const Math::BoundingBox& worldBox,
                               VoxelData::VoxelResolution resolution,
                               bool checkExistence = true);
    
    // Box selection from two rays (corner to corner)
    SelectionSet selectFromRays(const Math::Ray& startRay,
                              const Math::Ray& endRay,
                              float maxDistance,
                              VoxelData::VoxelResolution resolution);
    
    // Box selection from grid coordinates
    SelectionSet selectFromGrid(const Math::Vector3i& minGrid,
                              const Math::Vector3i& maxGrid,
                              VoxelData::VoxelResolution resolution,
                              bool checkExistence = true);
    
    // Configuration
    void setSelectionMode(SelectionMode mode) { m_selectionMode = mode; }
    SelectionMode getSelectionMode() const { return m_selectionMode; }
    
    void setIncludePartial(bool include) { m_includePartial = include; }
    bool getIncludePartial() const { return m_includePartial; }
    
private:
    VoxelData::VoxelDataManager* m_voxelManager;
    SelectionMode m_selectionMode;
    bool m_includePartial; // Include voxels partially inside box
    
    // Helper methods
    Math::BoundingBox computeScreenBox(const Math::Vector2i& screenStart,
                                      const Math::Vector2i& screenEnd,
                                      const Math::Matrix4f& viewMatrix,
                                      const Math::Matrix4f& projMatrix,
                                      const Math::Vector2i& viewportSize);
    
    bool isVoxelInBox(const VoxelId& voxel, const Math::BoundingBox& box) const;
    bool voxelExists(const VoxelId& voxel) const;
};

}
}