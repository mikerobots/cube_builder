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
#include "logging/Logger.h"

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
        // Always update position for hover
        onMouseMove(event.x, event.y);
        
        // Check for button events
        static bool lastLeftPressed = false;
        static bool lastRightPressed = false;
        static bool lastMiddlePressed = false;
        
        // Handle button press/release events
        if (event.button == MouseButton::Left && event.pressed != lastLeftPressed) {
            onMouseClick(0, event.pressed, event.x, event.y);
            lastLeftPressed = event.pressed;
        } else if (event.button == MouseButton::Right && event.pressed != lastRightPressed) {
            onMouseClick(1, event.pressed, event.x, event.y);
            lastRightPressed = event.pressed;
        } else if (event.button == MouseButton::Middle) {
            if (event.pressed != lastMiddlePressed) {
                onMouseClick(2, event.pressed, event.x, event.y);
                lastMiddlePressed = event.pressed;
            }
            // Handle scroll
            if (event.deltaY != 0) {
                onMouseScroll(event.deltaY);
            }
        }
    });
}

void MouseInteraction::update() {
    updateHoverState();
}

void MouseInteraction::onMouseMove(float x, float y) {
    glm::vec2 oldPos = m_mousePos;
    m_mousePos = glm::vec2(x, y);
    
    // Debug log mouse movement
    static int moveCount = 0;
    if (moveCount++ % 60 == 0) {
        Logging::Logger::getInstance().debugfc("MouseInteraction",
            "Mouse move: pos=(%.1f,%.1f) oldPos=(%.1f,%.1f)", 
            x, y, oldPos.x, oldPos.y);
    }
    
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
    
    Logging::Logger::getInstance().debugfc("MouseInteraction", 
        "Mouse click: button=%d pressed=%d pos=(%.1f,%.1f) hasHoverFace=%d",
        button, pressed, x, y, m_hasHoverFace);
    
    // Print ray information on click
    if (pressed) {
        Math::Ray ray = getMouseRay(x, y);
        
        // Calculate NDC for debug
        float ndcX = (2.0f * x) / m_renderWindow->getWidth() - 1.0f;
        float ndcY = 1.0f - (2.0f * y) / m_renderWindow->getHeight();
        
        std::cout << "\n=== Mouse Click Ray Info ===" << std::endl;
        std::cout << "Mouse Position: (" << x << ", " << y << ")" << std::endl;
        std::cout << "Window Size: " << m_renderWindow->getWidth() << " x " << m_renderWindow->getHeight() << std::endl;
        std::cout << "NDC: (" << ndcX << ", " << ndcY << ")" << std::endl;
        std::cout << "Ray Origin: (" << ray.origin.x << ", " << ray.origin.y << ", " << ray.origin.z << ")" << std::endl;
        std::cout << "Ray Direction: (" << ray.direction.x << ", " << ray.direction.y << ", " << ray.direction.z << ")" << std::endl;
        std::cout << "Ray Direction Length: " << ray.direction.length() << std::endl;
        
        // Also print camera info for context
        auto camPos = m_cameraController->getCamera()->getPosition();
        auto camTarget = m_cameraController->getCamera()->getTarget();
        std::cout << "Camera Position: (" << camPos.x << ", " << camPos.y << ", " << camPos.z << ")" << std::endl;
        std::cout << "Camera Target: (" << camTarget.x << ", " << camTarget.y << ", " << camTarget.z << ")" << std::endl;
        std::cout << "===========================" << std::endl;
    }
    
    if (button == 0) { // Left click
        m_mousePressed = pressed;
        
        if (pressed && m_hasHoverFace) {
            Logging::Logger::getInstance().debug("MouseInteraction", "Placing voxel at hover position");
            placeVoxel();
        }
    } else if (button == 1) { // Right click
        if (pressed && m_hasHoverFace) {
            Logging::Logger::getInstance().debug("MouseInteraction", "Removing voxel at hover position");
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
    
    // Debug log the mouse position and NDC
    static int debugCount = 0;
    if (debugCount++ % 30 == 0) {
        Logging::Logger::getInstance().debugfc("MouseInteraction",
            "Mouse: screen=(%.1f,%.1f) window=(%d,%d) ndc=(%.3f,%.3f)",
            x, y, m_renderWindow->getWidth(), m_renderWindow->getHeight(), ndcX, ndcY);
    }
    
    // Get camera matrices
    Math::Matrix4f viewMat = m_cameraController->getCamera()->getViewMatrix();
    Math::Matrix4f projMat = m_cameraController->getCamera()->getProjectionMatrix();
    
    // Convert to glm matrices
    // GLM uses column-major ordering, our matrices use row-major
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            viewMatrix[col][row] = viewMat.m[row * 4 + col];
            projMatrix[col][row] = projMat.m[row * 4 + col];
        }
    }
    
    // Get camera position as ray origin
    Math::Vector3f camPosVec = m_cameraController->getCamera()->getPosition();
    glm::vec3 cameraPos(camPosVec.x, camPosVec.y, camPosVec.z);
    
    // Unproject a point on the far plane to get ray direction
    glm::mat4 invVP = glm::inverse(projMatrix * viewMatrix);
    glm::vec4 farPoint = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    farPoint /= farPoint.w;
    
    // Create ray from camera position through the unprojected point
    glm::vec3 origin = cameraPos;
    glm::vec3 direction = glm::normalize(glm::vec3(farPoint) - cameraPos);
    
    static int rayCount = 0;
    if (rayCount++ % 60 == 0) {
        // Debug camera info
        auto camPos = m_cameraController->getCamera()->getPosition();
        auto camTarget = m_cameraController->getCamera()->getTarget();
        Logging::Logger::getInstance().debugfc("MouseInteraction",
            "Camera: pos=(%.2f,%.2f,%.2f) target=(%.2f,%.2f,%.2f)",
            camPos.x, camPos.y, camPos.z, camTarget.x, camTarget.y, camTarget.z);
        Logging::Logger::getInstance().debugfc("MouseInteraction",
            "Ray: origin=(%.2f,%.2f,%.2f) dir=(%.3f,%.3f,%.3f)",
            origin.x, origin.y, origin.z, direction.x, direction.y, direction.z);
    }
    
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
    if (!grid) {
        Logging::Logger::getInstance().debug("MouseInteraction", "No grid available for raycast");
        return false;
    }
    
    // Debug: Check if grid has any voxels
    static int raycastCount = 0;
    if (raycastCount++ % 60 == 0) {
        size_t voxelCount = grid->getVoxelCount();
        Logging::Logger::getInstance().debugfc("MouseInteraction",
            "Raycasting against grid with %zu voxels", voxelCount);
    }
    
    // Cast ray and find nearest face
    hitFace = detector.detectFace(vfRay, *grid, m_voxelManager->getActiveResolution());
    
    static int hitCheckCount = 0;
    if (hitCheckCount++ % 60 == 0) {
        if (hitFace.isValid()) {
            auto voxelPos = hitFace.getVoxelPosition();
            Logging::Logger::getInstance().debugfc("MouseInteraction",
                "Hit face at voxel (%d,%d,%d)", voxelPos.x, voxelPos.y, voxelPos.z);
        }
    }
    
    return hitFace.isValid();
}

glm::ivec3 MouseInteraction::getPlacementPosition(const VisualFeedback::Face& face) const {
    // Get the voxel position of the face we clicked on
    Math::Vector3i clickedVoxel = face.getVoxelPosition();
    Math::Vector3f normal = face.getNormal();
    
    Logging::Logger::getInstance().debugfc("MouseInteraction",
        "Clicked voxel: (%d,%d,%d) face normal: (%.2f,%.2f,%.2f)",
        clickedVoxel.x, clickedVoxel.y, clickedVoxel.z, normal.x, normal.y, normal.z);
    
    // Calculate new voxel position by adding the normal direction
    Math::Vector3i newPos = clickedVoxel;
    if (normal.x > 0.5f) newPos.x += 1;
    else if (normal.x < -0.5f) newPos.x -= 1;
    else if (normal.y > 0.5f) newPos.y += 1;
    else if (normal.y < -0.5f) newPos.y -= 1;
    else if (normal.z > 0.5f) newPos.z += 1;
    else if (normal.z < -0.5f) newPos.z -= 1;
    
    Logging::Logger::getInstance().debugfc("MouseInteraction",
        "Placement grid pos: (%d,%d,%d)", newPos.x, newPos.y, newPos.z);
    
    return glm::ivec3(newPos.x, newPos.y, newPos.z);
}

void MouseInteraction::updateHoverState() {
    // Get mouse ray
    Math::Ray ray = getMouseRay(m_mousePos.x, m_mousePos.y);
    
    // Perform raycast
    VisualFeedback::Face newHoverFace;
    bool hasHit = performRaycast(ray, newHoverFace);
    
    static int updateCount = 0;
    if (updateCount++ % 60 == 0) { // Log every 60 frames (~1 second)
        Logging::Logger::getInstance().debugfc("MouseInteraction",
            "Hover update: mousePos=(%.1f,%.1f) hasHit=%d", m_mousePos.x, m_mousePos.y, hasHit);
    }
    
    if (hasHit) {
        if (!m_hasHoverFace) {
            auto center = newHoverFace.getCenter();
            Logging::Logger::getInstance().debugfc("MouseInteraction",
                "Started hovering over face at %.2f,%.2f,%.2f", center.x, center.y, center.z);
        }
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
        if (m_hasHoverFace) {
            Logging::Logger::getInstance().debug("MouseInteraction", "Stopped hovering over face");
        }
        m_hasHoverFace = false;
        m_feedbackRenderer->clearFaceHighlight();
        m_feedbackRenderer->clearVoxelPreview();
    }
}

void MouseInteraction::placeVoxel() {
    if (!m_hasHoverFace) {
        Logging::Logger::getInstance().debug("MouseInteraction", "placeVoxel: No hover face, returning");
        return;
    }
    
    Logging::Logger::getInstance().debugfc("MouseInteraction",
        "Placing voxel at position (%d, %d, %d)", m_previewPos.x, m_previewPos.y, m_previewPos.z);
    
    // Create place command (VoxelEditCommand with value = true)
    auto cmd = std::make_unique<UndoRedo::VoxelEditCommand>(
        m_voxelManager,
        Math::Vector3i(m_previewPos.x, m_previewPos.y, m_previewPos.z),
        m_voxelManager->getActiveResolution(),
        true  // Place voxel
    );
    
    // Execute through history manager
    bool success = m_historyManager->executeCommand(std::move(cmd));
    Logging::Logger::getInstance().debugfc("MouseInteraction",
        "Command execution result: %s", success ? "success" : "failed");
    
    // Update hover state since geometry changed
    updateHoverState();
    
    // Update mesh for rendering
    Logging::Logger::getInstance().debug("MouseInteraction", "Requesting mesh update");
    m_app->requestMeshUpdate();
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
    
    // Update mesh for rendering
    m_app->requestMeshUpdate();
}

} // namespace VoxelEditor