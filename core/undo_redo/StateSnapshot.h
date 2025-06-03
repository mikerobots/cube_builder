#pragma once

#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include "../voxel_data/VoxelTypes.h"

namespace VoxelEditor {

// Forward declarations
namespace VoxelData {
    class VoxelDataManager;
}
namespace Selection {
    class SelectionManager;
    class SelectionSet;
}
namespace Camera {
    class OrbitCamera;
}
namespace Rendering {
    struct RenderSettings;
}

namespace UndoRedo {

// Snapshot of the entire application state
class StateSnapshot {
public:
    struct VoxelDataSnapshot {
        std::vector<uint8_t> compressedData;
        size_t uncompressedSize;
        VoxelData::VoxelResolution activeResolution;
    };
    
    struct SelectionSnapshot {
        std::string id;
        std::unique_ptr<Selection::SelectionSet> selection;
    };
    
    struct CameraSnapshot {
        // Camera position, rotation, etc.
        float position[3];
        float rotation[4]; // Quaternion
        float fov;
        float nearPlane;
        float farPlane;
    };
    
    struct RenderSnapshot {
        bool showGrid;
        bool showAxes;
        bool showBoundingBoxes;
        bool enableShadows;
        bool enableAmbientOcclusion;
        float ambientIntensity;
    };
    
    StateSnapshot();
    ~StateSnapshot();
    
    // Capture current state
    bool captureVoxelData(const VoxelData::VoxelDataManager* voxelManager);
    bool captureSelections(const Selection::SelectionManager* selectionManager);
    bool captureCamera(const Camera::OrbitCamera* camera);
    bool captureRenderSettings(const Rendering::RenderSettings* renderSettings);
    
    // Restore state
    bool restoreVoxelData(VoxelData::VoxelDataManager* voxelManager) const;
    bool restoreSelections(Selection::SelectionManager* selectionManager) const;
    bool restoreCamera(Camera::OrbitCamera* camera) const;
    bool restoreRenderSettings(Rendering::RenderSettings* renderSettings) const;
    
    // Full state operations
    bool captureFullState(const VoxelData::VoxelDataManager* voxelManager,
                         const Selection::SelectionManager* selectionManager,
                         const Camera::OrbitCamera* camera,
                         const Rendering::RenderSettings* renderSettings);
    
    bool restoreFullState(VoxelData::VoxelDataManager* voxelManager,
                         Selection::SelectionManager* selectionManager,
                         Camera::OrbitCamera* camera,
                         Rendering::RenderSettings* renderSettings) const;
    
    // Metadata
    void setDescription(const std::string& description) { m_description = description; }
    const std::string& getDescription() const { return m_description; }
    
    std::chrono::system_clock::time_point getTimestamp() const { return m_timestamp; }
    
    // Memory management
    size_t getMemoryUsage() const;
    void compress();
    void decompress();
    bool isCompressed() const { return m_compressed; }
    
    // Serialization
    bool saveToFile(const std::string& filepath) const;
    bool loadFromFile(const std::string& filepath);
    
private:
    // State data
    std::unique_ptr<VoxelDataSnapshot> m_voxelData;
    std::vector<SelectionSnapshot> m_selections;
    std::unique_ptr<CameraSnapshot> m_camera;
    std::unique_ptr<RenderSnapshot> m_renderSettings;
    
    // Metadata
    std::string m_description;
    std::chrono::system_clock::time_point m_timestamp;
    bool m_compressed;
    
    // Helper methods
    void compressVoxelData();
    void decompressVoxelData();
};

// Factory for creating state snapshots
class StateSnapshotFactory {
public:
    static std::unique_ptr<StateSnapshot> createFullSnapshot(
        const VoxelData::VoxelDataManager* voxelManager,
        const Selection::SelectionManager* selectionManager,
        const Camera::OrbitCamera* camera,
        const Rendering::RenderSettings* renderSettings,
        const std::string& description = "");
    
    static std::unique_ptr<StateSnapshot> createVoxelSnapshot(
        const VoxelData::VoxelDataManager* voxelManager,
        const std::string& description = "");
    
    static std::unique_ptr<StateSnapshot> createSelectionSnapshot(
        const Selection::SelectionManager* selectionManager,
        const std::string& description = "");
};

}
}