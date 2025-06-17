# Selection System Subsystem

## Purpose
Manages voxel selection operations, multi-selection handling, selection persistence, and provides visual feedback for selected elements.

## Current Implementation Status
The selection system is fully implemented with all major components:

### Core Components
- **SelectionTypes** - All data structures, enums, and type definitions
- **SelectionSet** - Efficient voxel storage with set operations and statistics
- **SelectionManager** - Main interface with history, named sets, and event integration
- **SelectionRenderer** - Visual rendering with outline, fill, and animation support

### Selection Algorithms
- **BoxSelector** - Box/rectangle selection from screen, world, or grid coordinates
- **SphereSelector** - Sphere, ellipsoid, and hemisphere selection algorithms
- **FloodFillSelector** - Connected component selection with various connectivity modes

### Features
- Multiple selection modes (Replace, Add, Subtract, Intersect)
- Selection history with undo/redo support
- Named selection sets for saving/loading selections
- Preview mode for visualizing selections before applying
- Visual feedback with customizable styles and animations
- Event system integration for selection change notifications

## Key Components

### SelectionManager
**Responsibility**: Main interface for all selection operations
- Single and multi-voxel selection with various modes
- Selection state management with preview support
- History tracking with configurable undo/redo stack
- Named selection sets for saving/loading commonly used selections
- Event notifications for selection changes
- Integration with VoxelDataManager for validation
- Configurable selection styles and visual properties

### SelectionSet
**Responsibility**: Efficient storage and manipulation of selected voxels
- Unordered set-based storage for O(1) lookups
- Set operations (union, intersection, subtract, symmetric difference)
- Spatial queries (bounds, center, statistics)
- Filtering with custom predicates
- Iterator support for range-based loops
- Serialization/deserialization for persistence
- Cached bounds and center for performance

### SelectionRenderer
**Responsibility**: Visual representation of selections
- Multiple render modes (outline, fill, outline+fill, highlight)
- Animated selection effects with configurable speed
- Wireframe bounds visualization
- Selection statistics display
- Gizmo rendering for selection manipulation
- Shader-based rendering for performance
- Support for preview selections with different styling

### BoxSelector
**Responsibility**: Box-based selection algorithms
- Screen-space box selection (2D selection rectangle)
- World-space box selection (3D bounding box)
- Grid-space box selection (voxel grid coordinates)
- Ray-based box selection (corner-to-corner rays)
- Configurable partial inclusion (voxels partially in box)
- Integration with viewport and camera transforms

### SphereSelector
**Responsibility**: Sphere and ellipsoid selection algorithms
- Sphere selection from center and radius
- Ellipsoid selection with rotation support
- Hemisphere selection with directional normal
- Ray-based sphere selection (place sphere at ray intersection)
- Configurable partial inclusion for boundary voxels
- Optional falloff for gradient selection weights
- Support for all voxel resolutions

### FloodFillSelector
**Responsibility**: Connected component selection
- Multiple connectivity modes (6-face, 18-edge, 26-vertex)
- Custom predicate-based flood fill
- Distance-limited flood fill (maximum steps)
- Bounds-constrained flood fill
- Planar flood fill (2D selection on 3D plane)
- Maximum voxel limit for performance
- Support for diagonal connectivity

## Interface Design

### SelectionManager Interface
```cpp
class SelectionManager {
public:
    // Basic selection operations
    void selectVoxel(const VoxelId& voxel);
    void deselectVoxel(const VoxelId& voxel);
    void toggleVoxel(const VoxelId& voxel);
    
    // Multi-selection operations
    void selectAll();
    void selectNone();
    void selectInverse();
    void selectByResolution(VoxelResolution resolution);
    
    // Region selection
    void selectBox(const BoundingBox& box, VoxelResolution resolution);
    void selectSphere(const Vector3f& center, float radius, VoxelResolution resolution);
    void selectCylinder(const Vector3f& base, const Vector3f& direction, 
                       float radius, float height, VoxelResolution resolution);
    void selectFloodFill(const VoxelId& seed, FloodFillCriteria criteria);
    
    // Selection with mode
    void select(const SelectionSet& selection, SelectionMode mode);
    void selectVoxel(const VoxelId& voxel, SelectionMode mode);
    void selectRegion(const SelectionRegion& region, VoxelResolution resolution, SelectionMode mode);
    
    // Selection queries
    bool isSelected(const VoxelId& voxel) const;
    const SelectionSet& getSelection() const;
    SelectionSet getSelectionCopy() const;
    size_t getSelectionSize() const;
    bool hasSelection() const;
    BoundingBox getSelectionBounds() const;
    SelectionStats getSelectionStats() const;
    
    // Selection history
    void pushSelectionToHistory();
    bool canUndo() const;
    bool canRedo() const;
    void undoSelection();
    void redoSelection();
    void clearHistory();
    void setMaxHistorySize(size_t size);
    
    // Selection sets (named selections)
    void saveSelectionSet(const std::string& name);
    bool loadSelectionSet(const std::string& name);
    std::vector<std::string> getSelectionSetNames() const;
    void deleteSelectionSet(const std::string& name);
    bool hasSelectionSet(const std::string& name) const;
    void clearSelectionSets();
    
    // Set operations
    void unionWith(const SelectionSet& other);
    void intersectWith(const SelectionSet& other);
    void subtractFrom(const SelectionSet& other);
    
    // Filtering
    void filterSelection(const SelectionPredicate& predicate);
    SelectionSet getFilteredSelection(const SelectionPredicate& predicate) const;
    
    // Validation
    void validateSelection();
    bool isValidSelection() const;
    
    // Preview mode
    void setPreviewMode(bool enabled);
    bool isPreviewMode() const;
    void applyPreview();
    void cancelPreview();
    const SelectionSet& getPreviewSelection() const;
    
    // Configuration
    void setSelectionStyle(const SelectionStyle& style);
    const SelectionStyle& getSelectionStyle() const;
};
```

## Data Structures

### VoxelId
```cpp
struct VoxelId {
    Vector3i position;
    VoxelResolution resolution;
    
    bool operator==(const VoxelId& other) const;
    bool operator!=(const VoxelId& other) const;
    bool operator<(const VoxelId& other) const;
    size_t hash() const;
    
    // Utility methods
    Vector3f getWorldPosition() const;
    float getVoxelSize() const;
    BoundingBox getBounds() const;
};

// Hash specialization for unordered containers
namespace std {
    template<>
    struct hash<VoxelId> {
        size_t operator()(const VoxelId& id) const {
            return id.hash();
        }
    };
}
```

### SelectionSet
```cpp
class SelectionSet {
public:
    // Basic operations
    void add(const VoxelId& voxel);
    void remove(const VoxelId& voxel);
    bool contains(const VoxelId& voxel) const;
    void clear();
    
    // Bulk operations
    void addRange(const std::vector<VoxelId>& voxels);
    void removeRange(const std::vector<VoxelId>& voxels);
    void addSet(const SelectionSet& other);
    void removeSet(const SelectionSet& other);
    
    // Set operations (return new set)
    SelectionSet unionWith(const SelectionSet& other) const;
    SelectionSet intersectWith(const SelectionSet& other) const;
    SelectionSet subtract(const SelectionSet& other) const;
    SelectionSet symmetricDifference(const SelectionSet& other) const;
    
    // In-place set operations
    void unite(const SelectionSet& other);
    void intersect(const SelectionSet& other);
    void subtractFrom(const SelectionSet& other);
    
    // Queries
    size_t size() const;
    bool empty() const;
    std::vector<VoxelId> toVector() const;
    BoundingBox getBounds() const;
    Vector3f getCenter() const;
    SelectionStats getStats() const;
    
    // Filtering
    SelectionSet filter(const SelectionPredicate& predicate) const;
    void filterInPlace(const SelectionPredicate& predicate);
    
    // Iteration support
    using iterator = std::unordered_set<VoxelId>::iterator;
    using const_iterator = std::unordered_set<VoxelId>::const_iterator;
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    
    // Visitor pattern
    void forEach(const SelectionVisitor& visitor) const;
    
    // Comparison
    bool operator==(const SelectionSet& other) const;
    bool operator!=(const SelectionSet& other) const;
    
    // Serialization
    void serialize(BinaryWriter& writer) const;
    void deserialize(BinaryReader& reader);
    
private:
    std::unordered_set<VoxelId> m_voxels;
    mutable std::optional<BoundingBox> m_cachedBounds;
    mutable std::optional<Vector3f> m_cachedCenter;
};
```

### Enumerations
```cpp
enum class SelectionMode {
    Replace,    // Replace current selection
    Add,        // Add to current selection
    Subtract,   // Remove from current selection
    Intersect   // Intersect with current selection
};

enum class SelectionChangeType {
    Added,
    Removed,
    Replaced,
    Cleared,
    Modified
};

enum class SelectionOperationType {
    Move,
    Copy,
    Delete,
    Transform,
    Duplicate
};

enum class FloodFillCriteria {
    Connected6,         // 6-connected voxels (face neighbors)
    Connected18,        // 18-connected voxels (face + edge neighbors)  
    Connected26,        // 26-connected voxels (all neighbors)
    SameResolution,     // Same resolution level
    ConnectedSameRes    // Connected + same resolution
};
```

### Supporting Structures
```cpp
// Selection statistics
struct SelectionStats {
    size_t voxelCount = 0;
    size_t groupCount = 0;
    std::unordered_map<VoxelResolution, size_t> countByResolution;
    BoundingBox bounds;
    Vector3f center;
    float totalVolume = 0.0f;
};

// Selection visual style
struct SelectionStyle {
    Color outlineColor = Color(0.0f, 1.0f, 0.0f, 1.0f);  // Green
    Color fillColor = Color(0.0f, 1.0f, 0.0f, 0.2f);     // Semi-transparent
    float outlineThickness = 2.0f;
    bool animated = true;
    float animationSpeed = 1.0f;
    bool showBounds = true;
    bool showCount = true;
};

// Selection context for operations
struct SelectionContext {
    SelectionMode mode = SelectionMode::Replace;
    bool continuous = false;              // For drag selection
    bool preview = false;                 // Show preview before applying
    std::optional<SelectionRegion> region;
    std::optional<SelectionPredicate> filter;
};

// Selection region definition
struct SelectionRegion {
    enum Type { Box, Sphere, Cylinder, Cone, Custom } type;
    BoundingBox box;
    Vector3f center;
    float radius;
    float height;
    Vector3f direction;
};

// Function pointer types
using SelectionPredicate = std::function<bool(const VoxelId&)>;
using SelectionVisitor = std::function<void(const VoxelId&)>;
```

## Selection Algorithms

### BoxSelector Interface
```cpp
class BoxSelector {
public:
    // Box selection from screen coordinates
    SelectionSet selectFromScreen(const Vector2i& screenStart, const Vector2i& screenEnd,
                                const Matrix4f& viewMatrix, const Matrix4f& projMatrix,
                                const Vector2i& viewportSize, VoxelResolution resolution);
    
    // Box selection from world coordinates
    SelectionSet selectFromWorld(const BoundingBox& worldBox, VoxelResolution resolution,
                               bool checkExistence = true);
    
    // Box selection from two rays (corner to corner)
    SelectionSet selectFromRays(const Ray& startRay, const Ray& endRay,
                              float maxDistance, VoxelResolution resolution);
    
    // Box selection from grid coordinates
    SelectionSet selectFromGrid(const Vector3i& minGrid, const Vector3i& maxGrid,
                              VoxelResolution resolution, bool checkExistence = true);
    
    // Configuration
    void setSelectionMode(SelectionMode mode);
    void setIncludePartial(bool include);
};
```

### SphereSelector Interface
```cpp
class SphereSelector {
public:
    // Sphere selection from center and radius
    SelectionSet selectFromSphere(const Vector3f& center, float radius,
                                VoxelResolution resolution, bool checkExistence = true);
    
    // Sphere selection from ray (select sphere at intersection point)
    SelectionSet selectFromRay(const Ray& ray, float radius,
                             float maxDistance, VoxelResolution resolution);
    
    // Ellipsoid selection
    SelectionSet selectEllipsoid(const Vector3f& center, const Vector3f& radii,
                               const Quaternion& rotation, VoxelResolution resolution,
                               bool checkExistence = true);
    
    // Hemisphere selection
    SelectionSet selectHemisphere(const Vector3f& center, float radius,
                                const Vector3f& normal, VoxelResolution resolution,
                                bool checkExistence = true);
    
    // Configuration
    void setSelectionMode(SelectionMode mode);
    void setIncludePartial(bool include);
    void setFalloff(bool enabled, float start = 0.8f);
};
```

### FloodFillSelector Interface
```cpp
class FloodFillSelector {
public:
    // Basic flood fill from seed voxel
    SelectionSet selectFloodFill(const VoxelId& seed, 
                               FloodFillCriteria criteria = FloodFillCriteria::Connected6);
    
    // Flood fill with custom predicate
    SelectionSet selectFloodFillCustom(const VoxelId& seed,
                                     const SelectionPredicate& predicate);
    
    // Flood fill with maximum distance/steps
    SelectionSet selectFloodFillLimited(const VoxelId& seed,
                                      FloodFillCriteria criteria, int maxSteps);
    
    // Flood fill within bounds
    SelectionSet selectFloodFillBounded(const VoxelId& seed,
                                      FloodFillCriteria criteria,
                                      const BoundingBox& bounds);
    
    // Planar flood fill (2D flood fill on a plane)
    SelectionSet selectPlanarFloodFill(const VoxelId& seed,
                                     const Vector3f& planeNormal,
                                     float planeTolerance = 0.01f);
    
    // Configuration
    void setMaxVoxels(size_t max);
    void setConnectivityMode(ConnectivityMode mode);
    
    enum class ConnectivityMode {
        Face6,      // 6-connectivity (faces only)
        Edge18,     // 18-connectivity (faces + edges)
        Vertex26    // 26-connectivity (faces + edges + vertices)
    };
};
```

## Visual Feedback

### SelectionRenderer Interface
```cpp
class SelectionRenderer {
public:
    // Initialize/shutdown
    bool initialize();
    void shutdown();
    
    // Main render methods
    void render(const Camera* camera, float deltaTime);
    void renderPreview(const SelectionSet& preview, const Camera* camera);
    
    // Render specific selection shapes
    void renderBox(const BoundingBox& box, const Color& color, float thickness = 1.0f);
    void renderSphere(const Vector3f& center, float radius, const Color& color);
    void renderCylinder(const Vector3f& base, const Vector3f& direction, 
                       float radius, float height, const Color& color);
    
    // Configuration
    void setStyle(const SelectionStyle& style);
    void setRenderMode(SelectionRenderMode mode);
    void setShowGizmos(bool show);
    
    enum class SelectionRenderMode {
        Outline,        // Wireframe outline only
        Fill,          // Semi-transparent fill only
        OutlineAndFill, // Both outline and fill
        Highlight      // Animated highlight effect
    };
};
```

### Visual Features
- **Render Modes**: 
  - Outline mode for wireframe visualization
  - Fill mode for semi-transparent volume
  - Combined mode for both effects
  - Highlight mode with animation
  
- **Animated Effects**:
  - Pulsing/breathing animation for active selections
  - Configurable animation speed
  - Color interpolation for smooth transitions
  
- **Selection Indicators**:
  - Wireframe bounding box around entire selection
  - Individual voxel highlighting
  - Selection statistics overlay (count, volume)
  - Preview selections with distinct styling
  
- **Performance Optimizations**:
  - Instanced rendering for multiple voxels
  - Cached geometry for static selections
  - LOD system for distant selections
  - Frustum culling for off-screen voxels

## Selection Operations

Selection operations (Move, Copy, Delete, Transform) are not implemented as separate command classes in the current design. These operations are expected to be implemented in the undo/redo system or as part of higher-level voxel manipulation commands.

The SelectionManager provides the selection data and change notifications needed for these operations through:
- `getSelection()` - Get current selection for operations
- `SelectionChangedEvent` - Notify when selection changes
- History support for undoing selection changes
- Integration with VoxelDataManager for validation

## Performance Optimization

### Spatial Indexing
- Octree-based spatial indexing for fast region queries
- Cached bounding boxes for selections
- Incremental updates for selection changes
- Level-of-detail for large selections

### Memory Management
- Bit vectors for dense selections
- Sparse storage for scattered selections
- Memory pooling for temporary selections
- Automatic optimization based on selection density

### Rendering Optimization
- Instanced rendering for multiple selections
- Frustum culling for off-screen selections
- Level-of-detail for distant selections
- Batched rendering updates

## Events

### SelectionChangedEvent
```cpp
struct SelectionChangedEvent : public Event<SelectionChangedEvent> {
    SelectionSet oldSelection;
    SelectionSet newSelection;
    SelectionChangeType changeType;
    
    SelectionChangedEvent(const SelectionSet& oldSel, const SelectionSet& newSel, 
                         SelectionChangeType type);
    const char* getEventType() const override { return "SelectionChangedEvent"; }
};
```

### SelectionOperationEvent
```cpp
struct SelectionOperationEvent : public Event<SelectionOperationEvent> {
    SelectionOperationType operationType;
    SelectionSet affectedVoxels;
    bool success;
    
    SelectionOperationEvent(SelectionOperationType type, const SelectionSet& voxels, 
                           bool result);
    const char* getEventType() const override { return "SelectionOperationEvent"; }
};
```

Events are dispatched through the EventDispatcher when:
- Selection is modified (add/remove/clear)
- Selection mode changes affect the selection
- Named selection sets are loaded
- Preview selections are applied
- Selection validation removes invalid voxels

## Testing Strategy

### Unit Tests
- Selection set operations
- Selection algorithms accuracy
- History management
- Serialization/deserialization
- Performance with large selections

### Integration Tests
- Voxel data manager integration
- Visual feedback rendering
- Group system integration
- Undo/redo system integration

### Visual Tests
- Selection highlighting accuracy
- Multi-selection visual clarity
- Animation smoothness
- Rendering performance

### Performance Tests
- Large selection handling
- Memory usage optimization
- Rendering performance
- Spatial query efficiency

## Dependencies
- **Core/VoxelData**: Access to voxel data for validation
- **Core/Rendering**: Visual feedback rendering
- **Foundation/Math**: Geometric calculations and transformations
- **Foundation/Events**: Selection change notifications
- **Core/UndoRedo**: Command pattern integration

## Input Integration
- Mouse selection (click, drag, box select)
- Keyboard modifiers (Ctrl for add, Shift for range)
- Touch selection for mobile interfaces
- VR hand tracking selection

## Current Implementation Notes

### Fully Implemented Features
- ✅ Complete SelectionManager with history and named sets
- ✅ SelectionSet with all set operations and statistics
- ✅ Box, Sphere, and Flood Fill selection algorithms
- ✅ SelectionRenderer with multiple render modes
- ✅ Event system integration with proper event types
- ✅ Selection statistics tracking
- ✅ Cylinder selection support
- ✅ Preview mode for selections
- ✅ Configurable visual styles

### Design Considerations
1. **Selection Operations**: Move/Copy/Delete operations are not implemented as separate command classes. These should be implemented in the undo/redo system or as voxel manipulation commands that use the selection system.

2. **Thread Safety**: The current implementation assumes single-threaded usage. No explicit synchronization is provided.

3. **Performance Optimizations**: 
   - Uses unordered_set for O(1) voxel lookups
   - Caches bounds and center calculations
   - No spatial indexing currently implemented

4. **Memory Usage**: 
   - Each VoxelId stores position (12 bytes) + resolution (1 byte) + padding
   - No adaptive storage based on selection density
   - No compression for large selections

### Future Enhancements
- Spatial indexing (octree) for large selections
- Adaptive storage strategies (bit vectors for dense selections)
- Multi-threaded selection operations
- Selection groups and hierarchies
- Advanced selection shapes (cone, custom meshes)
- Selection history compression
- Network synchronization support