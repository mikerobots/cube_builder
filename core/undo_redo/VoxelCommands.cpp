#include "VoxelCommands.h"
#include "../../foundation/logging/Logger.h"
#include <algorithm>
#include <sstream>

namespace VoxelEditor {
namespace UndoRedo {

// VoxelEditCommand implementation
VoxelEditCommand::VoxelEditCommand(VoxelData::VoxelDataManager* voxelManager,
                                   const Math::Vector3i& position,
                                   VoxelData::VoxelResolution resolution,
                                   bool newValue)
    : m_voxelManager(voxelManager)
    , m_position(position)
    , m_resolution(resolution)
    , m_newValue(newValue) {
    
    if (!m_voxelManager) {
        throw std::invalid_argument("VoxelEditCommand: VoxelDataManager cannot be null");
    }
    
    // Capture current state
    m_oldValue = m_voxelManager->getVoxel(m_position, m_resolution);
}

bool VoxelEditCommand::execute() {
    if (m_executed && m_voxelManager->getVoxel(m_position, m_resolution) == m_newValue) {
        // Already in desired state
        return true;
    }
    
    bool success = m_voxelManager->setVoxel(m_position, m_resolution, m_newValue);
    if (success) {
        m_executed = true;
    }
    
    return success;
}

bool VoxelEditCommand::undo() {
    if (!m_executed) {
        return false;
    }
    
    bool success = m_voxelManager->setVoxel(m_position, m_resolution, m_oldValue);
    if (success) {
        m_executed = false;
    }
    
    return success;
}

size_t VoxelEditCommand::getMemoryUsage() const {
    return sizeof(*this);
}

bool VoxelEditCommand::canMergeWith(const Command& other) const {
    if (other.getType() != CommandType::VoxelEdit) {
        return false;
    }
    
    const auto* otherVoxelEdit = dynamic_cast<const VoxelEditCommand*>(&other);
    if (!otherVoxelEdit) {
        return false;
    }
    
    // Can merge if it's the same voxel position and resolution
    return m_position == otherVoxelEdit->m_position && 
           m_resolution == otherVoxelEdit->m_resolution;
}

std::unique_ptr<Command> VoxelEditCommand::mergeWith(std::unique_ptr<Command> other) {
    auto otherVoxelEdit = dynamic_cast<VoxelEditCommand*>(other.get());
    if (!otherVoxelEdit || !canMergeWith(*otherVoxelEdit)) {
        return nullptr;
    }
    
    // Keep the old value from this command and new value from the other
    m_newValue = otherVoxelEdit->m_newValue;
    
    // Release the other command and return this one
    other.release();
    return std::unique_ptr<Command>(this);
}

// BulkVoxelEditCommand implementation
BulkVoxelEditCommand::BulkVoxelEditCommand(VoxelData::VoxelDataManager* voxelManager,
                                           const std::vector<VoxelChange>& changes)
    : m_voxelManager(voxelManager)
    , m_changes(changes) {
    
    if (!m_voxelManager) {
        throw std::invalid_argument("BulkVoxelEditCommand: VoxelDataManager cannot be null");
    }
}

bool BulkVoxelEditCommand::execute() {
    if (m_compressed) {
        decompress();
    }
    
    bool allSuccessful = true;
    
    for (const auto& change : m_changes) {
        bool success = m_voxelManager->setVoxel(change.position, change.resolution, change.newValue);
        if (!success) {
            allSuccessful = false;
            Logging::Logger::getInstance().error("BulkVoxelEditCommand: Failed to set voxel at " +
                                               change.position.toString());
        }
    }
    
    if (allSuccessful) {
        m_executed = true;
    }
    
    return allSuccessful;
}

bool BulkVoxelEditCommand::undo() {
    if (!m_executed) {
        return false;
    }
    
    if (m_compressed) {
        decompress();
    }
    
    bool allSuccessful = true;
    
    // Undo in reverse order
    for (auto it = m_changes.rbegin(); it != m_changes.rend(); ++it) {
        bool success = m_voxelManager->setVoxel(it->position, it->resolution, it->oldValue);
        if (!success) {
            allSuccessful = false;
            Logging::Logger::getInstance().error("BulkVoxelEditCommand: Failed to undo voxel at " +
                                               it->position.toString());
        }
    }
    
    if (allSuccessful) {
        m_executed = false;
    }
    
    return allSuccessful;
}

std::string BulkVoxelEditCommand::getName() const {
    std::stringstream ss;
    ss << "Edit " << m_changes.size() << " Voxels";
    return ss.str();
}

size_t BulkVoxelEditCommand::getMemoryUsage() const {
    size_t size = sizeof(*this);
    
    if (m_compressed) {
        size += m_compressedData.capacity();
    } else {
        size += m_changes.capacity() * sizeof(VoxelChange);
    }
    
    return size;
}

void BulkVoxelEditCommand::compress() {
    if (m_compressed || m_changes.empty()) {
        return;
    }
    
    compressChanges();
    m_compressed = true;
}

void BulkVoxelEditCommand::decompress() {
    if (!m_compressed) {
        return;
    }
    
    decompressChanges();
    m_compressed = false;
}

void BulkVoxelEditCommand::addChange(const VoxelChange& change) {
    if (m_compressed) {
        decompress();
    }
    
    m_changes.push_back(change);
}

void BulkVoxelEditCommand::addChanges(const std::vector<VoxelChange>& changes) {
    if (m_compressed) {
        decompress();
    }
    
    m_changes.insert(m_changes.end(), changes.begin(), changes.end());
}

void BulkVoxelEditCommand::compressChanges() {
    // TODO: Implement actual compression
    // For now, just clear the changes to save memory
    m_changes.clear();
    m_changes.shrink_to_fit();
}

void BulkVoxelEditCommand::decompressChanges() {
    // TODO: Implement actual decompression
    // For now, we can't recover if we've compressed
    if (m_changes.empty() && !m_compressedData.empty()) {
        Logging::Logger::getInstance().warning("BulkVoxelEditCommand: Cannot decompress - compression not implemented");
    }
}

// VoxelFillCommand implementation
VoxelFillCommand::VoxelFillCommand(VoxelData::VoxelDataManager* voxelManager,
                                   const Math::BoundingBox& region,
                                   VoxelData::VoxelResolution resolution,
                                   bool fillValue)
    : m_voxelManager(voxelManager)
    , m_region(region)
    , m_resolution(resolution)
    , m_fillValue(fillValue) {
    
    if (!m_voxelManager) {
        throw std::invalid_argument("VoxelFillCommand: VoxelDataManager cannot be null");
    }
}

bool VoxelFillCommand::execute() {
    // Calculate voxel positions within the region
    float voxelSize = VoxelData::getVoxelSize(m_resolution);
    
    Math::Vector3i minVoxel(
        static_cast<int>(std::floor(m_region.min.x / voxelSize)),
        static_cast<int>(std::floor(m_region.min.y / voxelSize)),
        static_cast<int>(std::floor(m_region.min.z / voxelSize))
    );
    
    Math::Vector3i maxVoxel(
        static_cast<int>(std::ceil(m_region.max.x / voxelSize)),
        static_cast<int>(std::ceil(m_region.max.y / voxelSize)),
        static_cast<int>(std::ceil(m_region.max.z / voxelSize))
    );
    
    // Store previous state and execute fill
    m_previousState.clear();
    m_previousState.reserve((maxVoxel.x - minVoxel.x + 1) * 
                           (maxVoxel.y - minVoxel.y + 1) * 
                           (maxVoxel.z - minVoxel.z + 1));
    
    bool allSuccessful = true;
    
    for (int x = minVoxel.x; x <= maxVoxel.x; ++x) {
        for (int y = minVoxel.y; y <= maxVoxel.y; ++y) {
            for (int z = minVoxel.z; z <= maxVoxel.z; ++z) {
                Math::Vector3i pos(x, y, z);
                bool oldValue = m_voxelManager->getVoxel(pos, m_resolution);
                
                m_previousState.emplace_back(pos, m_resolution, oldValue, m_fillValue);
                
                if (oldValue != m_fillValue) {
                    bool success = m_voxelManager->setVoxel(pos, m_resolution, m_fillValue);
                    if (!success) {
                        allSuccessful = false;
                    }
                }
            }
        }
    }
    
    if (allSuccessful) {
        m_executed = true;
    }
    
    return allSuccessful;
}

bool VoxelFillCommand::undo() {
    if (!m_executed) {
        return false;
    }
    
    bool allSuccessful = true;
    
    // Restore previous state
    for (const auto& change : m_previousState) {
        if (change.oldValue != change.newValue) {
            bool success = m_voxelManager->setVoxel(change.position, change.resolution, change.oldValue);
            if (!success) {
                allSuccessful = false;
            }
        }
    }
    
    if (allSuccessful) {
        m_executed = false;
    }
    
    return allSuccessful;
}

size_t VoxelFillCommand::getMemoryUsage() const {
    return sizeof(*this) + m_previousState.capacity() * sizeof(BulkVoxelEditCommand::VoxelChange);
}

// VoxelCopyCommand implementation
VoxelCopyCommand::VoxelCopyCommand(VoxelData::VoxelDataManager* voxelManager,
                                   const std::vector<Math::Vector3i>& sourcePositions,
                                   const Math::Vector3i& offset,
                                   VoxelData::VoxelResolution resolution)
    : m_voxelManager(voxelManager)
    , m_sourcePositions(sourcePositions)
    , m_offset(offset)
    , m_resolution(resolution) {
    
    if (!m_voxelManager) {
        throw std::invalid_argument("VoxelCopyCommand: VoxelDataManager cannot be null");
    }
}

bool VoxelCopyCommand::execute() {
    m_changes.clear();
    m_changes.reserve(m_sourcePositions.size());
    
    bool allSuccessful = true;
    
    for (const auto& sourcePos : m_sourcePositions) {
        bool sourceValue = m_voxelManager->getVoxel(sourcePos, m_resolution);
        if (sourceValue) {
            Math::Vector3i destPos = sourcePos + m_offset;
            bool oldValue = m_voxelManager->getVoxel(destPos, m_resolution);
            
            m_changes.emplace_back(destPos, m_resolution, oldValue, true);
            
            bool success = m_voxelManager->setVoxel(destPos, m_resolution, true);
            if (!success) {
                allSuccessful = false;
            }
        }
    }
    
    if (allSuccessful) {
        m_executed = true;
    }
    
    return allSuccessful;
}

bool VoxelCopyCommand::undo() {
    if (!m_executed) {
        return false;
    }
    
    bool allSuccessful = true;
    
    // Restore previous state at destination positions
    for (const auto& change : m_changes) {
        bool success = m_voxelManager->setVoxel(change.position, change.resolution, change.oldValue);
        if (!success) {
            allSuccessful = false;
        }
    }
    
    if (allSuccessful) {
        m_executed = false;
    }
    
    return allSuccessful;
}

size_t VoxelCopyCommand::getMemoryUsage() const {
    return sizeof(*this) + 
           m_sourcePositions.capacity() * sizeof(Math::Vector3i) +
           m_changes.capacity() * sizeof(BulkVoxelEditCommand::VoxelChange);
}

// VoxelMoveCommand implementation
VoxelMoveCommand::VoxelMoveCommand(VoxelData::VoxelDataManager* voxelManager,
                                   const std::vector<Math::Vector3i>& positions,
                                   const Math::Vector3i& offset,
                                   VoxelData::VoxelResolution resolution)
    : m_voxelManager(voxelManager)
    , m_positions(positions)
    , m_offset(offset)
    , m_resolution(resolution) {
    
    if (!m_voxelManager) {
        throw std::invalid_argument("VoxelMoveCommand: VoxelDataManager cannot be null");
    }
}

bool VoxelMoveCommand::execute() {
    m_sourceChanges.clear();
    m_destChanges.clear();
    m_sourceChanges.reserve(m_positions.size());
    m_destChanges.reserve(m_positions.size());
    
    // First, gather all source voxels and destination states
    for (const auto& sourcePos : m_positions) {
        bool sourceValue = m_voxelManager->getVoxel(sourcePos, m_resolution);
        if (sourceValue) {
            Math::Vector3i destPos = sourcePos + m_offset;
            bool destOldValue = m_voxelManager->getVoxel(destPos, m_resolution);
            
            m_sourceChanges.emplace_back(sourcePos, m_resolution, true, false);
            m_destChanges.emplace_back(destPos, m_resolution, destOldValue, true);
        }
    }
    
    bool allSuccessful = true;
    
    // Clear source positions
    for (const auto& change : m_sourceChanges) {
        bool success = m_voxelManager->setVoxel(change.position, change.resolution, false);
        if (!success) {
            allSuccessful = false;
        }
    }
    
    // Set destination positions
    for (const auto& change : m_destChanges) {
        bool success = m_voxelManager->setVoxel(change.position, change.resolution, true);
        if (!success) {
            allSuccessful = false;
        }
    }
    
    if (allSuccessful) {
        m_executed = true;
    }
    
    return allSuccessful;
}

bool VoxelMoveCommand::undo() {
    if (!m_executed) {
        return false;
    }
    
    bool allSuccessful = true;
    
    // Restore destination positions first
    for (const auto& change : m_destChanges) {
        bool success = m_voxelManager->setVoxel(change.position, change.resolution, change.oldValue);
        if (!success) {
            allSuccessful = false;
        }
    }
    
    // Restore source positions
    for (const auto& change : m_sourceChanges) {
        bool success = m_voxelManager->setVoxel(change.position, change.resolution, change.oldValue);
        if (!success) {
            allSuccessful = false;
        }
    }
    
    if (allSuccessful) {
        m_executed = false;
    }
    
    return allSuccessful;
}

size_t VoxelMoveCommand::getMemoryUsage() const {
    return sizeof(*this) + 
           m_positions.capacity() * sizeof(Math::Vector3i) +
           m_sourceChanges.capacity() * sizeof(BulkVoxelEditCommand::VoxelChange) +
           m_destChanges.capacity() * sizeof(BulkVoxelEditCommand::VoxelChange);
}

}
}