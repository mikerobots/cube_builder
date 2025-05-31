#pragma once

#include "EventBase.h"
#include "../math/Vector3f.h"
#include "../math/Vector3i.h"

namespace VoxelEditor {
namespace Events {

// Forward declarations for voxel data types
namespace VoxelData {
    struct VoxelPosition;
    enum class VoxelResolution : uint8_t;
}

class VoxelChangedEvent : public Event<VoxelChangedEvent> {
public:
    VoxelChangedEvent(const Math::Vector3i& gridPos, VoxelData::VoxelResolution resolution, bool oldValue, bool newValue)
        : gridPos(gridPos), resolution(resolution), oldValue(oldValue), newValue(newValue) {}
    
    Math::Vector3i gridPos;
    VoxelData::VoxelResolution resolution;
    bool oldValue;
    bool newValue;
};

class ResolutionChangedEvent : public Event<ResolutionChangedEvent> {
public:
    ResolutionChangedEvent(VoxelData::VoxelResolution oldResolution, VoxelData::VoxelResolution newResolution)
        : oldResolution(oldResolution), newResolution(newResolution) {}
    
    VoxelData::VoxelResolution oldResolution;
    VoxelData::VoxelResolution newResolution;
};

class WorkspaceResizedEvent : public Event<WorkspaceResizedEvent> {
public:
    WorkspaceResizedEvent(const Math::Vector3f& oldSize, const Math::Vector3f& newSize)
        : oldSize(oldSize), newSize(newSize) {}
    
    Math::Vector3f oldSize;
    Math::Vector3f newSize;
};

using GroupId = uint32_t;

class GroupCreatedEvent : public Event<GroupCreatedEvent> {
public:
    GroupCreatedEvent(GroupId groupId, const std::string& name)
        : groupId(groupId), name(name) {}
    
    GroupId groupId;
    std::string name;
};

class GroupDeletedEvent : public Event<GroupDeletedEvent> {
public:
    GroupDeletedEvent(GroupId groupId, const std::string& name)
        : groupId(groupId), name(name) {}
    
    GroupId groupId;
    std::string name;
};

enum class GroupModificationType {
    MOVED, RENAMED, VISIBILITY_CHANGED, LOCKED_CHANGED
};

class GroupModifiedEvent : public Event<GroupModifiedEvent> {
public:
    GroupModifiedEvent(GroupId groupId, GroupModificationType type)
        : groupId(groupId), modificationType(type) {}
    
    GroupId groupId;
    GroupModificationType modificationType;
};

class SelectionChangedEvent : public Event<SelectionChangedEvent> {
public:
    enum class ChangeType { ADDED, REMOVED, REPLACED, CLEARED };
    
    SelectionChangedEvent(ChangeType changeType, size_t selectionSize)
        : changeType(changeType), selectionSize(selectionSize) {}
    
    ChangeType changeType;
    size_t selectionSize;
};

class CameraChangedEvent : public Event<CameraChangedEvent> {
public:
    enum class ChangeType { POSITION, ROTATION, ZOOM, VIEW_PRESET };
    
    CameraChangedEvent(ChangeType changeType)
        : changeType(changeType) {}
    
    ChangeType changeType;
};

class MemoryPressureEvent : public Event<MemoryPressureEvent> {
public:
    MemoryPressureEvent(size_t currentUsage, size_t maxUsage)
        : currentUsage(currentUsage), maxUsage(maxUsage) {}
    
    size_t currentUsage;
    size_t maxUsage;
};

class ApplicationExitEvent : public Event<ApplicationExitEvent> {
public:
    ApplicationExitEvent(int exitCode = 0) : exitCode(exitCode) {}
    
    int exitCode;
};

class ConfigurationChangedEvent : public Event<ConfigurationChangedEvent> {
public:
    ConfigurationChangedEvent(const std::string& key) : key(key) {}
    
    std::string key;
};

}
}