#pragma once

#include <cstdint>
#include <chrono>

namespace VoxelEditor {
namespace Rendering {

struct RenderStats {
    // Frame timing
    uint32_t frameCount = 0;
    float frameTime = 0.0f;         // Current frame time in ms
    float averageFrameTime = 0.0f;  // Average frame time in ms
    float fps = 0.0f;               // Frames per second
    float minFrameTime = 1000.0f;   // Minimum frame time in ms
    float maxFrameTime = 0.0f;      // Maximum frame time in ms
    
    // Geometry statistics
    uint32_t trianglesRendered = 0;
    uint32_t verticesProcessed = 0;
    uint32_t drawCalls = 0;
    uint32_t instancedDrawCalls = 0;
    
    // Memory statistics (in bytes)
    size_t vertexBufferMemory = 0;
    size_t indexBufferMemory = 0;
    size_t textureMemory = 0;
    size_t totalGPUMemory = 0;
    
    // Performance statistics
    float cpuTime = 0.0f;           // CPU time for rendering in ms
    float gpuTime = 0.0f;           // GPU time for rendering in ms
    uint32_t stateChanges = 0;      // Number of OpenGL state changes
    uint32_t shaderSwitches = 0;    // Number of shader program switches
    uint32_t textureBinds = 0;      // Number of texture bindings
    
    // Culling statistics
    uint32_t objectsCulled = 0;
    uint32_t objectsRendered = 0;
    uint32_t totalObjects = 0;
    
    // Resource counts
    uint32_t activeShaders = 0;
    uint32_t activeTextures = 0;
    uint32_t activeBuffers = 0;
    
    RenderStats() = default;
    
    void reset() {
        // Don't reset frame count, fps, and averages
        frameTime = 0.0f;
        
        // Reset per-frame statistics
        trianglesRendered = 0;
        verticesProcessed = 0;
        drawCalls = 0;
        instancedDrawCalls = 0;
        
        cpuTime = 0.0f;
        gpuTime = 0.0f;
        stateChanges = 0;
        shaderSwitches = 0;
        textureBinds = 0;
        
        objectsCulled = 0;
        objectsRendered = 0;
        totalObjects = 0;
    }
    
    void update(float deltaTime) {
        frameCount++;
        frameTime = deltaTime * 1000.0f; // Convert to milliseconds
        
        // Update FPS
        if (deltaTime > 0.0f) {
            fps = 1.0f / deltaTime;
        }
        
        // Update frame time statistics
        if (frameTime < minFrameTime) {
            minFrameTime = frameTime;
        }
        if (frameTime > maxFrameTime) {
            maxFrameTime = frameTime;
        }
        
        // Update rolling average frame time (over last 60 frames)
        const float alpha = 1.0f / 60.0f;
        averageFrameTime = (1.0f - alpha) * averageFrameTime + alpha * frameTime;
    }
    
    void addDrawCall(uint32_t triangles, uint32_t vertices, bool instanced = false) {
        if (instanced) {
            instancedDrawCalls++;
        } else {
            drawCalls++;
        }
        trianglesRendered += triangles;
        verticesProcessed += vertices;
    }
    
    void addStateChange() {
        stateChanges++;
    }
    
    void addShaderSwitch() {
        shaderSwitches++;
    }
    
    void addTextureBind() {
        textureBinds++;
    }
    
    void setCullingStats(uint32_t total, uint32_t rendered, uint32_t culled) {
        totalObjects = total;
        objectsRendered = rendered;
        objectsCulled = culled;
    }
    
    void setMemoryStats(size_t vbMem, size_t ibMem, size_t texMem) {
        vertexBufferMemory = vbMem;
        indexBufferMemory = ibMem;
        textureMemory = texMem;
        totalGPUMemory = vbMem + ibMem + texMem;
    }
    
    void setResourceCounts(uint32_t shaders, uint32_t textures, uint32_t buffers) {
        activeShaders = shaders;
        activeTextures = textures;
        activeBuffers = buffers;
    }
    
    float getCullingEfficiency() const {
        if (totalObjects == 0) return 0.0f;
        return static_cast<float>(objectsCulled) / static_cast<float>(totalObjects);
    }
    
    float getAverageTrianglesPerDrawCall() const {
        if (drawCalls + instancedDrawCalls == 0) return 0.0f;
        return static_cast<float>(trianglesRendered) / static_cast<float>(drawCalls + instancedDrawCalls);
    }
    
    float getAverageFPS() const {
        if (averageFrameTime <= 0.0f) return 0.0f;
        return 1000.0f / averageFrameTime;
    }
    
    size_t getTotalMemoryMB() const {
        return totalGPUMemory / (1024 * 1024);
    }
    
    float getGPUUtilization() const {
        if (frameTime <= 0.0f) return 0.0f;
        return (gpuTime / frameTime) * 100.0f;
    }
    
    float getCPUUtilization() const {
        if (frameTime <= 0.0f) return 0.0f;
        return (cpuTime / frameTime) * 100.0f;
    }
};

// Helper class for timing measurements
class RenderTimer {
public:
    RenderTimer() : m_startTime(std::chrono::high_resolution_clock::now()) {}
    
    void start() {
        m_startTime = std::chrono::high_resolution_clock::now();
    }
    
    float getElapsedMs() const {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - m_startTime);
        return static_cast<float>(duration.count()) / 1000.0f;
    }
    
    float restart() {
        float elapsed = getElapsedMs();
        start();
        return elapsed;
    }
    
private:
    std::chrono::high_resolution_clock::time_point m_startTime;
};

}
}