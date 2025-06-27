#include "VoxelCommands.h"
#include "../../foundation/logging/Logger.h"
#include "../../foundation/math/CoordinateTypes.h"
#include "../../foundation/math/CoordinateConverter.h"
#include <algorithm>
#include <sstream>
#include <string>

namespace VoxelEditor {
namespace UndoRedo {

// VoxelEditCommand implementation
VoxelEditCommand::VoxelEditCommand(VoxelData::VoxelDataManager* voxelManager,
                                   const Math::IncrementCoordinates& position,
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
    // Convert world bounds to increment coordinates for proper iteration
    Math::IncrementCoordinates minIncrement = Math::CoordinateConverter::worldToIncrement(
        Math::WorldCoordinates(m_region.min)
    );
    Math::IncrementCoordinates maxIncrement = Math::CoordinateConverter::worldToIncrement(
        Math::WorldCoordinates(m_region.max)
    );
    
    Math::Vector3i minInc = minIncrement.value();
    Math::Vector3i maxInc = maxIncrement.value();
    
    Logging::Logger::getInstance().debug("VoxelFillCommand::execute - Region: min=" + 
        m_region.min.toString() + ", max=" + m_region.max.toString());
    Logging::Logger::getInstance().debug("VoxelFillCommand::execute - Increment coords: min=" + 
        minInc.toString() + ", max=" + maxInc.toString());
    
    
    // First, check if all positions can be filled without actually filling them
    // This ensures the operation is atomic - either all succeed or none are changed
    std::vector<BulkVoxelEditCommand::VoxelChange> plannedChanges;
    plannedChanges.reserve((maxInc.x - minInc.x + 1) * 
                          (maxInc.y - minInc.y + 1) * 
                          (maxInc.z - minInc.z + 1));
    
    // Phase 1: Check all positions for validity
    int totalPositions = 0;
    int wouldFail = 0;
    for (int x = minInc.x; x <= maxInc.x; ++x) {
        for (int y = minInc.y; y <= maxInc.y; ++y) {
            for (int z = minInc.z; z <= maxInc.z; ++z) {
                Math::IncrementCoordinates pos(x, y, z);
                totalPositions++;
                bool oldValue = m_voxelManager->getVoxel(pos, m_resolution);
                
                if (oldValue != m_fillValue) {
                    // Check if we can set this voxel
                    if (!m_voxelManager->isValidPosition(pos, m_resolution)) {
                        wouldFail++;
                        Logging::Logger::getInstance().debug("VoxelFillCommand: Position invalid at " + pos.toString());
                    } else if (m_fillValue && m_voxelManager->wouldOverlap(pos, m_resolution)) {
                        wouldFail++;
                        Logging::Logger::getInstance().debug("VoxelFillCommand: Would overlap at " + pos.toString());
                    } else {
                        // This position can be filled
                        plannedChanges.emplace_back(pos, m_resolution, oldValue, m_fillValue);
                    }
                }
            }
        }
    }
    
    // If any positions would fail, abort the entire operation
    if (wouldFail > 0) {
        Logging::Logger::getInstance().debug("VoxelFillCommand: Aborting - " + std::to_string(wouldFail) + 
            " positions would fail out of " + std::to_string(totalPositions));
        return false;
    }
    
    // Phase 2: Execute all changes (we know they will all succeed)
    m_previousState.clear();
    m_previousState.reserve(plannedChanges.size());
    
    bool allSuccessful = true;
    int successCount = 0;
    
    for (const auto& change : plannedChanges) {
        m_previousState.push_back(change);
        bool success = m_voxelManager->setVoxel(change.position, change.resolution, change.newValue);
        if (!success) {
            // This shouldn't happen since we pre-validated
            allSuccessful = false;
            Logging::Logger::getInstance().error("VoxelFillCommand: Unexpected failure at " + change.position.toString());
        } else {
            successCount++;
        }
    }
    
    Logging::Logger::getInstance().debug("VoxelFillCommand: Filled " + std::to_string(successCount) + 
        " voxels out of " + std::to_string(totalPositions) + " positions");
    
    if (allSuccessful) {
        m_executed = true;
    } else {
        // If we somehow failed, undo any changes we made
        for (const auto& change : m_previousState) {
            m_voxelManager->setVoxel(change.position, change.resolution, change.oldValue);
        }
        m_previousState.clear();
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
                                   const std::vector<Math::IncrementCoordinates>& sourcePositions,
                                   const Math::IncrementCoordinates& offset,
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
            Math::IncrementCoordinates destPos(
                sourcePos.x() + m_offset.x(),
                sourcePos.y() + m_offset.y(),
                sourcePos.z() + m_offset.z()
            );
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
           m_sourcePositions.capacity() * sizeof(Math::IncrementCoordinates) +
           m_changes.capacity() * sizeof(BulkVoxelEditCommand::VoxelChange);
}

// VoxelMoveCommand implementation
VoxelMoveCommand::VoxelMoveCommand(VoxelData::VoxelDataManager* voxelManager,
                                   const std::vector<Math::IncrementCoordinates>& positions,
                                   const Math::IncrementCoordinates& offset,
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
            Math::IncrementCoordinates destPos(
                sourcePos.x() + m_offset.x(),
                sourcePos.y() + m_offset.y(),
                sourcePos.z() + m_offset.z()
            );
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
           m_positions.capacity() * sizeof(Math::IncrementCoordinates) +
           m_sourceChanges.capacity() * sizeof(BulkVoxelEditCommand::VoxelChange) +
           m_destChanges.capacity() * sizeof(BulkVoxelEditCommand::VoxelChange);
}

}
}