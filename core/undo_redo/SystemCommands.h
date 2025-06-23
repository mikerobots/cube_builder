#pragma once

#include "Command.h"
#include "../voxel_data/VoxelDataManager.h"
#include "../voxel_data/VoxelTypes.h"
// Forward declarations to avoid circular dependency  
namespace VoxelEditor {
namespace Camera {
    class CameraController;
    class Camera;
    enum class ViewPreset;
}
}
#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/CoordinateTypes.h"

namespace VoxelEditor {
namespace UndoRedo {

/**
 * @brief Command for changing voxel resolution
 */
class ResolutionChangeCommand : public Command {
public:
    ResolutionChangeCommand(VoxelData::VoxelDataManager* voxelManager,
                           VoxelData::VoxelResolution newResolution);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Change Resolution"; }
    CommandType getType() const override { return CommandType::Workspace; }
    size_t getMemoryUsage() const override;
    
private:
    VoxelData::VoxelDataManager* m_voxelManager;
    VoxelData::VoxelResolution m_oldResolution;
    VoxelData::VoxelResolution m_newResolution;
};

/**
 * @brief Command for changing workspace size
 */
class WorkspaceResizeCommand : public Command {
public:
    WorkspaceResizeCommand(VoxelData::VoxelDataManager* voxelManager,
                          const Math::Vector3f& newSize);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Resize Workspace"; }
    CommandType getType() const override { return CommandType::Workspace; }
    size_t getMemoryUsage() const override;
    
private:
    VoxelData::VoxelDataManager* m_voxelManager;
    Math::Vector3f m_oldSize;
    Math::Vector3f m_newSize;
};

/**
 * @brief Command for camera view changes
 */
class CameraViewCommand : public Command {
public:
    CameraViewCommand(Camera::CameraController* cameraController,
                     const Math::WorldCoordinates& newPosition,
                     const Math::WorldCoordinates& newTarget);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Change Camera View"; }
    CommandType getType() const override { return CommandType::Camera; }
    size_t getMemoryUsage() const override;
    
private:
    Camera::CameraController* m_cameraController;
    Math::WorldCoordinates m_oldPosition;
    Math::WorldCoordinates m_oldTarget;
    Math::WorldCoordinates m_newPosition;
    Math::WorldCoordinates m_newTarget;
    float m_oldDistance;
    float m_newDistance;
};

/**
 * @brief Command for camera view preset changes
 */
class CameraPresetCommand : public Command {
public:
    CameraPresetCommand(Camera::CameraController* cameraController,
                       Camera::ViewPreset newPreset);
    
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Change Camera Preset"; }
    CommandType getType() const override { return CommandType::Camera; }
    size_t getMemoryUsage() const override;
    
private:
    Camera::CameraController* m_cameraController;
    Camera::ViewPreset m_newPreset;
    // Store the old camera state to restore on undo
    Math::WorldCoordinates m_oldPosition;
    Math::WorldCoordinates m_oldTarget;
    float m_oldDistance;
    float m_oldYaw;
    float m_oldPitch;
};

/**
 * @brief Factory for creating system commands
 */
class SystemCommandFactory {
public:
    static std::unique_ptr<Command> createResolutionChangeCommand(
        VoxelData::VoxelDataManager* voxelManager,
        VoxelData::VoxelResolution newResolution);
    
    static std::unique_ptr<Command> createWorkspaceResizeCommand(
        VoxelData::VoxelDataManager* voxelManager,
        const Math::Vector3f& newSize);
    
    static std::unique_ptr<Command> createCameraViewCommand(
        Camera::CameraController* cameraController,
        const Math::WorldCoordinates& newPosition,
        const Math::WorldCoordinates& newTarget);
    
    static std::unique_ptr<Command> createCameraPresetCommand(
        Camera::CameraController* cameraController,
        Camera::ViewPreset newPreset);
};

} // namespace UndoRedo
} // namespace VoxelEditor