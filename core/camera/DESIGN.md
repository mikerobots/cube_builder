# Camera System Subsystem

## Purpose
Manages 3D camera controls, view presets, smooth transitions, and camera state persistence for the voxel editor across all platforms.

## Current Implementation Status
The camera subsystem is implemented as a set of header-only classes providing orbit-style camera controls with comprehensive input handling. The implementation is simpler than originally designed but functionally complete for basic 3D editing needs.

**Architecture Overview:**
- **Camera** - Abstract base class providing core camera state and matrix management
- **OrbitCamera** - Concrete implementation with spherical coordinate controls and built-in view presets
- **CameraController** - High-level controller managing mouse interaction and viewport integration
- **Viewport** - Screen-space coordinate transformations and ray casting utilities

**Implementation Characteristics:**
- All classes are header-only for optimal performance
- Event-driven architecture for state change notifications
- Lazy matrix computation with caching
- Built-in smoothing system for animated transitions
- Comprehensive test coverage (108 tests across 7 test suites)

## Key Components

### Camera (Abstract Base Class)
**Responsibility**: Core camera functionality with matrix management
- Position, target, and up vector management
- View and projection matrix generation with caching
- Event dispatch for camera state changes
- Direction vector calculations (forward, right, up)
- Abstract interface for view preset selection

### OrbitCamera (Concrete Implementation)
**Responsibility**: Spherical coordinate orbit camera
- Yaw/pitch/distance based positioning
- Mouse-driven orbit, pan, and zoom controls
- Built-in view presets (7 standard views)
- Distance and pitch constraints
- Optional smoothing/animation system
- Focus and framing utilities

### CameraController (High-Level Controller)
**Responsibility**: Input handling and camera management
- Mouse interaction state machine (orbit/pan/zoom modes)
- Drag threshold detection to prevent accidental movement
- Viewport-aware input processing
- 3D ray casting for mouse picking
- Automatic aspect ratio synchronization
- Camera and viewport lifecycle management

### Viewport (Coordinate System Manager)
**Responsibility**: Screen-space transformations
- Screen to normalized device coordinate conversion
- 3D ray generation from screen coordinates
- World to screen projection
- Viewport bounds management and testing
- Mouse delta calculation for camera controls

## Interface Design

### Camera Base Class
```cpp
class Camera {
public:
    Camera(Events::EventDispatcher* eventDispatcher = nullptr);
    virtual ~Camera() = default;
    
    // Core positioning
    virtual void setPosition(const Math::Vector3f& position);
    virtual void setTarget(const Math::Vector3f& target);
    virtual void setUp(const Math::Vector3f& up);
    
    const Math::Vector3f& getPosition() const { return m_position; }
    const Math::Vector3f& getTarget() const { return m_target; }
    const Math::Vector3f& getUp() const { return m_up; }
    
    // Projection settings
    void setFieldOfView(float fov);
    void setAspectRatio(float aspectRatio);
    void setNearFarPlanes(float nearPlane, float farPlane);
    
    float getFieldOfView() const { return m_fieldOfView; }
    float getAspectRatio() const { return m_aspectRatio; }
    float getNearPlane() const { return m_nearPlane; }
    float getFarPlane() const { return m_farPlane; }
    
    // Matrix access (lazy computation with caching)
    const Math::Matrix4f& getViewMatrix() const;
    const Math::Matrix4f& getProjectionMatrix() const;
    Math::Matrix4f getViewProjectionMatrix() const;
    
    // Direction vectors
    Math::Vector3f getForward() const;
    Math::Vector3f getRight() const;
    Math::Vector3f getActualUp() const;
    
    // Abstract view preset interface
    virtual void setViewPreset(ViewPreset preset) = 0;
    
protected:
    void dispatchCameraChangedEvent(Events::ChangeType changeType);
    
private:
    mutable Math::Matrix4f m_viewMatrix;
    mutable Math::Matrix4f m_projectionMatrix;
    mutable bool m_viewMatrixDirty = true;
    mutable bool m_projectionMatrixDirty = true;
};
```

### OrbitCamera Class
```cpp
class OrbitCamera : public Camera {
public:
    OrbitCamera(Events::EventDispatcher* eventDispatcher = nullptr);
    
    // Orbit controls
    void orbit(float deltaYaw, float deltaPitch);
    void zoom(float delta);
    void pan(const Math::Vector3f& delta);
    
    // Direct spherical coordinate control
    void setDistance(float distance);
    void setYaw(float yaw);
    void setPitch(float pitch);
    void setOrbitAngles(float yaw, float pitch);
    
    float getDistance() const { return m_distance; }
    float getYaw() const { return m_yaw; }
    float getPitch() const { return m_pitch; }
    
    // Constraints
    void setDistanceConstraints(float minDistance, float maxDistance);
    void setPitchConstraints(float minPitch, float maxPitch);
    
    // Sensitivity controls
    void setPanSensitivity(float sensitivity) { m_panSensitivity = sensitivity; }
    void setRotateSensitivity(float sensitivity) { m_rotateSensitivity = sensitivity; }
    void setZoomSensitivity(float sensitivity) { m_zoomSensitivity = sensitivity; }
    
    // Smoothing/animation
    void setSmoothing(bool enabled) { m_smoothingEnabled = enabled; }
    void setSmoothFactor(float factor) { m_smoothFactor = factor; }
    void update(float deltaTime);
    
    // View presets
    void setViewPreset(ViewPreset preset) override;
    
    // Utilities
    void focusOn(const Math::Vector3f& point, float optimalDistance = -1.0f);
    void frameBox(const Math::Vector3f& minBounds, const Math::Vector3f& maxBounds);
    
private:
    void updatePositionFromSpherical();
    void applyConstraints();
};
```

### CameraController Class
```cpp
class CameraController {
public:
    enum class InteractionMode {
        NONE,
        ORBIT,  // Left mouse button
        PAN,    // Middle mouse button
        ZOOM    // Right mouse button
    };
    
    CameraController(Events::EventDispatcher* eventDispatcher = nullptr);
    
    // Component access
    OrbitCamera* getCamera() const { return m_camera.get(); }
    Viewport* getViewport() const { return m_viewport.get(); }
    
    // Viewport configuration
    void setViewportSize(int width, int height);
    void setViewportBounds(int x, int y, int width, int height);
    
    // Mouse interaction
    void onMouseButtonDown(const Math::Vector2i& mousePos, int button);
    void onMouseButtonUp(const Math::Vector2i& mousePos, int button);
    void onMouseMove(const Math::Vector2i& mousePos);
    void onMouseWheel(const Math::Vector2i& mousePos, float delta);
    
    // 3D interaction utilities
    Math::Ray getMouseRay(const Math::Vector2i& mousePos) const;
    Math::Vector2i worldToScreen(const Math::Vector3f& worldPos) const;
    
    // Animation update
    void update(float deltaTime);
    
    // State queries
    bool isInteracting() const { return m_interactionMode != InteractionMode::NONE; }
    InteractionMode getInteractionMode() const { return m_interactionMode; }
    
private:
    static constexpr float DRAG_THRESHOLD = 3.0f;
};
```

## Data Structures

### ViewPreset Enum
```cpp
enum class ViewPreset {
    FRONT = 0,
    BACK = 1,
    LEFT = 2,
    RIGHT = 3,
    TOP = 4,
    BOTTOM = 5,
    ISOMETRIC = 6
};
```

### CameraChangedEvent
```cpp
namespace Events {
    enum class ChangeType {
        POSITION,      // Camera or target moved
        ROTATION,      // Up vector changed
        ZOOM,          // Field of view changed
        VIEW_PRESET    // View preset selected
    };
    
    struct CameraChangedEvent {
        ChangeType changeType;
        // Event is dispatched through EventDispatcher
    };
}
```

### View Preset Data (Internal)
```cpp
// Hardcoded in OrbitCamera implementation
struct PresetData {
    float yaw;      // Rotation around Y axis in degrees
    float pitch;    // Rotation around X axis in degrees
    float distance; // Distance from target
};

// Actual preset values:
// FRONT:      {0.0f, 0.0f, 10.0f}
// BACK:       {180.0f, 0.0f, 10.0f}
// LEFT:       {-90.0f, 0.0f, 10.0f}
// RIGHT:      {90.0f, 0.0f, 10.0f}
// TOP:        {0.0f, 90.0f, 10.0f}
// BOTTOM:     {0.0f, -90.0f, 10.0f}
// ISOMETRIC:  {45.0f, 35.26f, 12.0f}
```

### Default Values and Constraints
```cpp
// Camera defaults
const float DEFAULT_FOV = 45.0f;
const float DEFAULT_ASPECT_RATIO = 16.0f / 9.0f;
const float DEFAULT_NEAR_PLANE = 0.1f;
const float DEFAULT_FAR_PLANE = 1000.0f;

// OrbitCamera defaults
const float DEFAULT_DISTANCE = 10.0f;
const float MIN_DISTANCE = 0.5f;
const float MAX_DISTANCE = 100.0f;
const float MIN_PITCH = -90.0f;  // Prevents gimbal lock
const float MAX_PITCH = 90.0f;

// Sensitivity defaults
const float DEFAULT_PAN_SENSITIVITY = 1.0f;
const float DEFAULT_ROTATE_SENSITIVITY = 1.0f;
const float DEFAULT_ZOOM_SENSITIVITY = 0.1f;

// Smoothing defaults
const float DEFAULT_SMOOTH_FACTOR = 0.1f;
```

## Implementation Details

### Coordinate Systems
```cpp
// World Space: Right-handed, Y-up
// - Positive X: Right
// - Positive Y: Up
// - Positive Z: Forward (out of screen)

// Screen Space: Top-left origin, Y-down
// - Origin: Top-left corner
// - Positive X: Right
// - Positive Y: Down

// View Space: Camera looks down negative Z
// - Camera forward direction: -Z
// - Right vector: +X
// - Up vector: +Y
```

### Spherical Coordinate System (OrbitCamera)
```cpp
// Position calculation from spherical coordinates:
void OrbitCamera::updatePositionFromSpherical() {
    float yawRad = Math::MathUtils::degreesToRadians(m_yaw);
    float pitchRad = Math::MathUtils::degreesToRadians(m_pitch);
    
    // Calculate position relative to target
    float cosPitch = std::cos(pitchRad);
    float x = m_distance * std::sin(yawRad) * cosPitch;
    float y = m_distance * std::sin(pitchRad);
    float z = m_distance * std::cos(yawRad) * cosPitch;
    
    setPosition(m_target + Math::Vector3f(x, y, z));
}
```

### Viewport Transformations
```cpp
class Viewport {
public:
    // Screen to normalized device coordinates [-1, 1]
    Math::Vector2f screenToNormalized(const Math::Vector2i& screenPos) const {
        float nx = (2.0f * (screenPos.x - m_x) / m_width) - 1.0f;
        float ny = 1.0f - (2.0f * (screenPos.y - m_y) / m_height);
        return Math::Vector2f(nx, ny);
    }
    
    // Generate 3D ray from screen position
    Math::Ray screenToWorldRay(const Math::Vector2i& screenPos,
                              const Math::Matrix4f& viewMatrix,
                              const Math::Matrix4f& projectionMatrix) const {
        Math::Vector2f ndc = screenToNormalized(screenPos);
        
        // Near and far points in NDC
        Math::Vector4f nearPoint(ndc.x, ndc.y, -1.0f, 1.0f);
        Math::Vector4f farPoint(ndc.x, ndc.y, 1.0f, 1.0f);
        
        // Transform to world space
        Math::Matrix4f invViewProj = (projectionMatrix * viewMatrix).inverse();
        Math::Vector4f worldNear = invViewProj * nearPoint;
        Math::Vector4f worldFar = invViewProj * farPoint;
        
        // Perspective divide
        worldNear /= worldNear.w;
        worldFar /= worldFar.w;
        
        // Create ray
        Math::Vector3f origin(worldNear.x, worldNear.y, worldNear.z);
        Math::Vector3f direction = Math::Vector3f(worldFar.x, worldFar.y, worldFar.z) - origin;
        direction.normalize();
        
        return Math::Ray(origin, direction);
    }
};
```

### Smoothing System (OrbitCamera)
```cpp
// Simple exponential smoothing implementation
void OrbitCamera::update(float deltaTime) {
    if (!m_smoothingEnabled) return;
    
    // Smooth distance
    if (std::abs(m_targetDistance - m_distance) > 0.001f) {
        m_distance = Math::MathUtils::lerp(m_distance, m_targetDistance, m_smoothFactor);
        m_distanceChanged = true;
    }
    
    // Smooth yaw (handle wrap-around)
    float yawDiff = m_targetYaw - m_yaw;
    if (yawDiff > 180.0f) yawDiff -= 360.0f;
    if (yawDiff < -180.0f) yawDiff += 360.0f;
    if (std::abs(yawDiff) > 0.01f) {
        m_yaw += yawDiff * m_smoothFactor;
        m_yaw = std::fmod(m_yaw + 360.0f, 360.0f);
        m_anglesChanged = true;
    }
    
    // Smooth pitch
    if (std::abs(m_targetPitch - m_pitch) > 0.01f) {
        m_pitch = Math::MathUtils::lerp(m_pitch, m_targetPitch, m_smoothFactor);
        m_anglesChanged = true;
    }
    
    // Smooth target position
    if ((m_targetTarget - m_target).lengthSquared() > 0.0001f) {
        m_target = Math::MathUtils::lerp(m_target, m_targetTarget, m_smoothFactor);
        m_targetChanged = true;
    }
    
    // Update position if anything changed
    if (m_distanceChanged || m_anglesChanged || m_targetChanged) {
        updatePositionFromSpherical();
    }
}
```

## Mouse Interaction State Machine

```cpp
// CameraController manages mouse interaction through a state machine:

void CameraController::onMouseButtonDown(const Math::Vector2i& mousePos, int button) {
    if (!m_viewport->contains(mousePos)) return;
    
    m_mouseDown = true;
    m_lastMousePos = mousePos;
    m_mouseDownPos = mousePos;
    m_hasDragged = false;
    
    // Determine interaction mode based on button
    switch (button) {
        case 0: m_interactionMode = InteractionMode::ORBIT; break;  // Left
        case 1: m_interactionMode = InteractionMode::PAN; break;    // Middle
        case 2: m_interactionMode = InteractionMode::ZOOM; break;   // Right
        default: m_interactionMode = InteractionMode::NONE; break;
    }
}

void CameraController::onMouseMove(const Math::Vector2i& mousePos) {
    if (!m_mouseDown) return;
    
    // Check drag threshold
    if (!m_hasDragged) {
        Math::Vector2f delta = (mousePos - m_mouseDownPos).toVector2f();
        if (delta.length() >= DRAG_THRESHOLD) {
            m_hasDragged = true;
        } else {
            return;
        }
    }
    
    // Calculate normalized mouse delta
    Math::Vector2f mouseDelta = m_viewport->getMouseDelta(mousePos, m_lastMousePos);
    
    // Apply interaction based on mode
    switch (m_interactionMode) {
        case InteractionMode::ORBIT:
            m_camera->orbit(mouseDelta.x * 180.0f, mouseDelta.y * 90.0f);
            break;
            
        case InteractionMode::PAN: {
            float distance = m_camera->getDistance();
            Math::Vector3f panDelta(
                -mouseDelta.x * distance,
                mouseDelta.y * distance,
                0.0f
            );
            m_camera->pan(panDelta);
            break;
        }
        
        case InteractionMode::ZOOM:
            m_camera->zoom(mouseDelta.y * 2.0f);
            break;
    }
    
    m_lastMousePos = mousePos;
}
```

## Input Integration

### Current Implementation
The camera system currently supports mouse-based interaction through the CameraController:

**Mouse Controls:**
- **Left Mouse Drag**: Orbit camera around target
- **Middle Mouse Drag**: Pan camera (move target)
- **Right Mouse Drag**: Zoom in/out
- **Scroll Wheel**: Zoom in/out (with configurable sensitivity)
- **Drag Threshold**: 3 pixels to prevent accidental movement

**Interaction Characteristics:**
- All mouse interactions require the cursor to be within the viewport bounds
- Drag operations use normalized mouse deltas for consistent behavior
- Sensitivity can be configured independently for pan, rotate, and zoom
- Smoothing can be enabled for animated transitions

### Keyboard Integration (Not Yet Implemented)
The current implementation does not include keyboard shortcuts. These would need to be added:
- Number keys for view preset selection
- Reset camera functionality
- Constrained movement modifiers

### Touch/Mobile Support (Not Implemented)
No touch gesture support is currently implemented.

### VR Support (Not Implemented)
No VR-specific camera controls are implemented.

## Performance Considerations

### Matrix Caching
The implementation uses dirty flags to avoid redundant matrix calculations:
```cpp
// Only recalculate when needed
const Math::Matrix4f& Camera::getViewMatrix() const {
    if (m_viewMatrixDirty) {
        m_viewMatrix = Math::Matrix4f::lookAt(m_position, m_target, m_up);
        m_viewMatrixDirty = false;
    }
    return m_viewMatrix;
}
```

### Header-Only Design
- All classes are header-only for optimal inlining
- No virtual function overhead except for view preset selection
- Minimal dynamic allocations

### Smoothing System
- Simple exponential decay avoids complex calculations
- Early termination when values are close enough
- Optional system - can be disabled for immediate response

### Constraint Application
- Constraints checked only when values change
- Simple min/max clamping for efficient validation
- No complex boundary calculations

## Events

The camera system dispatches events through the EventDispatcher when camera state changes:

### CameraChangedEvent
```cpp
namespace Events {
    enum class ChangeType {
        POSITION,      // Camera position or target changed
        ROTATION,      // Camera up vector changed
        ZOOM,          // Field of view changed
        VIEW_PRESET    // View preset selected
    };
    
    struct CameraChangedEvent {
        ChangeType changeType;
    };
}
```

**Event Dispatch Points:**
- `setPosition()` - Dispatches POSITION event
- `setTarget()` - Dispatches POSITION event
- `setUp()` - Dispatches ROTATION event
- `setFieldOfView()` - Dispatches ZOOM event
- `setViewPreset()` - Dispatches VIEW_PRESET event

**Note:** Events are dispatched immediately on state change. There is no batching or deferred dispatch mechanism.

## Testing Coverage

The camera subsystem has comprehensive test coverage with ~2000 lines of tests:

### Unit Tests
**Well Covered:**
- Matrix calculation accuracy and caching behavior
- Spherical coordinate conversions
- Constraint enforcement (distance and pitch limits)
- View preset positioning
- Event dispatch verification
- Coordinate transformations (screen ↔ world)
- Mouse interaction state machine
- Drag threshold detection

**Test Files:**
- `test_Camera.cpp` - Base class functionality
- `test_OrbitCamera.cpp` - Orbit camera mechanics
- `test_CameraController.cpp` - Input handling
- `test_Viewport.cpp` - Coordinate transformations
- `test_OrbitCamera_transformations.cpp` - Advanced math validation
- `test_zoom_functionality.cpp` - Zoom behavior
- `test_setDistance.cpp` - Distance constraints

### Integration Tests
Currently limited - most testing is unit-level. Integration with rendering system is not directly tested.

### Missing Test Coverage
- Performance benchmarks
- Memory usage validation
- Error recovery scenarios
- Rendering integration
- Multi-threaded access

## Dependencies

### Direct Dependencies
- **Foundation/Math**: 
  - Vector2f, Vector2i, Vector3f - Position and direction vectors
  - Matrix4f - View and projection matrices
  - Ray - 3D ray for mouse picking
  - MathUtils - Degree/radian conversion, interpolation
  
- **Foundation/Events**:
  - EventDispatcher - Event notification system
  - CameraChangedEvent - State change notifications
  
- **Foundation/Logging**:
  - Logger - Debug output (used sparingly)

### Dependency Characteristics
- All dependencies are header-only
- No external library dependencies
- No file I/O dependencies
- No configuration system integration (settings are hardcoded)

## Platform Considerations

### Current Implementation
The camera system is platform-agnostic and relies on normalized input:
- Mouse positions are provided in screen coordinates
- All calculations use floating-point for precision
- No platform-specific code or optimizations

### Desktop (Implemented)
- Full mouse interaction support
- Configurable sensitivity settings
- Viewport-aware input handling
- Sub-pixel precision for smooth movement

### Mobile/Touch (Not Implemented)
Would require:
- Touch gesture recognition layer
- Pinch-to-zoom handling
- Touch velocity tracking
- Modified sensitivity curves

### VR (Not Implemented)
Would require:
- 6DOF tracking integration
- Separate head/hand tracking
- Comfort options
- Different interaction paradigm

## File I/O Integration

### Current Status
**Not Implemented** - The camera system has no serialization support.

### Required for Implementation
To add persistence support, the following would be needed:

1. **State Serialization:**
   - Camera position, target, up vectors
   - Spherical coordinates (yaw, pitch, distance)
   - Projection settings (FOV, near/far planes)
   - Current view preset
   - Sensitivity settings

2. **Integration Points:**
   - Project save/load system
   - Settings/preferences system
   - Import/export functionality

3. **Version Compatibility:**
   - Version markers for camera data
   - Default value handling
   - Migration strategies

## Known Issues and Technical Debt

### Issue 1: Missing Orthographic Projection Support
- **Severity**: High
- **Impact**: Only perspective projection is available, limiting CAD-style editing capabilities
- **Current State**: Camera base class only generates perspective projection matrices
- **Proposed Solution**: Add projection type enum and conditional matrix generation
- **Effort**: Medium - requires matrix generation changes and testing

### Issue 2: No State Serialization
- **Severity**: High
- **Impact**: Camera state is lost when closing/reopening projects
- **Current State**: No serialization methods implemented
- **Proposed Solution**: Add serialize/deserialize methods to Camera and OrbitCamera
- **Effort**: Medium - requires file I/O integration

### Issue 3: Limited Animation System  
- **Severity**: Medium
- **Impact**: Only basic exponential smoothing, no advanced transitions or easing
- **Current State**: Simple lerp-based smoothing in OrbitCamera::update()
- **Proposed Solution**: Create dedicated animation system with easing functions
- **Effort**: High - requires new component design

### Issue 4: Hardcoded View Presets
- **Severity**: Medium
- **Impact**: Cannot add custom view presets or modify existing ones
- **Current State**: 7 presets hardcoded in static array
- **Proposed Solution**: Create ViewPresets manager class
- **Effort**: Medium - requires API changes

### Issue 5: No Keyboard Support
- **Severity**: Medium  
- **Impact**: No keyboard shortcuts for view selection or camera control
- **Current State**: Only mouse input is handled
- **Proposed Solution**: Add keyboard handler to CameraController
- **Effort**: Low - straightforward to implement

### Issue 6: Event System Coupling
- **Severity**: Low
- **Impact**: Harder to unit test, requires EventDispatcher in all tests
- **Current State**: Direct dependency on EventDispatcher
- **Proposed Solution**: Use optional callback or interface pattern
- **Effort**: Low - mostly refactoring

### Issue 7: No Bounds Constraints
- **Severity**: Low
- **Impact**: Camera can move anywhere without limits
- **Current State**: Only distance and pitch constraints implemented
- **Proposed Solution**: Add optional bounding box constraints
- **Effort**: Low - simple bounds checking

## Future Enhancements

### Short Term
1. Add orthographic projection support
2. Implement basic state serialization  
3. Add keyboard shortcut support
4. Improve error handling and validation

### Medium Term
1. Create proper animation system with easing
2. Add custom view preset management
3. Implement camera paths/flythrough support
4. Add multi-viewport camera synchronization

### Long Term  
1. Add touch gesture support for mobile
2. Implement VR camera modes
3. Create visual debugging tools
4. Add advanced constraint systems

## Recent Improvements (2025-06-16)

### Code Quality Fixes
1. **Fixed Perspective Divide in Ray Generation**: 
   - `Viewport::screenToWorldRay` now properly handles homogeneous coordinates
   - Uses Vector4f for correct perspective divide instead of Vector3f
   - Ensures accurate ray casting for mouse interaction

2. **Fixed World-to-Screen Projection**:
   - `Viewport::worldToScreen` now uses proper homogeneous coordinates
   - Correctly divides by w component for perspective projection
   - Improves accuracy of screen coordinate calculations

3. **Eliminated Magic Numbers**:
   - Added named constants in CameraController for drag threshold, scales
   - Added REFERENCE_SIZE constant in Viewport for zoom normalization
   - Improves code maintainability and clarity

4. **Fixed Frame-Rate Dependent Smoothing**:
   - Removed hardcoded 60 FPS reference in smoothing calculations
   - Now properly frame-rate independent using exponential decay
   - Ensures consistent behavior across different frame rates

### Test Status
All 108 tests continue to pass after these improvements, validating that the fixes maintain backward compatibility while improving correctness.