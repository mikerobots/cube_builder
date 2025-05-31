#pragma once

#include "EventBase.h"
#include "../math/Vector3f.h"
#include "../math/Vector3i.h"

namespace VoxelEditor {
namespace Events {

enum class VoxelResolution : uint8_t {
    Size_1cm = 0,   Size_2cm = 1,   Size_4cm = 2,   Size_8cm = 3,   Size_16cm = 4,
    Size_32cm = 5,  Size_64cm = 6,  Size_128cm = 7, Size_256cm = 8, Size_512cm = 9
};

class VoxelChangedEvent : public Event<VoxelChangedEvent> {
public:
    VoxelChangedEvent(const Math::Vector3i& position, VoxelResolution resolution, bool oldValue, bool newValue)
        : position(position), resolution(resolution), oldValue(oldValue), newValue(newValue) {}
    
    Math::Vector3i position;
    VoxelResolution resolution;
    bool oldValue;
    bool newValue;
};

class ResolutionChangedEvent : public Event<ResolutionChangedEvent> {
public:
    ResolutionChangedEvent(VoxelResolution oldResolution, VoxelResolution newResolution)
        : oldResolution(oldResolution), newResolution(newResolution) {}
    
    VoxelResolution oldResolution;
    VoxelResolution newResolution;
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

}
}