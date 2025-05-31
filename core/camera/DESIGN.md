# Camera System Subsystem

## Purpose
Manages 3D camera controls, view presets, smooth transitions, and camera state persistence for the voxel editor across all platforms.

## Key Components

### CameraManager
**Responsibility**: Main interface for camera operations
- Coordinate camera movement and rotation
- Manage view presets and transitions
- Handle camera state persistence
- Provide camera matrices for rendering

### OrbitCamera
**Responsibility**: Core camera implementation
- Orbit-style camera controls
- Target-based rotation and zoom
- Constraint handling (zoom limits, bounds)
- Smooth interpolation for movements

### ViewPresets
**Responsibility**: Predefined camera positions
- Standard orthographic views (front, back, top, etc.)
- Isometric and perspective views
- Custom user-defined presets
- View transition calculations

### CameraAnimator
**Responsibility**: Smooth camera transitions
- Interpolated camera movements
- Easing functions for natural motion
- Concurrent animation management
- Animation cancellation and blending

## Interface Design

```cpp
class CameraManager {
public:
    // View management
    void setView(ViewType view);
    void setCustomView(const Vector3f& position, const Vector3f& target, const Vector3f& up);
    ViewType getCurrentView() const;
    
    // Camera movement
    void orbit(float deltaAzimuth, float deltaElevation);
    void pan(const Vector2f& delta);
    void zoom(float factor);
    void dolly(float distance);
    
    // Target management
    void setTarget(const Vector3f& target);
    Vector3f getTarget() const;
    void frameSelection(const BoundingBox& bounds);
    void frameAll(const BoundingBox& worldBounds);
    
    // Animation control
    void setAnimationEnabled(bool enabled);
    void setAnimationDuration(float seconds);
    bool isAnimating() const;
    void stopAnimation();
    
    // Camera properties
    void setProjection(ProjectionType type);
    void setFieldOfView(float fovDegrees);
    void setNearFar(float nearPlane, float farPlane);
    void setViewportSize(int width, int height);
    
    // Matrix access
    Matrix4f getViewMatrix() const;
    Matrix4f getProjectionMatrix() const;
    Matrix4f getViewProjectionMatrix() const;
    
    // State management
    void reset();
    CameraState getState() const;
    void setState(const CameraState& state);
    
    // Constraints
    void setZoomLimits(float minZoom, float maxZoom);
    void setOrbitLimits(float minElevation, float maxElevation);
    void setBounds(const BoundingBox& bounds);
    
private:
    std::unique_ptr<OrbitCamera> m_camera;
    std::unique_ptr<ViewPresets> m_presets;
    std::unique_ptr<CameraAnimator> m_animator;
    EventDispatcher* m_eventDispatcher;
};
```

## Data Structures

### ViewType
```cpp
enum class ViewType {
    Custom,
    Front,
    Back,
    Left,
    Right,
    Top,
    Bottom,
    Isometric,
    Perspective
};
```

### ProjectionType
```cpp
enum class ProjectionType {
    Perspective,
    Orthographic
};
```

### CameraState
```cpp
struct CameraState {
    Vector3f position;
    Vector3f target;
    Vector3f up;
    float distance;
    float azimuth;          // Rotation around Y axis
    float elevation;        // Rotation around X axis
    ProjectionType projectionType;
    float fieldOfView;
    float nearPlane;
    float farPlane;
    ViewType currentView;
    
    bool operator==(const CameraState& other) const;
    void serialize(BinaryWriter& writer) const;
    void deserialize(BinaryReader& reader);
};
```

### CameraConstraints
```cpp
struct CameraConstraints {
    float minZoom = 0.1f;
    float maxZoom = 100.0f;
    float minElevation = -89.0f;  // degrees
    float maxElevation = 89.0f;   // degrees
    std::optional<BoundingBox> bounds;
    bool constrainTarget = false;
    
    void applyToState(CameraState& state) const;
};
```

## Camera Implementation

### OrbitCamera
```cpp
class OrbitCamera {
public:
    OrbitCamera();
    
    // Core operations
    void orbit(float deltaAzimuth, float deltaElevation);
    void pan(const Vector2f& screenDelta, const Matrix4f& projectionMatrix, const Vector2i& viewportSize);
    void zoom(float factor);
    void dolly(float distance);
    
    // Position calculation
    Vector3f calculatePosition() const;
    Matrix4f calculateViewMatrix() const;
    
    // State access
    void setState(const CameraState& state);
    CameraState getState() const;
    
    // Constraints
    void setConstraints(const CameraConstraints& constraints);
    void applyConstraints();
    
    // Utility functions
    void lookAt(const Vector3f& position, const Vector3f& target, const Vector3f& up);
    void frameBox(const BoundingBox& box, float margin = 0.1f);
    
private:
    Vector3f m_target;
    Vector3f m_up;
    float m_distance;
    float m_azimuth;
    float m_elevation;
    float m_fieldOfView;
    float m_nearPlane;
    float m_farPlane;
    ProjectionType m_projectionType;
    CameraConstraints m_constraints;
    
    void updateUpVector();
    Vector3f sphericalToCartesian(float azimuth, float elevation, float radius) const;
};
```

### ViewPresets
```cpp
class ViewPresets {
public:
    CameraState getPreset(ViewType view) const;
    void setCustomPreset(const std::string& name, const CameraState& state);
    std::vector<std::string> getCustomPresetNames() const;
    
    // Calculate view transformations
    CameraState calculateFrontView(const BoundingBox& bounds) const;
    CameraState calculateBackView(const BoundingBox& bounds) const;
    CameraState calculateTopView(const BoundingBox& bounds) const;
    CameraState calculateBottomView(const BoundingBox& bounds) const;
    CameraState calculateLeftView(const BoundingBox& bounds) const;
    CameraState calculateRightView(const BoundingBox& bounds) const;
    CameraState calculateIsometricView(const BoundingBox& bounds) const;
    
private:
    std::unordered_map<std::string, CameraState> m_customPresets;
    BoundingBox m_sceneBounds;
    
    CameraState createOrthographicView(const Vector3f& direction, const Vector3f& up, const BoundingBox& bounds) const;
    float calculateOptimalDistance(const BoundingBox& bounds, float fieldOfView) const;
};
```

### CameraAnimator
```cpp
class CameraAnimator {
public:
    void animateTo(const CameraState& targetState, float duration, EasingFunction easing = EasingFunction::EaseInOut);
    void animateOrbit(float deltaAzimuth, float deltaElevation, float duration);
    void animateZoom(float targetDistance, float duration);
    void animatePan(const Vector3f& targetPosition, float duration);
    
    bool isAnimating() const;
    void stop();
    void update(float deltaTime);
    
    CameraState getCurrentState() const;
    
private:
    struct Animation {
        CameraState startState;
        CameraState targetState;
        float duration;
        float elapsedTime;
        EasingFunction easing;
        bool active;
    };
    
    Animation m_currentAnimation;
    
    CameraState interpolateStates(const CameraState& start, const CameraState& end, float t, EasingFunction easing) const;
    float applyEasing(float t, EasingFunction easing) const;
};
```

## Easing Functions

```cpp
enum class EasingFunction {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    EaseInBack,
    EaseOutBack,
    EaseInOutBack
};

class EasingFunctions {
public:
    static float apply(float t, EasingFunction function);
    
private:
    static float easeInQuad(float t);
    static float easeOutQuad(float t);
    static float easeInOutQuad(float t);
    static float easeInBack(float t);
    static float easeOutBack(float t);
    static float easeInOutBack(float t);
};
```

## Input Integration

### Mouse Controls
- **Left Drag**: Orbit around target
- **Right Drag**: Pan camera
- **Middle Drag**: Alternative pan
- **Scroll Wheel**: Zoom in/out
- **Double Click**: Frame selection or auto-focus

### Keyboard Shortcuts
- **Number Keys 1-7**: Switch to preset views
- **F**: Frame selection
- **Home**: Reset camera
- **Ctrl + Mouse**: Constrained movement

### Touch Controls (Qt)
- **Single Touch Drag**: Orbit
- **Two Finger Drag**: Pan
- **Pinch**: Zoom
- **Double Tap**: Frame selection

### VR Controls
- **Head Movement**: Natural camera movement
- **Hand Gestures**: Point to focus target
- **Voice Commands**: "Front view", "Reset camera"

## Performance Considerations

### Matrix Caching
- Cache view and projection matrices
- Invalidate only when camera changes
- Avoid unnecessary matrix calculations
- Use dirty flags for optimization

### Animation Optimization
- Skip interpolation for very small changes
- Use fixed timestep for consistent animation
- Batch multiple small movements
- Early termination for completed animations

### Constraint Validation
- Lazy constraint application
- Incremental constraint checking
- Optimized bounds testing
- Minimal state copying

## Events

### CameraChanged Event
```cpp
struct CameraChangedEvent {
    CameraState oldState;
    CameraState newState;
    ChangeType changeType; // POSITION, ROTATION, ZOOM, VIEW_PRESET
};
```

### ViewChanged Event
```cpp
struct ViewChangedEvent {
    ViewType oldView;
    ViewType newView;
    bool animated;
};
```

## Testing Strategy

### Unit Tests
- Matrix calculation accuracy
- Constraint enforcement
- Animation interpolation
- State serialization
- View preset calculations

### Integration Tests
- Input handling integration
- Rendering system integration
- State persistence
- Cross-platform behavior

### Visual Tests
- Smooth animation playback
- Correct view transitions
- Constraint visualization
- Matrix accuracy validation

### Performance Tests
- Animation frame rate consistency
- Matrix calculation performance
- Memory usage validation
- Large scene handling

## Dependencies
- **Foundation/Math**: Matrix and vector operations
- **Foundation/Events**: Camera change notifications
- **Core/Input**: Input event processing
- **Foundation/Config**: Camera settings and preferences

## Platform Considerations

### Desktop
- Mouse and keyboard input
- Multiple monitor support
- High precision input handling
- Customizable shortcuts

### Mobile/Touch
- Touch gesture recognition
- Accelerometer integration
- Battery-conscious animations
- Adaptive sensitivity

### VR
- Head tracking integration
- Hand gesture recognition
- Comfort settings (motion sickness)
- Spatial audio synchronization

## File I/O Integration
- Save camera state with projects
- Export camera paths for animation
- Import camera presets
- Version compatibility for camera data