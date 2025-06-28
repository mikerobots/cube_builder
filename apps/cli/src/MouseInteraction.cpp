// Standard library includes first
#include <iostream>
#include <type_traits>
#include <vector>
#include <stack>
#include <mutex>
#include <memory>
#include <algorithm>

// GLFW for input handling
#include <GLFW/glfw3.h>

// Application headers
#include "cli/MouseInteraction.h"
#include "cli/Application.h"
#include "cli/RenderWindow.h"

// Core includes
#include "voxel_data/VoxelDataManager.h"
#include "camera/CameraController.h"
#include "input/InputManager.h"
#include "input/PlacementValidation.h"
#include "visual_feedback/FeedbackRenderer.h"
#include "visual_feedback/FaceDetector.h"
#include "undo_redo/HistoryManager.h"
#include "undo_redo/VoxelCommands.h"
#include "undo_redo/PlacementCommands.h"
#include "math/Ray.h"
#include "math/BoundingBox.h"
#include "logging/Logger.h"
#include "camera/OrbitCamera.h"
#include "math/CoordinateTypes.h"

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
    
    // Debug: Log whether we're in headless mode
    if (!m_renderWindow) {
        Logging::Logger::getInstance().info("MouseInteraction", "Initializing in headless mode (no render window)");
    }
    
    // Set up mouse callbacks (only if render window exists - not in headless mode)
    if (m_renderWindow) {
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
            // Handle scroll/pinch zoom
            if (event.deltaY != 0 || event.deltaX != 0) {
                onMouseScroll(event.deltaX, event.deltaY, event.ctrl, event.shift);
            }
        }
    });
    }
}

void MouseInteraction::update() {
    updateHoverState();
}

void MouseInteraction::onMouseMove(float x, float y) {
    // Check if mouse position is within window bounds
    if (m_renderWindow) {
        if (x < 0 || y < 0 || x >= m_renderWindow->getWidth() || y >= m_renderWindow->getHeight()) {
            // Mouse is outside window - clear hover state
            if (m_hasHoverFace) {
                m_hasHoverFace = false;
                m_feedbackRenderer->clearFaceHighlight();
                m_feedbackRenderer->clearVoxelPreview();
                Logging::Logger::getInstance().debug("MouseInteraction", "Mouse left window bounds - clearing hover state");
            }
            return;
        }
    }
    
    glm::vec2 oldPos = m_mousePos;
    m_mousePos = glm::vec2(x, y);
    
    // Debug log mouse movement
    static int moveCount = 0;
    if (moveCount++ % 60 == 0) {
        Logging::Logger::getInstance().debugfc("MouseInteraction",
            "Mouse move: pos=(%.1f,%.1f) oldPos=(%.1f,%.1f)", 
            x, y, oldPos.x, oldPos.y);
    }
    
    // Camera controls
    if (m_orbitMode) {
        // Orbit mode (either middle mouse or Ctrl/Cmd+left mouse)
        glm::vec2 delta = m_mousePos - oldPos;
        
        // Convert pixel movement to degrees (orbit expects degrees based on the OrbitCamera code)
        // Reduce sensitivity for more controlled movement
        float sensitivity = 0.2f;  // degrees per pixel
        float deltaYaw = delta.x * sensitivity;    // Positive X movement = rotate right
        float deltaPitch = delta.y * sensitivity;  // Positive Y movement = rotate down
        
        auto* orbitCamera = dynamic_cast<Camera::OrbitCamera*>(m_cameraController->getCamera());
        if (orbitCamera) {
            // Debug: Check target before and after orbit
            auto targetBefore = orbitCamera->getTarget();
            auto posBefore = orbitCamera->getPosition();
            
            orbitCamera->orbit(deltaYaw, deltaPitch);
            
            auto targetAfter = orbitCamera->getTarget();
            auto posAfter = orbitCamera->getPosition();
            
            // Log significant changes
            if ((targetAfter - targetBefore).length() > 0.001f) {
                Logging::Logger::getInstance().warningfc("MouseInteraction",
                    "Target moved during orbit! Before: (%.2f,%.2f,%.2f) After: (%.2f,%.2f,%.2f)",
                    targetBefore.x(), targetBefore.y(), targetBefore.z(),
                    targetAfter.x(), targetAfter.y(), targetAfter.z());
            }
        }
        
        // Log orbit only occasionally to avoid spam
        static int orbitLogCount = 0;
        if (orbitLogCount++ % 30 == 0 && orbitCamera) {
            auto pos = orbitCamera->getPosition();
            auto target = orbitCamera->getTarget();
            float distance = orbitCamera->getDistance();
            float yaw = orbitCamera->getYaw();
            float pitch = orbitCamera->getPitch();
            
            Logging::Logger::getInstance().debugfc("MouseInteraction",
                "Orbit: yaw=%.1f° pitch=%.1f° dist=%.2f target=(%.2f,%.2f,%.2f) pos=(%.2f,%.2f,%.2f)",
                yaw, pitch, distance, target.x(), target.y(), target.z(), pos.x(), pos.y(), pos.z());
        }
    } else if (m_panMode) {
        // Pan mode (Shift+left mouse)
        glm::vec2 delta = m_mousePos - oldPos;
        
        // Convert pixel movement to world space pan
        // Negative because we want to drag the world, not the camera
        float panSensitivity = 0.01f;
        float distance = m_cameraController->getCamera()->getDistance();
        
        // Scale pan speed based on zoom level for consistent feel
        float scaledSensitivity = panSensitivity * distance * 0.1f;
        
        Math::Vector3f panDelta(-delta.x * scaledSensitivity, 
                                delta.y * scaledSensitivity, 
                                0.0f);
        
        auto* orbitCamera = dynamic_cast<Camera::OrbitCamera*>(m_cameraController->getCamera());
        if (orbitCamera) {
            orbitCamera->pan(panDelta);
            
            // Log pan occasionally
            static int panLogCount = 0;
            if (panLogCount++ % 30 == 0) {
                auto target = orbitCamera->getTarget();
                Logging::Logger::getInstance().debugfc("MouseInteraction",
                    "Pan: delta=(%.1f,%.1f) target=(%.2f,%.2f,%.2f)",
                    delta.x, delta.y, target.x(), target.y(), target.z());
            }
        }
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
        
        // Get window dimensions, use default size if headless
        int width = 800;
        int height = 600;
        if (m_renderWindow) {
            width = m_renderWindow->getWidth();
            height = m_renderWindow->getHeight();
        }
        
        // Calculate NDC for debug
        float ndcX = (2.0f * x) / width - 1.0f;
        float ndcY = 1.0f - (2.0f * y) / height;
        
        std::cout << "\n=== Mouse Click Ray Info ===" << std::endl;
        std::cout << "Mouse Position: (" << x << ", " << y << ")" << std::endl;
        std::cout << "Window Size: " << width << " x " << height << std::endl;
        std::cout << "NDC: (" << ndcX << ", " << ndcY << ")" << std::endl;
        std::cout << "Ray Origin: (" << ray.origin.x << ", " << ray.origin.y << ", " << ray.origin.z << ")" << std::endl;
        std::cout << "Ray Direction: (" << ray.direction.x << ", " << ray.direction.y << ", " << ray.direction.z << ")" << std::endl;
        std::cout << "Ray Direction Length: " << ray.direction.length() << std::endl;
        
        // Also print camera info for context
        auto camPos = m_cameraController->getCamera()->getPosition();
        auto camTarget = m_cameraController->getCamera()->getTarget();
        std::cout << "Camera Position: (" << camPos.x() << ", " << camPos.y() << ", " << camPos.z() << ")" << std::endl;
        std::cout << "Camera Target: (" << camTarget.x() << ", " << camTarget.y() << ", " << camTarget.z() << ")" << std::endl;
        std::cout << "===========================" << std::endl;
    }
    
    // Get modifier keys (safely handle headless mode)
    bool ctrlPressed = false;
    bool cmdPressed = false;
    bool shiftPressed = false;
    
    // In headless mode, we can't query key states, so we'll assume no modifiers
    // Note: we cannot call ANY GLFW functions if GLFW is not initialized
    // In headless mode, GLFW is not initialized at all, so we skip key state queries entirely
    
    // On macOS, treat Cmd as Ctrl for consistency
    bool modifierPressed = ctrlPressed || cmdPressed;
    
    if (button == 0) { // Left click
        m_mousePressed = pressed;
        
        if (shiftPressed) {
            // Shift + Left click for pan
            if (pressed) {
                m_dragStart = m_mousePos;
                m_panMode = true;
                Logging::Logger::getInstance().debug("MouseInteraction", "Starting pan mode");
            } else {
                m_panMode = false;
                Logging::Logger::getInstance().debug("MouseInteraction", "Ending pan mode");
            }
        } else if (modifierPressed) {
            // Ctrl/Cmd + Left click for orbit
            if (pressed) {
                m_dragStart = m_mousePos;
                m_orbitMode = true;
                
                // Auto-center on voxels when starting orbit
                centerCameraOnVoxels();
                
                Logging::Logger::getInstance().debug("MouseInteraction", "Starting orbit mode");
            } else {
                m_orbitMode = false;
                Logging::Logger::getInstance().debug("MouseInteraction", "Ending orbit mode");
            }
        } else if (pressed && m_hasHoverFace) {
            // Regular left click for voxel placement
            Logging::Logger::getInstance().debug("MouseInteraction", "Placing voxel at hover position");
            placeVoxel();
        }
    } else if (button == 1) { // Right click
        if (pressed && m_hasHoverFace && !modifierPressed) {
            Logging::Logger::getInstance().debug("MouseInteraction", "Removing voxel at hover position");
            removeVoxel();
        }
    } else if (button == 2) { // Middle click
        m_middlePressed = pressed;
        if (pressed) {
            m_dragStart = m_mousePos;
            m_orbitMode = true;
            
            // Auto-center on voxels when starting orbit
            centerCameraOnVoxels();
        } else {
            m_orbitMode = false;
        }
    }
}

void MouseInteraction::onMouseScroll(float deltaX, float deltaY, bool ctrlPressed, bool shiftPressed) {
    // Handle different zoom methods:
    // 1. Trackpad pinch: Usually comes as scroll with Ctrl modifier on macOS
    // 2. Mouse wheel: Standard scroll without modifiers
    // 3. Two-finger scroll: Comes as regular scroll events
    // 4. Native trackpad: May have both X and Y components
    
    float zoomDelta = 0.0f;
    bool isPinchGesture = false;
    
    // Detect pinch gesture patterns
    if (ctrlPressed) {
        // Explicit pinch zoom (Ctrl+scroll)
        isPinchGesture = true;
        zoomDelta = deltaY;
    } else if (std::abs(deltaX) > 0.001f && std::abs(deltaY) > 0.001f) {
        // Two-finger scroll often has both X and Y components
        // Use the magnitude for smoother zoom
        float magnitude = std::sqrt(deltaX * deltaX + deltaY * deltaY);
        zoomDelta = (deltaY > 0) ? magnitude : -magnitude;
        isPinchGesture = true;
    } else {
        // Regular vertical scroll
        zoomDelta = deltaY;
    }
    
    // Apply zoom with appropriate speed
    float zoomSpeed = isPinchGesture ? 0.3f : 0.1f;
    
    // Invert the zoom direction for more intuitive control
    // Pinch out (fingers apart) = zoom in (decrease distance)
    // Pinch in (fingers together) = zoom out (increase distance)
    float factor = 1.0f - zoomDelta * zoomSpeed;
    
    float currentDistance = m_cameraController->getCamera()->getDistance();
    float newDistance = currentDistance * factor;
    
    // Clamp zoom distance
    newDistance = std::max(1.0f, std::min(50.0f, newDistance));
    m_cameraController->getCamera()->setDistance(newDistance);
    
    // Debug log only significant changes
    if (std::abs(newDistance - currentDistance) > 0.01f) {
        Logging::Logger::getInstance().debugfc("MouseInteraction",
            "%s zoom: deltaX=%.3f deltaY=%.3f factor=%.3f distance: %.2f -> %.2f",
            isPinchGesture ? "Pinch" : "Scroll",
            deltaX, deltaY, factor, currentDistance, newDistance);
    }
}

Math::Ray MouseInteraction::getMouseRay(float x, float y) const {
    // Get window dimensions, use default size if headless
    int width = 800;
    int height = 600;
    if (m_renderWindow) {
        width = m_renderWindow->getWidth();
        height = m_renderWindow->getHeight();
    }
    
    // Get normalized device coordinates
    float ndcX = (2.0f * x) / width - 1.0f;
    float ndcY = 1.0f - (2.0f * y) / height;
    
    // Debug log the mouse position and NDC
    static int debugCount = 0;
    if (debugCount++ % 30 == 0) {
        Logging::Logger::getInstance().debugfc("MouseInteraction",
            "Mouse: screen=(%.1f,%.1f) window=(%d,%d) ndc=(%.3f,%.3f)",
            x, y, width, height, ndcX, ndcY);
    }
    
    // Get camera matrices
    Math::Matrix4f viewMat = m_cameraController->getCamera()->getViewMatrix();
    Math::Matrix4f projMat = m_cameraController->getCamera()->getProjectionMatrix();
    
    // Convert to glm matrices
    // Math::Matrix4f is row-major: m[row * 4 + col]
    // GLM is column-major: mat[col][row]
    // Need to transpose during conversion
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            // Math::Matrix4f element at (row,col) goes to GLM element at [col][row]
            viewMatrix[col][row] = viewMat.m[row * 4 + col];
            projMatrix[col][row] = projMat.m[row * 4 + col];
        }
    }
    
    // Get camera position as ray origin
    Math::Vector3f camPosVec = m_cameraController->getCamera()->getPosition().value();
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
            camPos.x(), camPos.y(), camPos.z(), camTarget.x(), camTarget.y(), camTarget.z());
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
    
    // Cast ray and find nearest face or ground plane
    hitFace = detector.detectFaceOrGround(vfRay, *grid, m_voxelManager->getActiveResolution());
    
    static int hitCheckCount = 0;
    if (hitCheckCount++ % 60 == 0) {
        if (hitFace.isValid()) {
            auto voxelPos = hitFace.getVoxelPosition();
            Logging::Logger::getInstance().debugfc("MouseInteraction",
                "Hit face at voxel (%d,%d,%d)", voxelPos.x(), voxelPos.y(), voxelPos.z());
        }
    }
    
    return hitFace.isValid();
}

glm::ivec3 MouseInteraction::getPlacementPosition(const VisualFeedback::Face& face) const {
    using namespace Input;
    using namespace VoxelData;
    
    // Check Shift key state for snapping override
    bool shiftPressed = false;
    
    // Only query key state if we have a valid GLFW context (not in headless mode)
    GLFWwindow* currentWindow = glfwGetCurrentContext();
    if (currentWindow != nullptr) {
        shiftPressed = glfwGetKey(currentWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                       glfwGetKey(currentWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    }
    
    VoxelResolution resolution = m_voxelManager->getActiveResolution();
    Math::Vector3f workspaceSize = m_voxelManager->getWorkspaceManager()->getSize();
    
    // Get the hit point on the face for smart snapping
    Math::Vector3f hitPoint;
    if (face.isGroundPlane()) {
        hitPoint = face.getGroundPlaneHitPoint().value();
    } else {
        // For voxel faces, calculate hit point based on face center
        Math::Vector3i voxelPos = face.getVoxelPosition().value();
        Math::Vector3f voxelWorldPos = Math::CoordinateConverter::incrementToWorld(Math::IncrementCoordinates(voxelPos)).value();
        float voxelSize = getVoxelSize(resolution);
        
        // Get face center position
        hitPoint = voxelWorldPos + Math::Vector3f(voxelSize * 0.5f, voxelSize * 0.5f, voxelSize * 0.5f);
        
        // Offset hit point to the face surface
        Math::Vector3f normal = face.getNormal();
        hitPoint += normal * (voxelSize * 0.5f);
    }
    
    Math::Vector3i snappedPos;
    
    // Convert VisualFeedback::FaceDirection to VoxelData::FaceDirection for later use
    VoxelData::FaceDirection surfaceFaceDir = VoxelData::FaceDirection::PosY; // default
    if (!face.isGroundPlane()) {
        VisualFeedback::FaceDirection vfDir = face.getDirection();
        switch (vfDir) {
            case VisualFeedback::FaceDirection::PositiveX: surfaceFaceDir = VoxelData::FaceDirection::PosX; break;
            case VisualFeedback::FaceDirection::NegativeX: surfaceFaceDir = VoxelData::FaceDirection::NegX; break;
            case VisualFeedback::FaceDirection::PositiveY: surfaceFaceDir = VoxelData::FaceDirection::PosY; break;
            case VisualFeedback::FaceDirection::NegativeY: surfaceFaceDir = VoxelData::FaceDirection::NegY; break;
            case VisualFeedback::FaceDirection::PositiveZ: surfaceFaceDir = VoxelData::FaceDirection::PosZ; break;
            case VisualFeedback::FaceDirection::NegativeZ: surfaceFaceDir = VoxelData::FaceDirection::NegZ; break;
        }
    }
    
    if (face.isGroundPlane()) {
        // For ground plane, use smart context snapping (no specific surface face)
        Input::PlacementContext context = PlacementUtils::getSmartPlacementContext(
            Math::WorldCoordinates(hitPoint), resolution, shiftPressed, workspaceSize, *m_voxelManager, nullptr);
        snappedPos = context.snappedIncrementPos.value();
        
        Logging::Logger::getInstance().debugfc("MouseInteraction",
            "Ground plane placement: hit=%.2f,%.2f,%.2f snapped=%d,%d,%d shift=%d",
            hitPoint.x, hitPoint.y, hitPoint.z, snappedPos.x, snappedPos.y, snappedPos.z, shiftPressed);
    } else {
        // For voxel faces, use surface face grid snapping
        Math::IncrementCoordinates surfaceFaceVoxelPos = face.getVoxelPosition();
        VoxelResolution surfaceFaceVoxelRes = face.getResolution();
        
        Input::PlacementContext context = PlacementUtils::getSmartPlacementContext(
            Math::WorldCoordinates(hitPoint), resolution, shiftPressed, workspaceSize, *m_voxelManager,
            &surfaceFaceVoxelPos, surfaceFaceVoxelRes, surfaceFaceDir);
        snappedPos = context.snappedIncrementPos.value();
        
        Logging::Logger::getInstance().debugfc("MouseInteraction",
            "Surface face placement: voxel=%d,%d,%d dir=%d hit=%.2f,%.2f,%.2f snapped=%d,%d,%d shift=%d",
            surfaceFaceVoxelPos.x(), surfaceFaceVoxelPos.y(), surfaceFaceVoxelPos.z(), static_cast<int>(surfaceFaceDir),
            hitPoint.x, hitPoint.y, hitPoint.z, snappedPos.x, snappedPos.y, snappedPos.z, shiftPressed);
    }
    
    // Final validation - ensure position is valid
    Math::IncrementCoordinates snappedIncCoords(snappedPos);
    auto validation = m_voxelManager->validatePosition(snappedIncCoords, resolution);
    if (!validation.valid) {
        // Only log as warning if this is an actual click, not just hover preview
        static int warningCount = 0;
        if (warningCount++ < 5) { // Limit logging to avoid spam
            Logging::Logger::getInstance().debugfc("MouseInteraction",
                "Smart snapping resulted in invalid position %d,%d,%d, using fallback",
                snappedPos.x, snappedPos.y, snappedPos.z);
        }
        
        // Fallback: use simple adjacent position calculation
        if (face.isGroundPlane()) {
            // For ground plane, just use the snapped position with Y=0
            snappedPos.y = 0;
            // Clamp to workspace bounds (centered coordinate system)
            float voxelSizeF = getVoxelSize(resolution);
            int maxIncrement = static_cast<int>(workspaceSize.x / voxelSizeF) / 2;
            int minIncrement = -maxIncrement;
            snappedPos.x = std::max(minIncrement, std::min(snappedPos.x, maxIncrement - 1));
            snappedPos.z = std::max(minIncrement, std::min(snappedPos.z, maxIncrement - 1));
        } else {
            Math::Vector3i clickedVoxel = face.getVoxelPosition().value();
            Math::Vector3i newPos = m_voxelManager->getAdjacentPosition(
                clickedVoxel, surfaceFaceDir, face.getResolution(), resolution);
            snappedPos = newPos;
        }
    }
    
    return glm::ivec3(snappedPos.x, snappedPos.y, snappedPos.z);
}

void MouseInteraction::updateHoverState() {
    // Don't update hover state while in orbit or pan mode
    if (m_orbitMode || m_panMode) {
        // Clear any existing hover highlights during camera movement
        if (m_hasHoverFace) {
            m_hasHoverFace = false;
            m_feedbackRenderer->clearFaceHighlight();
            m_feedbackRenderer->clearVoxelPreview();
        }
        return;
    }
    
    // Get mouse ray
    Math::Ray ray = getMouseRay(m_mousePos.x, m_mousePos.y);
    
    // Set ray for visualization if enabled (will be rendered during render phase)
    if (m_rayVisualizationEnabled && m_feedbackRenderer) {
        VisualFeedback::Ray vfRay(ray.origin, ray.direction);
        m_feedbackRenderer->setDebugRay(vfRay, true);
    } else if (m_feedbackRenderer) {
        m_feedbackRenderer->clearDebugRay();
    }
    
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
            auto center = newHoverFace.getCenter().value();
            Logging::Logger::getInstance().debugfc("MouseInteraction",
                "Started hovering over face at %.2f,%.2f,%.2f", center.x, center.y, center.z);
        }
        m_hasHoverFace = true;
        m_hoverFace = newHoverFace;
        m_previewPos = getPlacementPosition(m_hoverFace);
        
        // Update visual feedback
        m_feedbackRenderer->renderFaceHighlight(m_hoverFace);
        
        // Validate preview position before rendering
        VoxelData::VoxelResolution currentResolution = m_voxelManager->getActiveResolution();
        Math::Vector3i previewPosVec(m_previewPos.x, m_previewPos.y, m_previewPos.z);
        
        // Check if preview position is valid for current resolution
        Math::IncrementCoordinates previewIncCoords(previewPosVec);
        auto validation = m_voxelManager->validatePosition(previewIncCoords, currentResolution);
        bool validPosition = validation.valid;
        
        // Render preview with appropriate color based on validation
        Rendering::Color previewColor = validPosition ? 
            Rendering::Color::Green() : Rendering::Color::Red();
        m_feedbackRenderer->renderVoxelPreview(previewPosVec, currentResolution, previewColor);
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
    
    // Use PlacementCommandFactory to create placement command with validation
    auto cmd = UndoRedo::PlacementCommandFactory::createPlacementCommand(
        m_voxelManager,
        Math::IncrementCoordinates(Math::Vector3i(m_previewPos.x, m_previewPos.y, m_previewPos.z)),
        m_voxelManager->getActiveResolution()
    );
    
    if (!cmd) {
        Logging::Logger::getInstance().warning("MouseInteraction", "Failed to create placement command - validation failed");
        return;
    }
    
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
    Math::Vector3i voxelPos = m_hoverFace.getVoxelPosition().value();
    
    // Use PlacementCommandFactory to create removal command with validation
    auto cmd = UndoRedo::PlacementCommandFactory::createRemovalCommand(
        m_voxelManager,
        Math::IncrementCoordinates(voxelPos),
        m_voxelManager->getActiveResolution()
    );
    
    if (!cmd) {
        Logging::Logger::getInstance().warning("MouseInteraction", "Failed to create removal command - validation failed");
        return;
    }
    
    // Execute through history manager
    m_historyManager->executeCommand(std::move(cmd));
    
    // Update hover state since geometry changed
    updateHoverState();
    
    // Update mesh for rendering
    m_app->requestMeshUpdate();
}

void MouseInteraction::centerCameraOnVoxels() {
    // Calculate bounds of all voxels
    Math::BoundingBox bounds;
    bool hasVoxels = false;
    
    auto* grid = m_voxelManager->getGrid(m_voxelManager->getActiveResolution());
    if (grid) {
        float voxelSize = VoxelData::getVoxelSize(m_voxelManager->getActiveResolution());
        auto allVoxels = m_voxelManager->getAllVoxels();
        
        for (const auto& voxel : allVoxels) {
            // Convert grid position to world position (center of voxel)
            // Use CoordinateConverter for proper centered coordinate system
            Math::WorldCoordinates worldPos = Math::CoordinateConverter::incrementToWorld(voxel.incrementPos);
            Math::Vector3f voxelCenter = worldPos.value();
            
            if (!hasVoxels) {
                bounds.min = bounds.max = voxelCenter;
                hasVoxels = true;
            } else {
                bounds.expand(voxelCenter);
            }
        }
    }
    
    if (hasVoxels) {
        Math::Vector3f center = bounds.getCenter();
        auto* orbitCamera = dynamic_cast<Camera::OrbitCamera*>(m_cameraController->getCamera());
        if (orbitCamera) {
            orbitCamera->setTarget(Math::WorldCoordinates(center));
            
            // Optionally adjust distance based on bounds size
            Math::Vector3f size = bounds.max - bounds.min;
            float maxDimension = std::max({size.x, size.y, size.z});
            float distance = maxDimension * 2.0f; // Give some breathing room
            distance = std::max(1.0f, std::min(50.0f, distance)); // Clamp to reasonable range
            orbitCamera->setDistance(distance);
            
            Logging::Logger::getInstance().debugfc("MouseInteraction",
                "Centered camera on voxels: center=(%.2f,%.2f,%.2f) distance=%.2f",
                center.x, center.y, center.z, distance);
        }
    } else {
        // No voxels, keep current position
        Logging::Logger::getInstance().debug("MouseInteraction", "No voxels to center on");
    }
}

} // namespace VoxelEditor