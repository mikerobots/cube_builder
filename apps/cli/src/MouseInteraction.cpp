// Standard library includes first
#include <iostream>
#include <type_traits>
#include <vector>
#include <stack>
#include <mutex>
#include <memory>
#include <algorithm>

// Application headers
#include "cli/MouseInteraction.h"
#include "cli/Application.h"
#include "cli/RenderWindow.h"

// Core includes
#include "voxel_data/VoxelDataManager.h"
#include "camera/CameraController.h"
#include "input/InputManager.h"
#include "visual_feedback/FeedbackRenderer.h"
#include "visual_feedback/FaceDetector.h"
#include "undo_redo/HistoryManager.h"
#include "undo_redo/VoxelCommands.h"
#include "math/Ray.h"

namespace VoxelEditor {

MouseInteraction::MouseInteraction(Application* app)
    : m_app(app) {
}

void MouseInteraction::initialize() {
    // Cache system pointers
    m_voxelManager = m_app->getVoxelManager();
    m_cameraController = m_app->getCameraController();
    m_inputManager = m_app->getInputManager();
    m_feedbackRenderer = m_app->getFeedbackRenderer();
    m_historyManager = m_app->getHistoryManager();
    m_renderWindow = m_app->getRenderWindow();
    
    // Set up mouse callbacks
    m_renderWindow->setMouseCallback([this](const MouseEvent& event) {
        if (event.button == MouseButton::Left || event.button == MouseButton::Right) {
            onMouseClick(static_cast<int>(event.button), event.pressed, event.x, event.y);
        } else if (event.button == MouseButton::Middle) {
            if (event.deltaY != 0) {
                onMouseScroll(event.deltaY);
            }
        }
        
        // Always update position for hover
        onMouseMove(event.x, event.y);
    });
}

void MouseInteraction::update() {
    updateHoverState();
}

void MouseInteraction::onMouseMove(float x, float y) {
    glm::vec2 oldPos = m_mousePos;
    m_mousePos = glm::vec2(x, y);
    
    // Camera orbit mode
    if (m_orbitMode && m_middlePressed) {
        glm::vec2 delta = m_mousePos - oldPos;
        
        // Convert pixel movement to radians
        float sensitivity = 0.01f;
        float deltaTheta = -delta.x * sensitivity;
        float deltaPhi = -delta.y * sensitivity;
        
        m_cameraController->getCamera()->orbit(deltaTheta, deltaPhi);
    }
}

void MouseInteraction::onMouseClick(int button, bool pressed, float x, float y) {
    m_mousePos = glm::vec2(x, y);
    
    if (button == 0) { // Left click
        m_mousePressed = pressed;
        
        if (pressed && m_hasHoverFace) {
            placeVoxel();
        }
    } else if (button == 1) { // Right click
        if (pressed && m_hasHoverFace) {
            removeVoxel();
        }
    } else if (button == 2) { // Middle click
        m_middlePressed = pressed;
        if (pressed) {
            m_dragStart = m_mousePos;
            m_orbitMode = true;
        } else {
            m_orbitMode = false;
        }
    }
}

void MouseInteraction::onMouseScroll(float deltaY) {
    // Zoom with scroll
    float zoomSpeed = 0.1f;
    float factor = 1.0f + deltaY * zoomSpeed;
    
    float currentDistance = m_cameraController->getCamera()->getDistance();
    m_cameraController->getCamera()->setDistance(currentDistance / factor);
}

Math::Ray MouseInteraction::getMouseRay(float x, float y) const {
    // Get normalized device coordinates
    float ndcX = (2.0f * x) / m_renderWindow->getWidth() - 1.0f;
    float ndcY = 1.0f - (2.0f * y) / m_renderWindow->getHeight();
    
    // Get camera matrices
    Math::Matrix4f viewMat = m_cameraController->getCamera()->getViewMatrix();
    Math::Matrix4f projMat = m_cameraController->getCamera()->getProjectionMatrix();
    
    // Convert to glm matrices
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            viewMatrix[i][j] = viewMat.m[i * 4 + j];
            projMatrix[i][j] = projMat.m[i * 4 + j];
        }
    }
    
    // Unproject near and far points
    glm::mat4 invVP = glm::inverse(projMatrix * viewMatrix);
    
    glm::vec4 nearPoint = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 farPoint = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    
    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;
    
    // Create ray
    glm::vec3 origin = glm::vec3(nearPoint);
    glm::vec3 direction = glm::normalize(glm::vec3(farPoint) - origin);
    
    return Math::Ray(Math::Vector3f(origin.x, origin.y, origin.z), 
                     Math::Vector3f(direction.x, direction.y, direction.z));
}

bool MouseInteraction::performRaycast(const Math::Ray& ray, VisualFeedback::Face& hitFace) const {
    // Create face detector
    VisualFeedback::FaceDetector detector;
    
    // Create VisualFeedback::Ray from Math::Ray
    VisualFeedback::Ray vfRay(ray.origin, ray.direction);
    
    // Get active grid
    const VoxelData::VoxelGrid* grid = m_voxelManager->getGrid(m_voxelManager->getActiveResolution());
    if (!grid) return false;
    
    // Cast ray and find nearest face
    hitFace = detector.detectFace(vfRay, *grid, m_voxelManager->getActiveResolution());
    return hitFace.isValid();
}

glm::ivec3 MouseInteraction::getPlacementPosition(const VisualFeedback::Face& face) const {
    // Get voxel size for current resolution
    float voxelSize = VoxelData::getVoxelSize(m_voxelManager->getActiveResolution());
    
    // Get face center and normal
    Math::Vector3f center = face.getCenter();
    Math::Vector3f normal = face.getNormal();
    
    // Offset along normal by half voxel size
    Math::Vector3f offset = normal * (voxelSize * 0.5f);
    Math::Vector3f placementPos = center + offset;
    
    // Convert to voxel coordinates
    VoxelData::VoxelPosition voxelPos = VoxelData::VoxelPosition::fromWorldSpace(
        placementPos, 
        m_voxelManager->getActiveResolution(),
        m_voxelManager->getWorkspaceSize()
    );
    
    return glm::ivec3(voxelPos.gridPos.x, voxelPos.gridPos.y, voxelPos.gridPos.z);
}

void MouseInteraction::updateHoverState() {
    // Get mouse ray
    Math::Ray ray = getMouseRay(m_mousePos.x, m_mousePos.y);
    
    // Perform raycast
    VisualFeedback::Face newHoverFace;
    bool hasHit = performRaycast(ray, newHoverFace);
    
    if (hasHit) {
        m_hasHoverFace = true;
        m_hoverFace = newHoverFace;
        m_previewPos = getPlacementPosition(m_hoverFace);
        
        // Update visual feedback
        m_feedbackRenderer->renderFaceHighlight(m_hoverFace);
        m_feedbackRenderer->renderVoxelPreview(
            Math::Vector3i(m_previewPos.x, m_previewPos.y, m_previewPos.z), 
            m_voxelManager->getActiveResolution()
        );
    } else {
        m_hasHoverFace = false;
        m_feedbackRenderer->clearFaceHighlight();
        m_feedbackRenderer->clearVoxelPreview();
    }
}

void MouseInteraction::placeVoxel() {
    if (!m_hasHoverFace) return;
    
    // Create place command (VoxelEditCommand with value = true)
    auto cmd = std::make_unique<UndoRedo::VoxelEditCommand>(
        m_voxelManager,
        Math::Vector3i(m_previewPos.x, m_previewPos.y, m_previewPos.z),
        m_voxelManager->getActiveResolution(),
        true  // Place voxel
    );
    
    // Execute through history manager
    m_historyManager->executeCommand(std::move(cmd));
    
    // Update hover state since geometry changed
    updateHoverState();
}

void MouseInteraction::removeVoxel() {
    if (!m_hasHoverFace) return;
    
    // Get the voxel position from the face
    Math::Vector3i voxelPos = m_hoverFace.getVoxelPosition();
    
    // Remove the voxel we're pointing at (VoxelEditCommand with value = false)
    auto cmd = std::make_unique<UndoRedo::VoxelEditCommand>(
        m_voxelManager,
        voxelPos,
        m_voxelManager->getActiveResolution(),
        false  // Remove voxel
    );
    
    // Execute through history manager
    m_historyManager->executeCommand(std::move(cmd));
    
    // Update hover state since geometry changed
    updateHoverState();
}

} // namespace VoxelEditor