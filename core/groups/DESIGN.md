# Group Management Subsystem

## Purpose
Manages voxel grouping, group operations (move, hide, copy), hierarchical organization, and group metadata persistence.

## Key Components

### GroupManager
**Responsibility**: Main interface for group operations
- Create and manage groups
- Handle group visibility
- Coordinate group operations
- Maintain group hierarchy

### VoxelGroup
**Responsibility**: Individual group representation
- Store group metadata (name, color, etc.)
- Track member voxels
- Handle group-specific operations
- Maintain group state

### GroupHierarchy
**Responsibility**: Nested group management
- Parent-child relationships
- Inheritance of properties
- Hierarchical operations
- Cycle detection

### GroupOperations
**Responsibility**: Bulk group operations
- Move entire groups
- Copy/duplicate groups
- Merge/split operations
- Batch transformations

## Interface Design

```cpp
class GroupManager {
public:
    // Group lifecycle
    GroupId createGroup(const std::string& name, const std::vector<VoxelId>& voxels);
    bool deleteGroup(GroupId id);
    bool renameGroup(GroupId id, const std::string& newName);
    
    // Group membership
    bool addVoxelToGroup(GroupId id, const VoxelId& voxel);
    bool removeVoxelFromGroup(GroupId id, const VoxelId& voxel);
    std::vector<VoxelId> getGroupVoxels(GroupId id) const;
    
    // Group visibility
    void hideGroup(GroupId id);
    void showGroup(GroupId id);
    bool isGroupVisible(GroupId id) const;
    void setGroupOpacity(GroupId id, float opacity);
    
    // Group operations
    void moveGroup(GroupId id, const Vector3f& offset);
    GroupId copyGroup(GroupId id, const std::string& newName = "");
    void rotateGroup(GroupId id, const Quaternion& rotation);
    void scaleGroup(GroupId id, float scale);
    
    // Hierarchy management
    bool setParentGroup(GroupId child, GroupId parent);
    GroupId getParentGroup(GroupId id) const;
    std::vector<GroupId> getChildGroups(GroupId id) const;
    
    // Group queries
    std::vector<GroupInfo> listGroups() const;
    std::vector<GroupId> findGroupsByName(const std::string& name) const;
    GroupId findGroupContaining(const VoxelId& voxel) const;
    
    // Group locking
    void lockGroup(GroupId id);
    void unlockGroup(GroupId id);
    bool isGroupLocked(GroupId id) const;
    
private:
    std::unordered_map<GroupId, std::unique_ptr<VoxelGroup>> m_groups;
    std::unique_ptr<GroupHierarchy> m_hierarchy;
    GroupId m_nextGroupId;
    VoxelDataManager* m_voxelManager;
    EventDispatcher* m_eventDispatcher;
};
```

## Data Structures

### GroupId
```cpp
using GroupId = uint32_t;
constexpr GroupId INVALID_GROUP_ID = 0;
```

### VoxelId
```cpp
struct VoxelId {
    Vector3i position;
    VoxelResolution resolution;
    
    bool operator==(const VoxelId& other) const;
    size_t hash() const;
};
```

### GroupInfo
```cpp
struct GroupInfo {
    GroupId id;
    std::string name;
    Color color;
    bool visible;
    bool locked;
    float opacity;
    size_t voxelCount;
    GroupId parentId;
    std::vector<GroupId> childIds;
    BoundingBox bounds;
};
```

### GroupMetadata
```cpp
struct GroupMetadata {
    std::string name;
    Color color;
    bool visible;
    bool locked;
    float opacity;
    Vector3f pivot;
    std::string description;
    std::chrono::time_point<std::chrono::system_clock> created;
    std::chrono::time_point<std::chrono::system_clock> modified;
};
```

## Group Operations

### Move Operation
```cpp
class MoveGroupOperation {
public:
    MoveGroupOperation(GroupId id, const Vector3f& offset);
    void execute(VoxelDataManager& voxelManager);
    void undo(VoxelDataManager& voxelManager);
    
private:
    GroupId m_groupId;
    Vector3f m_offset;
    std::vector<std::pair<VoxelId, VoxelId>> m_moves; // old -> new positions
};
```

### Copy Operation
```cpp
class CopyGroupOperation {
public:
    CopyGroupOperation(GroupId sourceId, const std::string& newName, const Vector3f& offset = Vector3f::Zero());
    GroupId execute(GroupManager& groupManager, VoxelDataManager& voxelManager);
    void undo(GroupManager& groupManager, VoxelDataManager& voxelManager);
    
private:
    GroupId m_sourceId;
    GroupId m_createdId;
    std::string m_newName;
    Vector3f m_offset;
    std::vector<VoxelId> m_createdVoxels;
};
```

## Visual Properties

### Group Colors
- Auto-assign colors from predefined palette
- User-customizable colors
- Color inheritance in hierarchies
- High-contrast color selection

### Group Indicators
- Outline rendering for group boundaries
- Color-coded group visualization
- Opacity blending for hidden groups
- Visual hierarchy indicators

## Hierarchy Management

### Rules
- Groups can have at most one parent
- Circular references are prevented
- Operations propagate to children
- Visibility inheritance from parents

### Hierarchy Operations
```cpp
class GroupHierarchy {
public:
    bool addChild(GroupId parent, GroupId child);
    bool removeChild(GroupId parent, GroupId child);
    bool hasAncestor(GroupId descendant, GroupId ancestor) const;
    std::vector<GroupId> getAllDescendants(GroupId id) const;
    std::vector<GroupId> getRootGroups() const;
    
private:
    std::unordered_map<GroupId, GroupId> m_parentMap;
    std::unordered_map<GroupId, std::vector<GroupId>> m_childrenMap;
};
```

## Events

### GroupCreated Event
```cpp
struct GroupCreatedEvent {
    GroupId groupId;
    std::string name;
    std::vector<VoxelId> voxels;
};
```

### GroupModified Event
```cpp
struct GroupModifiedEvent {
    GroupId groupId;
    GroupModificationType type; // MOVED, RENAMED, VISIBILITY_CHANGED, etc.
    // Type-specific data
};
```

### GroupDeleted Event
```cpp
struct GroupDeletedEvent {
    GroupId groupId;
    std::string name;
    std::vector<VoxelId> releasedVoxels;
};
```

## Performance Considerations

### Memory Optimization
- Store voxel references, not copies
- Use bit vectors for large group membership
- Lazy evaluation of group bounds
- Memory pooling for group objects

### Operation Efficiency
- Spatial indexing for group intersection queries
- Cached bounding boxes
- Incremental updates for group modifications
- Parallel processing for bulk operations

### Rendering Integration
- Frustum culling per group
- Level-of-detail based on group size
- Instanced rendering for similar groups
- Occlusion culling optimization

## Testing Strategy

### Unit Tests
- Group creation and deletion
- Membership management
- Hierarchy validation
- Operation correctness
- Event propagation

### Integration Tests
- Interaction with voxel data manager
- Rendering system integration
- File I/O roundtrip testing
- Undo/redo system integration

### Performance Tests
- Large group handling
- Deep hierarchy performance
- Bulk operation efficiency
- Memory usage validation

## Dependencies
- **Core/VoxelData**: Voxel operations and validation
- **Foundation/Events**: Event system for notifications
- **Foundation/Math**: Vector operations and transformations
- **Core/Rendering**: Visual representation of groups
- **Core/UndoRedo**: Command pattern integration

## File I/O Integration
- Serialize group metadata and hierarchy
- Store group-voxel relationships efficiently
- Version compatibility for group data
- Incremental saves for group changes