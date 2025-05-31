#include "StateSnapshot.h"
#include "../voxel_data/VoxelDataManager.h"
#include "../selection/SelectionManager.h"
#include "../selection/SelectionSet.h"
#include "../../foundation/logging/Logger.h"
#include <fstream>
#include <cstring>

namespace VoxelEditor {
namespace UndoRedo {

StateSnapshot::StateSnapshot()
    : m_timestamp(std::chrono::system_clock::now())
    , m_compressed(false) {
}

StateSnapshot::~StateSnapshot() = default;

bool StateSnapshot::captureVoxelData(const VoxelData::VoxelDataManager* voxelManager) {
    if (!voxelManager) {
        Logging::Logger::getInstance().error("StateSnapshot: Cannot capture null VoxelDataManager");
        return false;
    }
    
    m_voxelData = std::make_unique<VoxelDataSnapshot>();
    
    // TODO: Implement actual voxel data capture
    // For now, just mark as captured
    m_voxelData->uncompressedSize = 0;
    
    return true;
}

bool StateSnapshot::captureSelections(const Selection::SelectionManager* selectionManager) {
    if (!selectionManager) {
        Logging::Logger::getInstance().error("StateSnapshot: Cannot capture null SelectionManager");
        return false;
    }
    
    m_selections.clear();
    
    // Capture current selection
    SelectionSnapshot snapshot;
    snapshot.id = "current";
    snapshot.selection = std::make_unique<Selection::SelectionSet>(selectionManager->getSelectionCopy());
    m_selections.push_back(std::move(snapshot));
    
    // Capture all named selection sets
    auto setNames = selectionManager->getSelectionSetNames();
    for (const auto& name : setNames) {
        SelectionSnapshot setSnapshot;
        setSnapshot.id = name;
        // We can't directly get the content of a named set without loading it
        // So we'll just mark it as existing
        setSnapshot.selection = nullptr; // Will be restored by name
        m_selections.push_back(std::move(setSnapshot));
    }
    
    return true;
}

bool StateSnapshot::captureCamera(const Camera::CameraState* cameraState) {
    if (!cameraState) {
        Logging::Logger::getInstance().error("StateSnapshot: Cannot capture null CameraState");
        return false;
    }
    
    m_camera = std::make_unique<CameraSnapshot>();
    
    // TODO: Implement actual camera state capture
    // For now, just use default values
    m_camera->position[0] = 0.0f;
    m_camera->position[1] = 0.0f;
    m_camera->position[2] = 10.0f;
    m_camera->rotation[0] = 0.0f;
    m_camera->rotation[1] = 0.0f;
    m_camera->rotation[2] = 0.0f;
    m_camera->rotation[3] = 1.0f;
    m_camera->fov = 60.0f;
    m_camera->nearPlane = 0.1f;
    m_camera->farPlane = 1000.0f;
    
    return true;
}

bool StateSnapshot::captureRenderSettings(const Rendering::RenderSettings* renderSettings) {
    if (!renderSettings) {
        Logging::Logger::getInstance().error("StateSnapshot: Cannot capture null RenderSettings");
        return false;
    }
    
    m_renderSettings = std::make_unique<RenderSnapshot>();
    
    // TODO: Implement actual render settings capture
    // For now, just use default values
    m_renderSettings->showGrid = true;
    m_renderSettings->showAxes = true;
    m_renderSettings->showBoundingBoxes = false;
    m_renderSettings->enableShadows = true;
    m_renderSettings->enableAmbientOcclusion = true;
    m_renderSettings->ambientIntensity = 0.2f;
    
    return true;
}

bool StateSnapshot::restoreVoxelData(VoxelData::VoxelDataManager* voxelManager) const {
    if (!voxelManager || !m_voxelData) {
        return false;
    }
    
    if (m_compressed) {
        // Need to decompress first
        const_cast<StateSnapshot*>(this)->decompress();
    }
    
    // TODO: Implement actual voxel data restoration
    
    return true;
}

bool StateSnapshot::restoreSelections(Selection::SelectionManager* selectionManager) const {
    if (!selectionManager) {
        return false;
    }
    
    bool allSuccessful = true;
    
    // Restore selections
    for (const auto& snapshot : m_selections) {
        if (snapshot.id == "current") {
            // Restore current selection
            if (snapshot.selection) {
                selectionManager->select(*snapshot.selection, Selection::SelectionMode::Replace);
            } else {
                selectionManager->selectNone();
            }
        } else {
            // Named selection sets are just marked as existing
            // We can't restore their content from here
        }
    }
    
    return allSuccessful;
}

bool StateSnapshot::restoreCamera(Camera::CameraState* cameraState) const {
    if (!cameraState || !m_camera) {
        return false;
    }
    
    // TODO: Implement actual camera state restoration
    
    return true;
}

bool StateSnapshot::restoreRenderSettings(Rendering::RenderSettings* renderSettings) const {
    if (!renderSettings || !m_renderSettings) {
        return false;
    }
    
    // TODO: Implement actual render settings restoration
    
    return true;
}

bool StateSnapshot::captureFullState(const VoxelData::VoxelDataManager* voxelManager,
                                   const Selection::SelectionManager* selectionManager,
                                   const Camera::CameraState* cameraState,
                                   const Rendering::RenderSettings* renderSettings) {
    bool success = true;
    
    if (voxelManager) {
        success &= captureVoxelData(voxelManager);
    }
    
    if (selectionManager) {
        success &= captureSelections(selectionManager);
    }
    
    if (cameraState) {
        success &= captureCamera(cameraState);
    }
    
    if (renderSettings) {
        success &= captureRenderSettings(renderSettings);
    }
    
    return success;
}

bool StateSnapshot::restoreFullState(VoxelData::VoxelDataManager* voxelManager,
                                   Selection::SelectionManager* selectionManager,
                                   Camera::CameraState* cameraState,
                                   Rendering::RenderSettings* renderSettings) const {
    bool success = true;
    
    if (voxelManager && m_voxelData) {
        success &= restoreVoxelData(voxelManager);
    }
    
    if (selectionManager) {
        success &= restoreSelections(selectionManager);
    }
    
    if (cameraState && m_camera) {
        success &= restoreCamera(cameraState);
    }
    
    if (renderSettings && m_renderSettings) {
        success &= restoreRenderSettings(renderSettings);
    }
    
    return success;
}

size_t StateSnapshot::getMemoryUsage() const {
    size_t size = sizeof(*this);
    size += m_description.capacity();
    
    if (m_voxelData) {
        size += sizeof(VoxelDataSnapshot);
        size += m_voxelData->compressedData.capacity();
    }
    
    for (const auto& snapshot : m_selections) {
        size += snapshot.id.capacity();
        if (snapshot.selection) {
            size += snapshot.selection->size() * sizeof(Selection::VoxelId);
        }
    }
    
    if (m_camera) {
        size += sizeof(CameraSnapshot);
    }
    
    if (m_renderSettings) {
        size += sizeof(RenderSnapshot);
    }
    
    return size;
}

void StateSnapshot::compress() {
    if (m_compressed) {
        return;
    }
    
    compressVoxelData();
    m_compressed = true;
}

void StateSnapshot::decompress() {
    if (!m_compressed) {
        return;
    }
    
    decompressVoxelData();
    m_compressed = false;
}

void StateSnapshot::compressVoxelData() {
    if (!m_voxelData) {
        return;
    }
    
    // TODO: Implement actual compression
    // For now, just clear uncompressed data
}

void StateSnapshot::decompressVoxelData() {
    if (!m_voxelData) {
        return;
    }
    
    // TODO: Implement actual decompression
}

bool StateSnapshot::saveToFile(const std::string& filepath) const {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        Logging::Logger::getInstance().error("StateSnapshot: Failed to open file for writing: " + filepath);
        return false;
    }
    
    // TODO: Implement actual serialization
    
    return true;
}

bool StateSnapshot::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        Logging::Logger::getInstance().error("StateSnapshot: Failed to open file for reading: " + filepath);
        return false;
    }
    
    // TODO: Implement actual deserialization
    
    return true;
}

// StateSnapshotFactory implementation
std::unique_ptr<StateSnapshot> StateSnapshotFactory::createFullSnapshot(
    const VoxelData::VoxelDataManager* voxelManager,
    const Selection::SelectionManager* selectionManager,
    const Camera::CameraState* cameraState,
    const Rendering::RenderSettings* renderSettings,
    const std::string& description) {
    
    auto snapshot = std::make_unique<StateSnapshot>();
    snapshot->setDescription(description);
    
    bool success = snapshot->captureFullState(voxelManager, selectionManager, 
                                            cameraState, renderSettings);
    
    if (!success) {
        Logging::Logger::getInstance().error("StateSnapshotFactory: Failed to create full snapshot");
        return nullptr;
    }
    
    return snapshot;
}

std::unique_ptr<StateSnapshot> StateSnapshotFactory::createVoxelSnapshot(
    const VoxelData::VoxelDataManager* voxelManager,
    const std::string& description) {
    
    auto snapshot = std::make_unique<StateSnapshot>();
    snapshot->setDescription(description);
    
    bool success = snapshot->captureVoxelData(voxelManager);
    
    if (!success) {
        Logging::Logger::getInstance().error("StateSnapshotFactory: Failed to create voxel snapshot");
        return nullptr;
    }
    
    return snapshot;
}

std::unique_ptr<StateSnapshot> StateSnapshotFactory::createSelectionSnapshot(
    const Selection::SelectionManager* selectionManager,
    const std::string& description) {
    
    auto snapshot = std::make_unique<StateSnapshot>();
    snapshot->setDescription(description);
    
    bool success = snapshot->captureSelections(selectionManager);
    
    if (!success) {
        Logging::Logger::getInstance().error("StateSnapshotFactory: Failed to create selection snapshot");
        return nullptr;
    }
    
    return snapshot;
}

}
}