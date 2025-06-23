#include "SystemCommands.h"
#include "foundation/logging/Logger.h"
#include "../camera/CameraController.h"
#include "../camera/Camera.h"
#include "../camera/OrbitCamera.h"

namespace VoxelEditor {
namespace UndoRedo {

// ============================================================================
// ResolutionChangeCommand
// ============================================================================

ResolutionChangeCommand::ResolutionChangeCommand(VoxelData::VoxelDataManager* voxelManager,
                                               VoxelData::VoxelResolution newResolution)
    : m_voxelManager(voxelManager)
    , m_newResolution(newResolution) {
    m_oldResolution = m_voxelManager->getActiveResolution();
}

bool ResolutionChangeCommand::execute() {
    if (!m_voxelManager) {
        LOG_ERROR("VoxelDataManager is null");
        return false;
    }
    
    m_voxelManager->setActiveResolution(m_newResolution);
    return true;
}

bool ResolutionChangeCommand::undo() {
    if (!m_voxelManager) {
        LOG_ERROR("VoxelDataManager is null");
        return false;
    }
    
    m_voxelManager->setActiveResolution(m_oldResolution);
    return true;
}

size_t ResolutionChangeCommand::getMemoryUsage() const {
    return sizeof(*this);
}

// ============================================================================
// WorkspaceResizeCommand
// ============================================================================

WorkspaceResizeCommand::WorkspaceResizeCommand(VoxelData::VoxelDataManager* voxelManager,
                                             const Math::Vector3f& newSize)
    : m_voxelManager(voxelManager)
    , m_newSize(newSize) {
    m_oldSize = m_voxelManager->getWorkspaceSize();
}

bool WorkspaceResizeCommand::execute() {
    if (!m_voxelManager) {
        LOG_ERROR("VoxelDataManager is null");
        return false;
    }
    
    return m_voxelManager->resizeWorkspace(m_newSize);
}

bool WorkspaceResizeCommand::undo() {
    if (!m_voxelManager) {
        LOG_ERROR("VoxelDataManager is null");
        return false;
    }
    
    return m_voxelManager->resizeWorkspace(m_oldSize);
}

size_t WorkspaceResizeCommand::getMemoryUsage() const {
    return sizeof(*this);
}

// ============================================================================
// CameraViewCommand
// ============================================================================

CameraViewCommand::CameraViewCommand(Camera::CameraController* cameraController,
                                   const Math::WorldCoordinates& newPosition,
                                   const Math::WorldCoordinates& newTarget)
    : m_cameraController(cameraController)
    , m_newPosition(newPosition)
    , m_newTarget(newTarget) {
    
    if (m_cameraController && m_cameraController->getCamera()) {
        auto* camera = m_cameraController->getCamera();
        m_oldPosition = camera->getPosition();
        m_oldTarget = camera->getTarget();
        m_oldDistance = camera->getDistance();
        
        // Calculate new distance based on new position and target
        float dx = m_newPosition.x() - m_newTarget.x();
        float dy = m_newPosition.y() - m_newTarget.y();
        float dz = m_newPosition.z() - m_newTarget.z();
        m_newDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
    }
}

bool CameraViewCommand::execute() {
    if (!m_cameraController || !m_cameraController->getCamera()) {
        LOG_ERROR("CameraController or Camera is null");
        return false;
    }
    
    auto* camera = m_cameraController->getCamera();
    camera->setPosition(m_newPosition);
    camera->setTarget(m_newTarget);
    camera->setDistance(m_newDistance);
    return true;
}

bool CameraViewCommand::undo() {
    if (!m_cameraController || !m_cameraController->getCamera()) {
        LOG_ERROR("CameraController or Camera is null");
        return false;
    }
    
    auto* camera = m_cameraController->getCamera();
    camera->setPosition(m_oldPosition);
    camera->setTarget(m_oldTarget);
    camera->setDistance(m_oldDistance);
    return true;
}

size_t CameraViewCommand::getMemoryUsage() const {
    return sizeof(*this);
}

// ============================================================================
// CameraPresetCommand
// ============================================================================

CameraPresetCommand::CameraPresetCommand(Camera::CameraController* cameraController,
                                       Camera::ViewPreset newPreset)
    : m_cameraController(cameraController)
    , m_newPreset(newPreset)
    , m_oldYaw(0.0f)
    , m_oldPitch(0.0f) {
    
    if (m_cameraController && m_cameraController->getCamera()) {
        auto* camera = m_cameraController->getCamera();
        m_oldPosition = camera->getPosition();
        m_oldTarget = camera->getTarget();
        m_oldDistance = camera->getDistance();
        
        // If it's an OrbitCamera, also store yaw and pitch
        if (auto* orbitCamera = dynamic_cast<Camera::OrbitCamera*>(camera)) {
            m_oldYaw = orbitCamera->getYaw();
            m_oldPitch = orbitCamera->getPitch();
        }
    }
}

bool CameraPresetCommand::execute() {
    if (!m_cameraController || !m_cameraController->getCamera()) {
        LOG_ERROR("CameraController or Camera is null");
        return false;
    }
    
    m_cameraController->setViewPreset(m_newPreset);
    
    // Special handling for isometric view
    if (m_newPreset == Camera::ViewPreset::ISOMETRIC) {
        m_cameraController->getCamera()->setDistance(3.0f);
    }
    
    return true;
}

bool CameraPresetCommand::undo() {
    if (!m_cameraController || !m_cameraController->getCamera()) {
        LOG_ERROR("CameraController or Camera is null");
        return false;
    }
    
    auto* camera = m_cameraController->getCamera();
    
    // If it's an OrbitCamera, restore yaw and pitch first
    if (auto* orbitCamera = dynamic_cast<Camera::OrbitCamera*>(camera)) {
        orbitCamera->setYaw(m_oldYaw);
        orbitCamera->setPitch(m_oldPitch);
        orbitCamera->setDistance(m_oldDistance);
        orbitCamera->setTarget(m_oldTarget);
    } else {
        // For non-orbit cameras, just restore position and target
        camera->setPosition(m_oldPosition);
        camera->setTarget(m_oldTarget);
        camera->setDistance(m_oldDistance);
    }
    
    return true;
}

size_t CameraPresetCommand::getMemoryUsage() const {
    return sizeof(*this);
}

// ============================================================================
// SystemCommandFactory
// ============================================================================

std::unique_ptr<Command> SystemCommandFactory::createResolutionChangeCommand(
    VoxelData::VoxelDataManager* voxelManager,
    VoxelData::VoxelResolution newResolution) {
    
    if (!voxelManager) {
        LOG_ERROR("VoxelDataManager is null");
        return nullptr;
    }
    
    // Don't create command if resolution hasn't changed
    if (voxelManager->getActiveResolution() == newResolution) {
        return nullptr;
    }
    
    return std::make_unique<ResolutionChangeCommand>(voxelManager, newResolution);
}

std::unique_ptr<Command> SystemCommandFactory::createWorkspaceResizeCommand(
    VoxelData::VoxelDataManager* voxelManager,
    const Math::Vector3f& newSize) {
    
    if (!voxelManager) {
        LOG_ERROR("VoxelDataManager is null");
        return nullptr;
    }
    
    // Don't create command if size hasn't changed
    auto currentSize = voxelManager->getWorkspaceSize();
    const float epsilon = 0.001f;
    if (std::abs(currentSize.x - newSize.x) < epsilon &&
        std::abs(currentSize.y - newSize.y) < epsilon &&
        std::abs(currentSize.z - newSize.z) < epsilon) {
        return nullptr;
    }
    
    return std::make_unique<WorkspaceResizeCommand>(voxelManager, newSize);
}

std::unique_ptr<Command> SystemCommandFactory::createCameraViewCommand(
    Camera::CameraController* cameraController,
    const Math::WorldCoordinates& newPosition,
    const Math::WorldCoordinates& newTarget) {
    
    if (!cameraController || !cameraController->getCamera()) {
        LOG_ERROR("CameraController or Camera is null");
        return nullptr;
    }
    
    // Don't create command if camera hasn't moved significantly
    auto* camera = cameraController->getCamera();
    auto currentPos = camera->getPosition();
    auto currentTarget = camera->getTarget();
    
    const float epsilon = 0.01f;
    if (std::abs(currentPos.x() - newPosition.x()) < epsilon &&
        std::abs(currentPos.y() - newPosition.y()) < epsilon &&
        std::abs(currentPos.z() - newPosition.z()) < epsilon &&
        std::abs(currentTarget.x() - newTarget.x()) < epsilon &&
        std::abs(currentTarget.y() - newTarget.y()) < epsilon &&
        std::abs(currentTarget.z() - newTarget.z()) < epsilon) {
        return nullptr;
    }
    
    return std::make_unique<CameraViewCommand>(cameraController, newPosition, newTarget);
}

std::unique_ptr<Command> SystemCommandFactory::createCameraPresetCommand(
    Camera::CameraController* cameraController,
    Camera::ViewPreset newPreset) {
    
    if (!cameraController || !cameraController->getCamera()) {
        LOG_ERROR("CameraController or Camera is null");
        return nullptr;
    }
    
    return std::make_unique<CameraPresetCommand>(cameraController, newPreset);
}

} // namespace UndoRedo
} // namespace VoxelEditor