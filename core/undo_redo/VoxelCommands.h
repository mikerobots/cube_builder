#pragma once

#include <vector>
#include <memory>

#include "Command.h"
#include "../voxel_data/VoxelDataManager.h"
#include "../voxel_data/VoxelTypes.h"
#include "../../foundation/math/Vector3i.h"
#include "../../foundation/math/BoundingBox.h"

namespace VoxelEditor {
namespace UndoRedo {

// Single voxel edit command
class VoxelEditCommand : public Command {
public:
    VoxelEditCommand(VoxelData::VoxelDataManager* voxelManager, 
                     const Math::Vector3i& position, 
                     VoxelData::VoxelResolution resolution, 
                     bool newValue);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Edit Voxel"; }
    CommandType getType() const override { return CommandType::VoxelEdit; }
    size_t getMemoryUsage() const override;
    
    bool canMergeWith(const Command& other) const override;
    std::unique_ptr<Command> mergeWith(std::unique_ptr<Command> other) override;
    
private:
    VoxelData::VoxelDataManager* m_voxelManager;
    Math::Vector3i m_position;
    VoxelData::VoxelResolution m_resolution;
    bool m_oldValue;
    bool m_newValue;
};

// Bulk voxel edit command for multiple voxels
class BulkVoxelEditCommand : public Command {
public:
    struct VoxelChange {
        Math::Vector3i position;
        VoxelData::VoxelResolution resolution;
        bool oldValue;
        bool newValue;
        
        VoxelChange(const Math::Vector3i& pos, VoxelData::VoxelResolution res, bool oldVal, bool newVal)
            : position(pos), resolution(res), oldValue(oldVal), newValue(newVal) {}
    };
    
    BulkVoxelEditCommand(VoxelData::VoxelDataManager* voxelManager, 
                         const std::vector<VoxelChange>& changes);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override;
    CommandType getType() const override { return CommandType::VoxelEdit; }
    size_t getMemoryUsage() const override;
    
    void compress() override;
    void decompress() override;
    
    // Add more changes to this command
    void addChange(const VoxelChange& change);
    void addChanges(const std::vector<VoxelChange>& changes);
    
    size_t getChangeCount() const { return m_changes.size(); }
    
private:
    VoxelData::VoxelDataManager* m_voxelManager;
    std::vector<VoxelChange> m_changes;
    bool m_compressed = false;
    std::vector<uint8_t> m_compressedData;
    
    void compressChanges();
    void decompressChanges();
};

// Fill operation command
class VoxelFillCommand : public Command {
public:
    VoxelFillCommand(VoxelData::VoxelDataManager* voxelManager,
                     const Math::BoundingBox& region,
                     VoxelData::VoxelResolution resolution,
                     bool fillValue);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Fill Voxels"; }
    CommandType getType() const override { return CommandType::VoxelEdit; }
    size_t getMemoryUsage() const override;
    
private:
    VoxelData::VoxelDataManager* m_voxelManager;
    Math::BoundingBox m_region;
    VoxelData::VoxelResolution m_resolution;
    bool m_fillValue;
    std::vector<BulkVoxelEditCommand::VoxelChange> m_previousState;
};

// Copy voxels command
class VoxelCopyCommand : public Command {
public:
    VoxelCopyCommand(VoxelData::VoxelDataManager* voxelManager,
                     const std::vector<Math::Vector3i>& sourcePositions,
                     const Math::Vector3i& offset,
                     VoxelData::VoxelResolution resolution);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Copy Voxels"; }
    CommandType getType() const override { return CommandType::VoxelEdit; }
    size_t getMemoryUsage() const override;
    
private:
    VoxelData::VoxelDataManager* m_voxelManager;
    std::vector<Math::Vector3i> m_sourcePositions;
    Math::Vector3i m_offset;
    VoxelData::VoxelResolution m_resolution;
    std::vector<BulkVoxelEditCommand::VoxelChange> m_changes;
};

// Move voxels command
class VoxelMoveCommand : public Command {
public:
    VoxelMoveCommand(VoxelData::VoxelDataManager* voxelManager,
                     const std::vector<Math::Vector3i>& positions,
                     const Math::Vector3i& offset,
                     VoxelData::VoxelResolution resolution);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Move Voxels"; }
    CommandType getType() const override { return CommandType::VoxelEdit; }
    size_t getMemoryUsage() const override;
    
private:
    VoxelData::VoxelDataManager* m_voxelManager;
    std::vector<Math::Vector3i> m_positions;
    Math::Vector3i m_offset;
    VoxelData::VoxelResolution m_resolution;
    std::vector<BulkVoxelEditCommand::VoxelChange> m_sourceChanges;
    std::vector<BulkVoxelEditCommand::VoxelChange> m_destChanges;
};

}
}