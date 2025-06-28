#include "PlacementCommands.h"
#include "../../foundation/logging/Logger.h"
#include "../input/PlacementValidation.h"
#include "../../foundation/math/CoordinateConverter.h"
#include "../voxel_data/VoxelTypes.h"
#include <sstream>
#include <cmath>

namespace VoxelEditor {
namespace UndoRedo {

using namespace VoxelData;
using namespace Math;
using namespace Logging;

// PlacementCommandFactory implementation
std::unique_ptr<Command> PlacementCommandFactory::createPlacementCommand(
    VoxelDataManager* voxelManager,
    const IncrementCoordinates& position,
    VoxelResolution resolution) {
    
    if (!voxelManager) {
        Logger::getInstance().error("PlacementCommandFactory: VoxelDataManager is null");
        return nullptr;
    }
    
    // Validate the placement
    ValidationResult validation = validatePlacement(voxelManager, position, resolution);
    if (!validation.valid) {
        std::stringstream ss;
        ss << "Invalid placement at " << position.toString()
           << " resolution " << VoxelData::getVoxelSizeName(resolution) << ": "
           << (validation.errors.empty() ? "Unknown error" : validation.errors[0]);
        Logger::getInstance().warning(ss.str());
        return nullptr;
    }
    
    return std::make_unique<VoxelPlacementCommand>(voxelManager, position, resolution);
}

std::unique_ptr<Command> PlacementCommandFactory::createRemovalCommand(
    VoxelDataManager* voxelManager,
    const IncrementCoordinates& position,
    VoxelResolution resolution) {
    
    if (!voxelManager) {
        Logger::getInstance().error("PlacementCommandFactory: VoxelDataManager is null");
        return nullptr;
    }
    
    // Validate the removal
    ValidationResult validation = validateRemoval(voxelManager, position, resolution);
    if (!validation.valid) {
        std::stringstream ss;
        ss << "Invalid removal at " << position.toString()
           << " resolution " << VoxelData::getVoxelSizeName(resolution) << ": "
           << (validation.errors.empty() ? "Unknown error" : validation.errors[0]);
        Logger::getInstance().warning(ss.str());
        return nullptr;
    }
    
    return std::make_unique<VoxelRemovalCommand>(voxelManager, position, resolution);
}

ValidationResult PlacementCommandFactory::validatePlacement(
    VoxelDataManager* voxelManager,
    const IncrementCoordinates& position,
    VoxelResolution resolution) {
    
    ValidationResult result;
    
    if (!voxelManager) {
        result.addError("VoxelDataManager is null");
        return result;
    }
    
    // Use the new comprehensive validation API
    auto validation = voxelManager->validatePosition(position, resolution, true); // Check overlap for placement
    
    if (!validation.valid) {
        result.addError(validation.errorMessage);
        return result;
    }
    
    // Check if voxel already exists at this position
    if (voxelManager->getVoxel(position, resolution)) {
        result.addWarning("Voxel already exists at this position");
    }
    
    return result;
}

ValidationResult PlacementCommandFactory::validateRemoval(
    VoxelDataManager* voxelManager,
    const IncrementCoordinates& position,
    VoxelResolution resolution) {
    
    ValidationResult result;
    
    if (!voxelManager) {
        result.addError("VoxelDataManager is null");
        return result;
    }
    
    // Use the new validation API (no overlap check needed for removal)
    auto validation = voxelManager->validatePosition(position, resolution, false);
    
    if (!validation.valid) {
        result.addError(validation.errorMessage);
        return result;
    }
    
    // Check if voxel exists at this position
    if (!voxelManager->getVoxel(position, resolution)) {
        result.addError("No voxel exists at this position to remove");
    }
    
    return result;
}

// VoxelPlacementCommand implementation
VoxelPlacementCommand::VoxelPlacementCommand(VoxelDataManager* voxelManager,
                                           const IncrementCoordinates& position,
                                           VoxelResolution resolution)
    : m_position(position)
    , m_resolution(resolution)
    , m_voxelManager(voxelManager) {
    
    if (!voxelManager) {
        throw std::invalid_argument("VoxelPlacementCommand: VoxelDataManager cannot be null");
    }
    
    // Create the underlying edit command
    m_baseCommand = std::make_unique<VoxelEditCommand>(voxelManager, position, resolution, true);
}

bool VoxelPlacementCommand::execute() {
    if (!m_baseCommand) {
        Logger::getInstance().error("VoxelPlacementCommand: No base command");
        return false;
    }
    
    // Validate before execution
    ValidationResult validation = validate();
    if (!validation.valid) {
        Logger::getInstance().error("VoxelPlacementCommand execution failed validation");
        for (const auto& error : validation.errors) {
            Logger::getInstance().error("  - " + error);
        }
        return false;
    }
    
    bool success = m_baseCommand->execute();
    
    if (success) {
        m_executed = true;
        std::stringstream ss;
        ss << "Placed voxel at " << m_position.toString()
           << " resolution " << VoxelData::getVoxelSizeName(m_resolution);
        Logger::getInstance().debug(ss.str());
    } else {
        Logger::getInstance().error("VoxelPlacementCommand: Base command execution failed");
    }
    
    return success;
}

bool VoxelPlacementCommand::undo() {
    if (!m_baseCommand || !m_executed) {
        return false;
    }
    
    bool success = m_baseCommand->undo();
    if (success) {
        m_executed = false;
        std::stringstream ss;
        ss << "Undid voxel placement at " << m_position.toString()
           << " resolution " << VoxelData::getVoxelSizeName(m_resolution);
        Logger::getInstance().debug(ss.str());
    }
    
    return success;
}

bool VoxelPlacementCommand::canUndo() const {
    return m_baseCommand && m_baseCommand->canUndo();
}

std::string VoxelPlacementCommand::getDescription() const {
    std::stringstream ss;
    ss << "Place " << VoxelData::getVoxelSizeName(m_resolution) << " voxel at " 
       << m_position.toString();
    return ss.str();
}

size_t VoxelPlacementCommand::getMemoryUsage() const {
    size_t baseSize = sizeof(*this);
    if (m_baseCommand) {
        baseSize += m_baseCommand->getMemoryUsage();
    }
    return baseSize;
}

bool VoxelPlacementCommand::canMergeWith(const Command& other) const {
    if (!m_baseCommand) {
        return false;
    }
    
    // Can merge with other placement commands at the same position
    const auto* otherPlacement = dynamic_cast<const VoxelPlacementCommand*>(&other);
    if (otherPlacement && otherPlacement->m_baseCommand) {
        return m_baseCommand->canMergeWith(*otherPlacement->m_baseCommand);
    }
    
    return false;
}

std::unique_ptr<Command> VoxelPlacementCommand::mergeWith(std::unique_ptr<Command> other) {
    auto otherPlacement = dynamic_cast<VoxelPlacementCommand*>(other.get());
    if (!otherPlacement || !canMergeWith(*otherPlacement)) {
        return nullptr;
    }
    
    // For placement commands at the same position, we just keep the latest one
    // (placing a voxel where one already exists is redundant)
    return std::move(other);
}

ValidationResult VoxelPlacementCommand::validate() const {
    return PlacementCommandFactory::validatePlacement(m_voxelManager, m_position, m_resolution);
}

// VoxelRemovalCommand implementation
VoxelRemovalCommand::VoxelRemovalCommand(VoxelDataManager* voxelManager,
                                        const IncrementCoordinates& position,
                                        VoxelResolution resolution)
    : m_position(position)
    , m_resolution(resolution)
    , m_voxelManager(voxelManager) {
    
    if (!voxelManager) {
        throw std::invalid_argument("VoxelRemovalCommand: VoxelDataManager cannot be null");
    }
    
    // Create the underlying edit command
    m_baseCommand = std::make_unique<VoxelEditCommand>(voxelManager, position, resolution, false);
}

bool VoxelRemovalCommand::execute() {
    if (!m_baseCommand) {
        return false;
    }
    
    // Validate before execution
    ValidationResult validation = validate();
    if (!validation.valid) {
        Logger::getInstance().error("VoxelRemovalCommand execution failed validation");
        return false;
    }
    
    bool success = m_baseCommand->execute();
    if (success) {
        m_executed = true;
        std::stringstream ss;
        ss << "Removed voxel at " << m_position.toString()
           << " resolution " << VoxelData::getVoxelSizeName(m_resolution);
        Logger::getInstance().debug(ss.str());
    }
    
    return success;
}

bool VoxelRemovalCommand::undo() {
    if (!m_baseCommand || !m_executed) {
        return false;
    }
    
    bool success = m_baseCommand->undo();
    if (success) {
        m_executed = false;
        std::stringstream ss;
        ss << "Undid voxel removal at " << m_position.toString()
           << " resolution " << VoxelData::getVoxelSizeName(m_resolution);
        Logger::getInstance().debug(ss.str());
    }
    
    return success;
}

bool VoxelRemovalCommand::canUndo() const {
    return m_baseCommand && m_baseCommand->canUndo();
}

std::string VoxelRemovalCommand::getDescription() const {
    std::stringstream ss;
    ss << "Remove " << VoxelData::getVoxelSizeName(m_resolution) << " voxel at " 
       << m_position.toString();
    return ss.str();
}

size_t VoxelRemovalCommand::getMemoryUsage() const {
    size_t baseSize = sizeof(*this);
    if (m_baseCommand) {
        baseSize += m_baseCommand->getMemoryUsage();
    }
    return baseSize;
}

bool VoxelRemovalCommand::canMergeWith(const Command& other) const {
    if (!m_baseCommand) {
        return false;
    }
    
    // Can merge with other removal commands at the same position
    const auto* otherRemoval = dynamic_cast<const VoxelRemovalCommand*>(&other);
    if (otherRemoval && otherRemoval->m_baseCommand) {
        return m_baseCommand->canMergeWith(*otherRemoval->m_baseCommand);
    }
    
    return false;
}

std::unique_ptr<Command> VoxelRemovalCommand::mergeWith(std::unique_ptr<Command> other) {
    auto otherRemoval = dynamic_cast<VoxelRemovalCommand*>(other.get());
    if (!otherRemoval || !canMergeWith(*otherRemoval)) {
        return nullptr;
    }
    
    // For removal commands at the same position, we just keep the latest one
    // (removing a voxel that's already removed is redundant)
    return std::move(other);
}

ValidationResult VoxelRemovalCommand::validate() const {
    return PlacementCommandFactory::validateRemoval(m_voxelManager, m_position, m_resolution);
}

} // namespace UndoRedo
} // namespace VoxelEditor