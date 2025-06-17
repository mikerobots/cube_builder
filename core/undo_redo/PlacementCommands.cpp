#include "PlacementCommands.h"
#include "../../foundation/logging/Logger.h"
#include "../input/PlacementValidation.h"
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
    const Vector3i& position,
    VoxelResolution resolution) {
    
    if (!voxelManager) {
        Logger::getInstance().error("PlacementCommandFactory: VoxelDataManager is null");
        return nullptr;
    }
    
    // Validate the placement
    ValidationResult validation = validatePlacement(voxelManager, position, resolution);
    if (!validation.valid) {
        std::stringstream ss;
        ss << "Invalid placement at (" << position.x << ", " << position.y << ", " << position.z 
           << ") resolution " << static_cast<int>(resolution) << ": "
           << (validation.errors.empty() ? "Unknown error" : validation.errors[0]);
        Logger::getInstance().warning(ss.str());
        return nullptr;
    }
    
    return std::make_unique<VoxelPlacementCommand>(voxelManager, position, resolution);
}

std::unique_ptr<Command> PlacementCommandFactory::createRemovalCommand(
    VoxelDataManager* voxelManager,
    const Vector3i& position,
    VoxelResolution resolution) {
    
    if (!voxelManager) {
        Logger::getInstance().error("PlacementCommandFactory: VoxelDataManager is null");
        return nullptr;
    }
    
    // Validate the removal
    ValidationResult validation = validateRemoval(voxelManager, position, resolution);
    if (!validation.valid) {
        std::stringstream ss;
        ss << "Invalid removal at (" << position.x << ", " << position.y << ", " << position.z 
           << ") resolution " << static_cast<int>(resolution) << ": "
           << (validation.errors.empty() ? "Unknown error" : validation.errors[0]);
        Logger::getInstance().warning(ss.str());
        return nullptr;
    }
    
    return std::make_unique<VoxelRemovalCommand>(voxelManager, position, resolution);
}

ValidationResult PlacementCommandFactory::validatePlacement(
    VoxelDataManager* voxelManager,
    const Vector3i& position,
    VoxelResolution resolution) {
    
    ValidationResult result;
    
    if (!voxelManager) {
        result.addError("VoxelDataManager is null");
        return result;
    }
    
    // Get workspace size for bounds checking
    Math::Vector3f workspaceSize = voxelManager->getWorkspaceSize();
    
    // Use proper placement validation from Input module
    Input::PlacementValidationResult placementResult = 
        Input::PlacementUtils::validatePlacement(position, resolution, workspaceSize);
    
    switch (placementResult) {
        case Input::PlacementValidationResult::InvalidPosition:
            result.addError("Invalid position coordinates");
            break;
        case Input::PlacementValidationResult::InvalidYBelowZero:
            result.addError("Cannot place voxels below ground plane (Y < 0)");
            break;
        case Input::PlacementValidationResult::InvalidOutOfBounds:
            result.addError("Position is outside workspace bounds");
            break;
        case Input::PlacementValidationResult::Valid:
            // Continue with additional checks
            break;
    }
    
    // Check if position is valid for 1cm increments
    if (!voxelManager->isValidIncrementPosition(position)) {
        result.addError("Position is not aligned to 1cm increments");
    }
    
    // Check grid alignment for voxel resolution
    Math::Vector3f worldPos = Input::PlacementUtils::incrementGridToWorld(position);
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    // Validate grid alignment (position must be multiple of voxel size)
    const float EPSILON = 0.001f; // 1mm tolerance
    if (std::fmod(worldPos.x + EPSILON, voxelSize) > 2 * EPSILON ||
        std::fmod(worldPos.y + EPSILON, voxelSize) > 2 * EPSILON ||
        std::fmod(worldPos.z + EPSILON, voxelSize) > 2 * EPSILON) {
        std::stringstream ss;
        ss << "Position (" << worldPos.x << ", " << worldPos.y << ", " << worldPos.z 
           << ") is not aligned to " << static_cast<int>(voxelSize * 100) << "cm grid";
        result.addError(ss.str());
    }
    
    // Check if position would overlap with existing voxel
    if (voxelManager->wouldOverlap(position, resolution)) {
        result.addError("Position would overlap with existing voxel");
    }
    
    // Check if voxel already exists at this position
    if (voxelManager->getVoxel(position, resolution)) {
        result.addWarning("Voxel already exists at this position");
    }
    
    return result;
}

ValidationResult PlacementCommandFactory::validateRemoval(
    VoxelDataManager* voxelManager,
    const Vector3i& position,
    VoxelResolution resolution) {
    
    ValidationResult result;
    
    if (!voxelManager) {
        result.addError("VoxelDataManager is null");
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
                                           const Vector3i& position,
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
        return false;
    }
    
    // Validate before execution
    ValidationResult validation = validate();
    if (!validation.valid) {
        Logger::getInstance().error("VoxelPlacementCommand execution failed validation");
        return false;
    }
    
    bool success = m_baseCommand->execute();
    if (success) {
        m_executed = true;
        std::stringstream ss;
        ss << "Placed voxel at (" << m_position.x << ", " << m_position.y << ", " << m_position.z 
           << ") resolution " << static_cast<int>(m_resolution);
        Logger::getInstance().debug(ss.str());
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
        ss << "Undid voxel placement at (" << m_position.x << ", " << m_position.y << ", " << m_position.z 
           << ") resolution " << static_cast<int>(m_resolution);
        Logger::getInstance().debug(ss.str());
    }
    
    return success;
}

bool VoxelPlacementCommand::canUndo() const {
    return m_baseCommand && m_baseCommand->canUndo();
}

std::string VoxelPlacementCommand::getDescription() const {
    std::stringstream ss;
    ss << "Place " << static_cast<int>(m_resolution) << "cm voxel at (" 
       << m_position.x << ", " << m_position.y << ", " << m_position.z << ")";
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
    
    // Merge the base commands
    auto mergedBase = m_baseCommand->mergeWith(std::move(otherPlacement->m_baseCommand));
    if (mergedBase) {
        m_baseCommand = std::unique_ptr<VoxelEditCommand>(
            dynamic_cast<VoxelEditCommand*>(mergedBase.release()));
        other.release();
        return std::unique_ptr<Command>(this);
    }
    
    return nullptr;
}

ValidationResult VoxelPlacementCommand::validate() const {
    return PlacementCommandFactory::validatePlacement(m_voxelManager, m_position, m_resolution);
}

// VoxelRemovalCommand implementation
VoxelRemovalCommand::VoxelRemovalCommand(VoxelDataManager* voxelManager,
                                        const Vector3i& position,
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
        ss << "Removed voxel at (" << m_position.x << ", " << m_position.y << ", " << m_position.z 
           << ") resolution " << static_cast<int>(m_resolution);
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
        ss << "Undid voxel removal at (" << m_position.x << ", " << m_position.y << ", " << m_position.z 
           << ") resolution " << static_cast<int>(m_resolution);
        Logger::getInstance().debug(ss.str());
    }
    
    return success;
}

bool VoxelRemovalCommand::canUndo() const {
    return m_baseCommand && m_baseCommand->canUndo();
}

std::string VoxelRemovalCommand::getDescription() const {
    std::stringstream ss;
    ss << "Remove " << static_cast<int>(m_resolution) << "cm voxel at (" 
       << m_position.x << ", " << m_position.y << ", " << m_position.z << ")";
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
    
    // Merge the base commands
    auto mergedBase = m_baseCommand->mergeWith(std::move(otherRemoval->m_baseCommand));
    if (mergedBase) {
        m_baseCommand = std::unique_ptr<VoxelEditCommand>(
            dynamic_cast<VoxelEditCommand*>(mergedBase.release()));
        other.release();
        return std::unique_ptr<Command>(this);
    }
    
    return nullptr;
}

ValidationResult VoxelRemovalCommand::validate() const {
    return PlacementCommandFactory::validateRemoval(m_voxelManager, m_position, m_resolution);
}

} // namespace UndoRedo
} // namespace VoxelEditor