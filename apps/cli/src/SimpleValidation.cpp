#include "cli/Application.h"
#include "cli/RenderWindow.h"
#include "cli/CommandTypes.h"
#include "voxel_data/VoxelDataManager.h"
#include "camera/CameraController.h"
#include "camera/Camera.h"
#include "logging/Logger.h"
#include <sstream>
#include <iomanip>

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

namespace VoxelEditor {

CommandResult executeSimpleValidateCommand(const CommandContext& ctx) {
    Application* app = ctx.getApp();
    auto* renderWindow = app->getRenderWindow();
    auto* voxelManager = app->getVoxelManager();
    
    std::stringstream ss;
    ss << "=== RENDER VALIDATION ===\n\n";
    
    // 1. Check voxel storage
    size_t voxelCount = voxelManager->getVoxelCount();
    ss << "VOXEL STORAGE:\n";
    ss << "  Voxel count: " << voxelCount << "\n";
    ss << "  Active resolution: " << VoxelData::getVoxelSizeName(voxelManager->getActiveResolution()) << "\n";
    
    // 2. Check OpenGL state
    ss << "\nOPENGL STATE:\n";
    
    GLint vao, vbo, program;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    
    ss << "  VAO: " << vao << "\n";
    ss << "  VBO: " << vbo << "\n";
    ss << "  Shader Program: " << program << "\n";
    
    // Check key states
    GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullFace = glIsEnabled(GL_CULL_FACE);
    
    ss << "  Depth Test: " << (depthTest ? "ON" : "OFF") << "\n";
    ss << "  Face Culling: " << (cullFace ? "ON" : "OFF") << "\n";
    
    // Check viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    ss << "  Viewport: " << viewport[2] << "x" << viewport[3] << " at (" 
       << viewport[0] << ", " << viewport[1] << ")\n";
    
    // 3. Check clear color
    GLfloat clearColor[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
    ss << "  Clear Color: (" << std::fixed << std::setprecision(2)
       << clearColor[0] << ", " << clearColor[1] << ", " 
       << clearColor[2] << ", " << clearColor[3] << ")\n";
    
    // 4. Sample some pixels
    ss << "\nPIXEL SAMPLING:\n";
    
    // Center pixel
    int cx = viewport[2] / 2;
    int cy = viewport[3] / 2;
    GLubyte centerPixel[4];
    glReadPixels(cx, cy, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, centerPixel);
    
    ss << "  Center (" << cx << ", " << cy << "): RGBA("
       << (int)centerPixel[0] << ", " << (int)centerPixel[1] << ", "
       << (int)centerPixel[2] << ", " << (int)centerPixel[3] << ")";
    
    // Check if it's the clear color
    bool isClearColor = (std::abs(centerPixel[0]/255.0f - clearColor[0]) < 0.01f &&
                        std::abs(centerPixel[1]/255.0f - clearColor[1]) < 0.01f &&
                        std::abs(centerPixel[2]/255.0f - clearColor[2]) < 0.01f);
    ss << (isClearColor ? " [CLEAR COLOR]" : " [RENDERED]") << "\n";
    
    // Sample corners
    struct { int x, y; const char* name; } samplePoints[] = {
        {50, 50, "Top-left"},
        {viewport[2]-50, 50, "Top-right"},
        {50, viewport[3]-50, "Bottom-left"},
        {viewport[2]-50, viewport[3]-50, "Bottom-right"}
    };
    
    int nonClearPixels = isClearColor ? 0 : 1;
    for (const auto& pt : samplePoints) {
        if (pt.x >= 0 && pt.x < viewport[2] && pt.y >= 0 && pt.y < viewport[3]) {
            GLubyte pixel[4];
            glReadPixels(pt.x, pt.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
            
            bool isThisClear = (std::abs(pixel[0]/255.0f - clearColor[0]) < 0.01f &&
                               std::abs(pixel[1]/255.0f - clearColor[1]) < 0.01f &&
                               std::abs(pixel[2]/255.0f - clearColor[2]) < 0.01f);
            
            if (!isThisClear) {
                nonClearPixels++;
                ss << "  " << pt.name << " (" << pt.x << ", " << pt.y << "): RGBA("
                   << (int)pixel[0] << ", " << (int)pixel[1] << ", "
                   << (int)pixel[2] << ", " << (int)pixel[3] << ") [RENDERED]\n";
            }
        }
    }
    
    ss << "  Non-clear pixels found: " << nonClearPixels << "\n";
    
    // 5. Camera info
    auto* cameraController = app->getCameraController();
    if (cameraController && cameraController->getCamera()) {
        auto* camera = cameraController->getCamera();
        ss << "\nCAMERA:\n";
        auto pos = camera->getPosition();
        auto target = camera->getTarget();
        ss << "  Position: (" << std::fixed << std::setprecision(2)
           << pos.x << ", " << pos.y << ", " << pos.z << ")\n";
        ss << "  Target: (" << target.x << ", " << target.y << ", " << target.z << ")\n";
        ss << "  Distance: " << (pos - target).length() << "\n";
    }
    
    // 6. Summary
    ss << "\nSUMMARY:\n";
    bool hasVoxels = voxelCount > 0;
    bool hasValidGL = vao > 0 && program > 0;
    bool hasRender = nonClearPixels > 0;
    
    ss << "  ✓ Voxels stored: " << (hasVoxels ? "YES" : "NO") << "\n";
    ss << "  ✓ OpenGL ready: " << (hasValidGL ? "YES" : "NO") << "\n";
    ss << "  ✓ Pixels rendered: " << (hasRender ? "YES" : "NO") << "\n";
    
    if (hasVoxels && hasValidGL && !hasRender) {
        ss << "\nPOSSIBLE ISSUES:\n";
        ss << "  - Camera might be looking away from voxels\n";
        ss << "  - Voxels might be outside view frustum\n";
        ss << "  - Shader might not be outputting color\n";
        ss << "  - Depth test might be failing\n";
        ss << "\nTRY: camera reset\n";
    }
    
    return CommandResult::Success(ss.str());
}

} // namespace VoxelEditor