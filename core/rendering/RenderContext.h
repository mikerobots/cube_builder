#pragma once

namespace VoxelEditor {
namespace Rendering {

struct RenderContext {
    int screenWidth = 1920;
    int screenHeight = 1080;
    float deltaTime = 0.016f;
    uint32_t frameNumber = 0;
};

} // namespace Rendering
} // namespace VoxelEditor