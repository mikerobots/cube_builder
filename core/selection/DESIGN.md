# Selection System Subsystem

## Purpose
Manages voxel selection operations, multi-selection handling, selection persistence, and provides visual feedback for selected elements.

## Current Implementation Status
The implementation closely matches the design with all major components implemented:
- **SelectionManager** - Main interface fully implemented with additional features
- **SelectionSet** - Efficient storage implemented with statistics
- **SelectionRenderer** - Visual representation implemented
- **BoxSelector** - Box selection algorithm implemented
- **SphereSelector** - Sphere selection algorithm implemented  
- **FloodFillSelector** - Flood fill algorithm implemented
- **SelectionTypes** - All data structures and enums defined

Missing components from design:
- **SelectionOperations** - Move/Copy/Delete operations not as separate classes (likely integrated into manager)

## Key Components

### SelectionManager
**Responsibility**: Main interface for selection operations
- Single and multi-voxel selection
- Selection state management
- Selection history tracking
- Integration with group operations

### SelectionSet
**Responsibility**: Efficient storage of selected voxels
- Set operations (union, intersection, difference)
- Spatial queries on selections
- Memory-efficient storage
- Serialization support

### SelectionOperations
**Responsibility**: Bulk operations on selected voxels
- Move selected voxels
- Copy/duplicate selections
- Delete selected voxels
- Transform operations

### SelectionRenderer
**Responsibility**: Visual representation of selections
- Highlight rendering
- Selection bounds visualization
- Multi-selection color coding
- Performance-optimized rendering

## Interface Design

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
    void selectFloodFill(const VoxelId& seed);
    
    // Selection queries
    bool isSelected(const VoxelId& voxel) const;
    std::vector<VoxelId> getSelection() const;
    size_t getSelectionSize() const;
    bool hasSelection() const;
    BoundingBox getSelectionBounds() const;
    
    // Selection history
    void pushSelectionToHistory();
    bool canUndo() const;
    bool canRedo() const;
    void undoSelection();
    void redoSelection();
    
    // Selection sets
    void saveSelectionSet(const std::string& name);
    bool loadSelectionSet(const std::string& name);
    std::vector<std::string> getSelectionSetNames() const;
    void deleteSelectionSet(const std::string& name);
    
    // Set operations
    void unionWith(const SelectionSet& other);
    void intersectWith(const SelectionSet& other);
    void subtractFrom(const SelectionSet& other);
    
private:
    SelectionSet m_currentSelection;
    std::stack<SelectionSet> m_undoStack;
    std::stack<SelectionSet> m_redoStack;
    std::unordered_map<std::string, SelectionSet> m_namedSets;
    VoxelDataManager* m_voxelManager;
    EventDispatcher* m_eventDispatcher;
    size_t m_maxHistorySize;
};
```

## Data Structures

### VoxelId
```cpp
struct VoxelId {
    Vector3i position;
    VoxelResolution resolution;
    
    bool operator==(const VoxelId& other) const;
    bool operator<(const VoxelId& other) const;
    size_t hash() const;
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
    
    // Set operations
    SelectionSet unionWith(const SelectionSet& other) const;
    SelectionSet intersectWith(const SelectionSet& other) const;
    SelectionSet subtract(const SelectionSet& other) const;
    
    // Queries
    size_t size() const;
    bool empty() const;
    std::vector<VoxelId> toVector() const;
    BoundingBox getBounds() const;
    
    // Iteration
    auto begin() const { return m_voxels.begin(); }
    auto end() const { return m_voxels.end(); }
    
    // Serialization
    void serialize(BinaryWriter& writer) const;
    void deserialize(BinaryReader& reader);
    
private:
    std::unordered_set<VoxelId> m_voxels;
    mutable std::optional<BoundingBox> m_cachedBounds;
};
```

### SelectionMode
```cpp
enum class SelectionMode {
    Replace,    // Replace current selection
    Add,        // Add to current selection
    Subtract,   // Remove from current selection
    Intersect   // Intersect with current selection
};
```

## Selection Algorithms

### Flood Fill Selection
```cpp
class FloodFillSelector {
public:
    SelectionSet select(const VoxelId& seed, VoxelDataManager& voxelManager, FloodFillCriteria criteria);
    
private:
    enum class FloodFillCriteria {
        Connected,          // Connected voxels only
        SameResolution,     // Same resolution level
        ConnectedSameRes    // Connected + same resolution
    };
    
    bool shouldInclude(const VoxelId& current, const VoxelId& neighbor, FloodFillCriteria criteria);
    std::vector<VoxelId> getNeighbors(const VoxelId& voxel);
};
```

### Box Selection
```cpp
class BoxSelector {
public:
    SelectionSet select(const BoundingBox& box, VoxelResolution resolution, VoxelDataManager& voxelManager);
    
private:
    bool isVoxelInBox(const VoxelId& voxel, const BoundingBox& box);
    std::vector<VoxelId> getVoxelsInRegion(const BoundingBox& box, VoxelResolution resolution);
};
```

### Sphere Selection
```cpp
class SphereSelector {
public:
    SelectionSet select(const Vector3f& center, float radius, VoxelResolution resolution, VoxelDataManager& voxelManager);
    
private:
    bool isVoxelInSphere(const VoxelId& voxel, const Vector3f& center, float radius);
    float distanceToVoxel(const VoxelId& voxel, const Vector3f& point);
};
```

## Visual Feedback

### Selection Rendering
```cpp
class SelectionRenderer {
public:
    void renderSelection(const SelectionSet& selection, const RenderContext& context);
    void renderSelectionBounds(const BoundingBox& bounds, const RenderContext& context);
    void renderSelectionPreview(const SelectionSet& preview, const RenderContext& context);
    
    // Visual settings
    void setSelectionColor(const Color& color);
    void setSelectionOpacity(float opacity);
    void setAnimationEnabled(bool enabled);
    
private:
    void renderVoxelHighlight(const VoxelId& voxel, const Color& color, const RenderContext& context);
    void renderWireframeBounds(const BoundingBox& bounds, const Color& color, const RenderContext& context);
    
    Color m_selectionColor;
    Color m_previewColor;
    float m_opacity;
    bool m_animationEnabled;
    std::unique_ptr<Mesh> m_highlightMesh;
};
```

### Visual Indicators
- **Selected Voxels**: Bright outline or overlay
- **Selection Bounds**: Wireframe bounding box
- **Multi-Selection**: Color-coded by selection order
- **Preview Selection**: Semi-transparent overlay
- **Animation**: Pulsing or breathing effect

## Selection Operations

### Move Operation
```cpp
class MoveSelectionOperation {
public:
    MoveSelectionOperation(const SelectionSet& selection, const Vector3f& offset);
    bool execute(VoxelDataManager& voxelManager, SelectionManager& selectionManager);
    bool undo(VoxelDataManager& voxelManager, SelectionManager& selectionManager);
    
private:
    SelectionSet m_originalSelection;
    SelectionSet m_newSelection;
    Vector3f m_offset;
    std::vector<std::pair<VoxelId, VoxelId>> m_moves;
};
```

### Copy Operation
```cpp
class CopySelectionOperation {
public:
    CopySelectionOperation(const SelectionSet& selection, const Vector3f& offset);
    SelectionSet execute(VoxelDataManager& voxelManager);
    bool undo(VoxelDataManager& voxelManager);
    
private:
    SelectionSet m_sourceSelection;
    SelectionSet m_createdSelection;
    Vector3f m_offset;
};
```

### Delete Operation
```cpp
class DeleteSelectionOperation {
public:
    DeleteSelectionOperation(const SelectionSet& selection);
    bool execute(VoxelDataManager& voxelManager, SelectionManager& selectionManager);
    bool undo(VoxelDataManager& voxelManager, SelectionManager& selectionManager);
    
private:
    SelectionSet m_deletedVoxels;
};
```

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

### SelectionChanged Event
```cpp
struct SelectionChangedEvent {
    SelectionSet oldSelection;
    SelectionSet newSelection;
    SelectionChangeType changeType; // ADDED, REMOVED, REPLACED, CLEARED
};
```

### SelectionOperationEvent
```cpp
struct SelectionOperationEvent {
    SelectionOperationType operationType; // MOVE, COPY, DELETE
    SelectionSet affectedVoxels;
    bool success;
};
```

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

## Known Issues and Technical Debt

### Issue 1: Missing Selection Operations Classes
- **Severity**: Medium
- **Impact**: Move/Copy/Delete operations not implemented as separate command classes
- **Proposed Solution**: Implement operation classes for undo/redo integration
- **Dependencies**: Undo/Redo system design

### Issue 2: No Cylinder Selection in Design
- **Severity**: Low
- **Impact**: SelectionManager has selectCylinder but not documented in design
- **Proposed Solution**: Update design to include cylinder selection or remove if not needed
- **Dependencies**: None

### Issue 3: Performance with Large Selections
- **Severity**: Medium
- **Impact**: No spatial indexing implementation visible for optimization
- **Proposed Solution**: Implement octree-based spatial indexing as designed
- **Dependencies**: Spatial data structure implementation

### Issue 4: Memory Optimization Not Implemented
- **Severity**: Low
- **Impact**: No bit vector or sparse storage optimization based on density
- **Proposed Solution**: Implement adaptive storage based on selection density
- **Dependencies**: Performance profiling data

### Issue 5: Selection Animation Not Clear
- **Severity**: Low
- **Impact**: Animation settings in renderer but implementation unclear
- **Proposed Solution**: Document and implement selection animation system
- **Dependencies**: Rendering pipeline timing

### Issue 6: Thread Safety Concerns
- **Severity**: Medium
- **Impact**: No visible thread safety measures in SelectionManager
- **Proposed Solution**: Add thread safety or document single-threaded requirements
- **Dependencies**: Application threading model

### Issue 7: Selection Statistics Implementation
- **Severity**: Low
- **Impact**: SelectionStats referenced but not defined in visible headers
- **Proposed Solution**: Define SelectionStats structure or remove if unused
- **Dependencies**: None

### Issue 8: Event System Integration Weak
- **Severity**: Medium
- **Impact**: EventDispatcher passed but event types not clearly defined
- **Proposed Solution**: Define and document selection event types
- **Dependencies**: Event system architecture