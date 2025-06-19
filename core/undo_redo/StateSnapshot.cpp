#include "StateSnapshot.h"
#include "../voxel_data/VoxelDataManager.h"
#include "../selection/SelectionManager.h"
#include "../selection/SelectionSet.h"
#include "../camera/OrbitCamera.h"
#include "../rendering/RenderConfig.h"
#include "../../foundation/logging/Logger.h"
#include "../../foundation/math/Vector3i.h"
#include <fstream>
#include <cstring>
#include <sstream>

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
    
    // Capture active resolution
    m_voxelData->activeResolution = voxelManager->getActiveResolution();
    
    // Capture voxel data for all resolution levels
    std::ostringstream dataStream;
    size_t totalVoxels = 0;
    
    for (int i = 0; i < static_cast<int>(VoxelData::VoxelResolution::COUNT); ++i) {
        VoxelData::VoxelResolution resolution = static_cast<VoxelData::VoxelResolution>(i);
        const VoxelData::VoxelGrid* grid = voxelManager->getGrid(resolution);
        
        if (grid) {
            auto voxels = grid->getAllVoxels();
            totalVoxels += voxels.size();
            
            // Write resolution level and voxel count
            uint8_t res = static_cast<uint8_t>(resolution);
            uint32_t count = static_cast<uint32_t>(voxels.size());
            dataStream.write(reinterpret_cast<const char*>(&res), sizeof(res));
            dataStream.write(reinterpret_cast<const char*>(&count), sizeof(count));
            
            // Write voxel positions
            for (const auto& voxel : voxels) {
                int32_t x = voxel.incrementPos.x();
                int32_t y = voxel.incrementPos.y();
                int32_t z = voxel.incrementPos.z();
                dataStream.write(reinterpret_cast<const char*>(&x), sizeof(x));
                dataStream.write(reinterpret_cast<const char*>(&y), sizeof(y));
                dataStream.write(reinterpret_cast<const char*>(&z), sizeof(z));
            }
        } else {
            // Write empty grid
            uint8_t res = static_cast<uint8_t>(resolution);
            uint32_t count = 0;
            dataStream.write(reinterpret_cast<const char*>(&res), sizeof(res));
            dataStream.write(reinterpret_cast<const char*>(&count), sizeof(count));
        }
    }
    
    // Convert stream to vector
    std::string data = dataStream.str();
    m_voxelData->compressedData.assign(data.begin(), data.end());
    m_voxelData->uncompressedSize = m_voxelData->compressedData.size();
    
    Logging::Logger::getInstance().info("StateSnapshot: Captured " + std::to_string(totalVoxels) + " voxels");
    
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

bool StateSnapshot::captureCamera(const Camera::OrbitCamera* camera) {
    if (!camera) {
        Logging::Logger::getInstance().error("StateSnapshot: Cannot capture null OrbitCamera");
        return false;
    }
    
    m_camera = std::make_unique<CameraSnapshot>();
    
    // Capture camera position
    auto position = camera->getPosition();
    m_camera->position[0] = position.x();
    m_camera->position[1] = position.y();
    m_camera->position[2] = position.z();
    
    // For OrbitCamera, we don't have a quaternion rotation directly
    // Instead we store the orbit parameters (yaw, pitch, distance)
    // We'll use the rotation array to store these values
    m_camera->rotation[0] = camera->getYaw();
    m_camera->rotation[1] = camera->getPitch();
    m_camera->rotation[2] = camera->getDistance();
    m_camera->rotation[3] = 1.0f; // Flag to indicate orbit camera data
    
    // Capture projection parameters
    m_camera->fov = camera->getFieldOfView();
    m_camera->nearPlane = camera->getNearPlane();
    m_camera->farPlane = camera->getFarPlane();
    
    return true;
}

bool StateSnapshot::captureRenderSettings(const Rendering::RenderSettings* renderSettings) {
    if (!renderSettings) {
        Logging::Logger::getInstance().error("StateSnapshot: Cannot capture null RenderSettings");
        return false;
    }
    
    m_renderSettings = std::make_unique<RenderSnapshot>();
    
    // Capture render settings
    m_renderSettings->showGrid = false; // Default values for now
    m_renderSettings->showAxes = false;
    m_renderSettings->showBoundingBoxes = renderSettings->showBounds;
    m_renderSettings->enableShadows = renderSettings->enableShadows;
    m_renderSettings->enableAmbientOcclusion = false; // Not available in RenderSettings
    m_renderSettings->ambientIntensity = renderSettings->lightIntensity;
    
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
    
    // Clear existing voxel data
    voxelManager->clearAll();
    
    // Restore active resolution
    voxelManager->setActiveResolution(m_voxelData->activeResolution);
    
    // Parse the stored data
    std::istringstream dataStream(std::string(m_voxelData->compressedData.begin(), 
                                             m_voxelData->compressedData.end()));
    
    // Read voxel data for each resolution level
    for (int i = 0; i < static_cast<int>(VoxelData::VoxelResolution::COUNT); ++i) {
        uint8_t res;
        uint32_t count;
        
        dataStream.read(reinterpret_cast<char*>(&res), sizeof(res));
        dataStream.read(reinterpret_cast<char*>(&count), sizeof(count));
        
        VoxelData::VoxelResolution resolution = static_cast<VoxelData::VoxelResolution>(res);
        
        // Read and restore voxel positions
        for (uint32_t j = 0; j < count; ++j) {
            int32_t x, y, z;
            dataStream.read(reinterpret_cast<char*>(&x), sizeof(x));
            dataStream.read(reinterpret_cast<char*>(&y), sizeof(y));
            dataStream.read(reinterpret_cast<char*>(&z), sizeof(z));
            
            Math::Vector3i pos(x, y, z);
            voxelManager->setVoxel(pos, resolution, true);
        }
    }
    
    Logging::Logger::getInstance().info("StateSnapshot: Restored voxel data");
    
    return dataStream.good();
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

bool StateSnapshot::restoreCamera(Camera::OrbitCamera* camera) const {
    if (!camera || !m_camera) {
        return false;
    }
    
    // Check if this is orbit camera data (rotation[3] == 1.0f)
    if (m_camera->rotation[3] == 1.0f) {
        // Restore orbit camera parameters
        camera->setYaw(m_camera->rotation[0]);
        camera->setPitch(m_camera->rotation[1]);
        camera->setDistance(m_camera->rotation[2]);
    }
    
    // Restore projection parameters
    camera->setFieldOfView(m_camera->fov);
    camera->setNearFarPlanes(m_camera->nearPlane, m_camera->farPlane);
    
    return true;
}

bool StateSnapshot::restoreRenderSettings(Rendering::RenderSettings* renderSettings) const {
    if (!renderSettings || !m_renderSettings) {
        return false;
    }
    
    // Restore render settings
    // Note: simplified restore for available fields
    renderSettings->showBounds = m_renderSettings->showBoundingBoxes;
    renderSettings->enableShadows = m_renderSettings->enableShadows;
    renderSettings->lightIntensity = m_renderSettings->ambientIntensity;
    
    return true;
}

bool StateSnapshot::captureFullState(const VoxelData::VoxelDataManager* voxelManager,
                                   const Selection::SelectionManager* selectionManager,
                                   const Camera::OrbitCamera* camera,
                                   const Rendering::RenderSettings* renderSettings) {
    bool success = true;
    
    if (voxelManager) {
        success &= captureVoxelData(voxelManager);
    }
    
    if (selectionManager) {
        success &= captureSelections(selectionManager);
    }
    
    if (camera) {
        success &= captureCamera(camera);
    }
    
    if (renderSettings) {
        success &= captureRenderSettings(renderSettings);
    }
    
    return success;
}

bool StateSnapshot::restoreFullState(VoxelData::VoxelDataManager* voxelManager,
                                   Selection::SelectionManager* selectionManager,
                                   Camera::OrbitCamera* camera,
                                   Rendering::RenderSettings* renderSettings) const {
    bool success = true;
    
    if (voxelManager && m_voxelData) {
        success &= restoreVoxelData(voxelManager);
    }
    
    if (selectionManager) {
        success &= restoreSelections(selectionManager);
    }
    
    if (camera && m_camera) {
        success &= restoreCamera(camera);
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
    if (!m_voxelData || m_voxelData->compressedData.empty()) {
        return;
    }
    
    // Simple RLE compression for voxel data
    std::vector<uint8_t> compressed;
    compressed.reserve(m_voxelData->compressedData.size() / 2); // Estimate
    
    size_t i = 0;
    while (i < m_voxelData->compressedData.size()) {
        uint8_t value = m_voxelData->compressedData[i];
        uint8_t count = 1;
        
        // Count consecutive identical bytes (up to 255)
        while (i + count < m_voxelData->compressedData.size() && 
               count < 255 && 
               m_voxelData->compressedData[i + count] == value) {
            count++;
        }
        
        // Write count and value
        compressed.push_back(count);
        compressed.push_back(value);
        
        i += count;
    }
    
    // Replace with compressed data if smaller
    if (compressed.size() < m_voxelData->compressedData.size()) {
        m_voxelData->compressedData = std::move(compressed);
        Logging::Logger::getInstance().info("StateSnapshot: Compressed voxel data from " + 
                                          std::to_string(m_voxelData->uncompressedSize) + 
                                          " to " + std::to_string(m_voxelData->compressedData.size()) + " bytes");
    }
}

void StateSnapshot::decompressVoxelData() {
    if (!m_voxelData || m_voxelData->compressedData.empty()) {
        return;
    }
    
    // Simple RLE decompression
    std::vector<uint8_t> decompressed;
    decompressed.reserve(m_voxelData->uncompressedSize);
    
    for (size_t i = 0; i < m_voxelData->compressedData.size(); i += 2) {
        if (i + 1 >= m_voxelData->compressedData.size()) break;
        
        uint8_t count = m_voxelData->compressedData[i];
        uint8_t value = m_voxelData->compressedData[i + 1];
        
        for (uint8_t j = 0; j < count; ++j) {
            decompressed.push_back(value);
        }
    }
    
    m_voxelData->compressedData = std::move(decompressed);
}

bool StateSnapshot::saveToFile(const std::string& filepath) const {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        Logging::Logger::getInstance().error("StateSnapshot: Failed to open file for writing: " + filepath);
        return false;
    }
    
    // Write snapshot header
    const char* magic = "SNAP";
    file.write(magic, 4);
    
    // Write version
    uint32_t version = 1;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    
    // Write timestamp
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(m_timestamp.time_since_epoch()).count();
    file.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
    
    // Write description
    uint32_t descLen = static_cast<uint32_t>(m_description.length());
    file.write(reinterpret_cast<const char*>(&descLen), sizeof(descLen));
    file.write(m_description.c_str(), descLen);
    
    // Write flags for what data is present
    uint8_t flags = 0;
    if (m_voxelData) flags |= 0x01;
    if (!m_selections.empty()) flags |= 0x02;
    if (m_camera) flags |= 0x04;
    if (m_renderSettings) flags |= 0x08;
    file.write(reinterpret_cast<const char*>(&flags), sizeof(flags));
    
    // Write voxel data if present
    if (m_voxelData) {
        uint32_t dataSize = static_cast<uint32_t>(m_voxelData->compressedData.size());
        file.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
        file.write(reinterpret_cast<const char*>(m_voxelData->compressedData.data()), dataSize);
        file.write(reinterpret_cast<const char*>(&m_voxelData->uncompressedSize), sizeof(m_voxelData->uncompressedSize));
        uint8_t res = static_cast<uint8_t>(m_voxelData->activeResolution);
        file.write(reinterpret_cast<const char*>(&res), sizeof(res));
    }
    
    // Write other data sections similarly...
    
    return file.good();
}

bool StateSnapshot::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        Logging::Logger::getInstance().error("StateSnapshot: Failed to open file for reading: " + filepath);
        return false;
    }
    
    // Read and verify header
    char magic[4];
    file.read(magic, 4);
    if (std::memcmp(magic, "SNAP", 4) != 0) {
        Logging::Logger::getInstance().error("StateSnapshot: Invalid file format");
        return false;
    }
    
    // Read version
    uint32_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != 1) {
        Logging::Logger::getInstance().error("StateSnapshot: Unsupported version");
        return false;
    }
    
    // Read timestamp
    int64_t timestamp;
    file.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
    m_timestamp = std::chrono::system_clock::time_point(std::chrono::seconds(timestamp));
    
    // Read description
    uint32_t descLen;
    file.read(reinterpret_cast<char*>(&descLen), sizeof(descLen));
    m_description.resize(descLen);
    file.read(&m_description[0], descLen);
    
    // Read flags
    uint8_t flags;
    file.read(reinterpret_cast<char*>(&flags), sizeof(flags));
    
    // Read voxel data if present
    if (flags & 0x01) {
        m_voxelData = std::make_unique<VoxelDataSnapshot>();
        uint32_t dataSize;
        file.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
        m_voxelData->compressedData.resize(dataSize);
        file.read(reinterpret_cast<char*>(m_voxelData->compressedData.data()), dataSize);
        file.read(reinterpret_cast<char*>(&m_voxelData->uncompressedSize), sizeof(m_voxelData->uncompressedSize));
        uint8_t res;
        file.read(reinterpret_cast<char*>(&res), sizeof(res));
        m_voxelData->activeResolution = static_cast<VoxelData::VoxelResolution>(res);
    }
    
    // Read other data sections similarly...
    
    return file.good();
}

// StateSnapshotFactory implementation
std::unique_ptr<StateSnapshot> StateSnapshotFactory::createFullSnapshot(
    const VoxelData::VoxelDataManager* voxelManager,
    const Selection::SelectionManager* selectionManager,
    const Camera::OrbitCamera* camera,
    const Rendering::RenderSettings* renderSettings,
    const std::string& description) {
    
    auto snapshot = std::make_unique<StateSnapshot>();
    snapshot->setDescription(description);
    
    bool success = snapshot->captureFullState(voxelManager, selectionManager, 
                                            camera, renderSettings);
    
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