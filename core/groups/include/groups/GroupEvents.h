#pragma once

#include "groups/GroupTypes.h"
#include "events/EventBase.h"
#include <string>
#include <vector>

namespace VoxelEditor {
namespace Groups {

// Group created event
struct GroupCreatedEvent : public Events::Event<GroupCreatedEvent> {
    GroupId groupId;
    std::string name;
    std::vector<VoxelId> voxels;
};

// Group modified event
struct GroupModifiedEvent : public Events::Event<GroupModifiedEvent> {
    GroupId groupId;
    GroupModificationType type;
    std::string oldName;        // for rename events
    std::string newName;        // for rename events
    bool oldVisible;            // for visibility events
    bool newVisible;            // for visibility events
    Math::Vector3f offset;      // for move events
};

// Group deleted event
struct GroupDeletedEvent : public Events::Event<GroupDeletedEvent> {
    GroupId groupId;
    std::string name;
    std::vector<VoxelId> releasedVoxels;
};

} // namespace Groups
} // namespace VoxelEditor