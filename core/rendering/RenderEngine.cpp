#include "RenderEngine.h"
#include "RenderTypes.h"

namespace VoxelEditor {
namespace Rendering {

RenderEngine::RenderEngine(Events::EventDispatcher* eventDispatcher) 
    : m_eventDispatcher(eventDispatcher)
    , m_initialized(false) {
}

RenderEngine::~RenderEngine() {
    if (m_initialized) {
        shutdown();
    }
}

bool RenderEngine::initialize(const RenderConfig& config) {
    if (m_initialized) {
        return false;
    }
    
    m_config = config;
    m_initialized = true;
    return true;
}

void RenderEngine::shutdown() {
    m_initialized = false;
}

void RenderEngine::clear(ClearFlags flags, const Color& color, float depth, int stencil) {
    // TODO: Implement clearing
}

void RenderEngine::setViewport(int x, int y, int width, int height) {
    // TODO: Implement viewport setting
}

ShaderId RenderEngine::getBuiltinShader(const std::string& name) {
    // TODO: Implement shader management
    return 0;
}

} // namespace Rendering
} // namespace VoxelEditor