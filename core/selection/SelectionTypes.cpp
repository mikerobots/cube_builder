#include "SelectionTypes.h"

namespace VoxelEditor {
namespace Selection {

Math::Vector3f VoxelId::getWorldPosition() const {
    float size = getVoxelSize();
    return Math::Vector3f(
        position.x * size + size * 0.5f,
        position.y * size + size * 0.5f,
        position.z * size + size * 0.5f
    );
}

float VoxelId::getVoxelSize() const {
    return VoxelData::getVoxelSize(resolution);
}

Math::BoundingBox VoxelId::getBounds() const {
    float size = getVoxelSize();
    Math::Vector3f min(
        position.x * size,
        position.y * size,
        position.z * size
    );
    Math::Vector3f max = min + Math::Vector3f(size, size, size);
    return Math::BoundingBox(min, max);
}

}
}