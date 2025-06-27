#pragma once

#include "SurfaceTypes.h"
#include "DualContouring.h"
#include "MeshBuilder.h"
#include "MeshSmoother.h"
#include "MeshValidator.h"
#include "../voxel_data/VoxelGrid.h"
#include "../voxel_data/VoxelDataManager.h"
#include "../../foundation/events/EventDispatcher.h"
#include "../../foundation/logging/Logger.h"
#include <memory>
#include <future>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <atomic>
#include <functional>

namespace VoxelEditor {
namespace SurfaceGen {

// Forward declarations
class LODManager;
class MeshCache;
class ProgressiveSmoothingCache;

class SurfaceGenerator {
public:
    SurfaceGenerator(Events::EventDispatcher* eventDispatcher = nullptr);
    ~SurfaceGenerator();
    
    // Main generation functions
    Mesh generateSurface(const VoxelData::VoxelGrid& grid, 
                        const SurfaceSettings& settings = SurfaceSettings::Default());
    Mesh generatePreviewMesh(const VoxelData::VoxelGrid& grid, int lodLevel = 1);
    Mesh generateExportMesh(const VoxelData::VoxelGrid& grid, 
                           ExportQuality quality = ExportQuality::High);
    
    // Generate smoothed surface for toy-like output
    Mesh generateSmoothedSurface(const VoxelData::VoxelGrid& grid, int smoothingLevel = 5);
    
    // Progressive smoothing for real-time preview
    std::string startProgressiveSmoothing(const VoxelData::VoxelGrid& grid, 
                                         const SurfaceSettings& settings);
    Mesh getProgressiveResult(const std::string& progressKey);
    void cancelProgressiveSmoothing(const std::string& progressKey);
    bool isProgressiveSmoothingComplete(const std::string& progressKey);
    
    // Multi-resolution support
    Mesh generateMultiResMesh(const VoxelData::VoxelDataManager& voxelManager,
                             VoxelData::VoxelResolution targetRes);
    std::vector<Mesh> generateAllResolutions(const VoxelData::VoxelDataManager& voxelManager);
    
    // Quality settings
    void setSurfaceSettings(const SurfaceSettings& settings) { m_settings = settings; }
    SurfaceSettings getSurfaceSettings() const { return m_settings; }
    
    // LOD management
    void setLODEnabled(bool enabled) { m_lodEnabled = enabled; }
    bool isLODEnabled() const { return m_lodEnabled; }
    int calculateLOD(float distance, const Math::BoundingBox& bounds) const;
    
    // Cache management
    void enableCaching(bool enabled) { m_cacheEnabled = enabled; }
    bool isCachingEnabled() const { return m_cacheEnabled; }
    void clearCache();
    size_t getCacheMemoryUsage() const;
    void setCacheMaxMemory(size_t maxBytes);
    
    // Async generation
    std::future<Mesh> generateSurfaceAsync(const VoxelData::VoxelGrid& grid, 
                                          const SurfaceSettings& settings);
    
    // Progress callback
    using ProgressCallback = std::function<void(float progress, const std::string& status)>;
    void setProgressCallback(ProgressCallback callback) { m_progressCallback = callback; }
    
    // Cancellation
    void cancelGeneration() { m_cancelRequested = true; }
    bool isCancelled() const { return m_cancelRequested; }
    
    // Event handling
    void onVoxelDataChanged(const Math::BoundingBox& region, VoxelData::VoxelResolution resolution);
    
private:
    // Core components
    std::unique_ptr<DualContouring> m_dualContouring;
    std::unique_ptr<MeshBuilder> m_meshBuilder;
    std::unique_ptr<LODManager> m_lodManager;
    std::unique_ptr<MeshCache> m_meshCache;
    std::unique_ptr<ProgressiveSmoothingCache> m_progressiveCache;
    
    // Settings
    SurfaceSettings m_settings;
    bool m_lodEnabled;
    bool m_cacheEnabled;
    std::atomic<bool> m_cancelRequested;
    
    // Event handling
    Events::EventDispatcher* m_eventDispatcher;
    ProgressCallback m_progressCallback;
    
    // Internal methods
    Mesh generateInternal(const VoxelData::VoxelGrid& grid, 
                         const SurfaceSettings& settings,
                         LODLevel lod = LODLevel::LOD0);
    
    void applyPostProcessing(Mesh& mesh, const SurfaceSettings& settings);
    void optimizeMesh(Mesh& mesh, float targetRatio);
    void applySmoothingToMesh(Mesh& mesh, const SurfaceSettings& settings);
    void validateMeshForPrinting(Mesh& mesh, const SurfaceSettings& settings);
    
    // Cache helpers
    size_t computeGridHash(const VoxelData::VoxelGrid& grid) const;
    std::string getCacheKey(size_t gridHash, const SurfaceSettings& settings, LODLevel lod) const;
    
    // Progress reporting
    void reportProgress(float progress, const std::string& status);
    
    // Multi-threading
    std::mutex m_generationMutex;
    std::vector<std::future<Mesh>> m_activeGenerations;
    std::vector<std::future<void>> m_activeProgressiveGenerations;
};

// LOD Manager
class LODManager {
public:
    LODManager();
    ~LODManager();
    
    // Generate LOD mesh
    Mesh generateLOD(const VoxelData::VoxelGrid& grid, LODLevel level, 
                    const SurfaceSettings& settings, DualContouring* algorithm);
    
    // LOD calculation
    LODLevel calculateLOD(float distance, const Math::BoundingBox& bounds) const;
    
    // LOD distances
    void setLODDistances(const std::vector<float>& distances);
    std::vector<float> getLODDistances() const { return m_lodDistances; }
    
    // Simplification ratios
    void setSimplificationRatio(LODLevel level, float ratio);
    float getSimplificationRatio(LODLevel level) const;
    
private:
    std::vector<float> m_lodDistances;
    std::unordered_map<LODLevel, float> m_simplificationRatios;
    
    // Grid downsampling
    std::unique_ptr<VoxelData::VoxelGrid> downsampleGrid(const VoxelData::VoxelGrid& grid, 
                                                         int factor);
};

// Progressive Smoothing Cache for real-time preview
class ProgressiveSmoothingCache {
public:
    struct CacheEntry {
        Mesh mesh;
        int smoothingLevel;
        PreviewQuality quality;
        std::chrono::steady_clock::time_point timestamp;
        bool isProgressive;  // True if this is a partial result
    };
    
    ProgressiveSmoothingCache();
    ~ProgressiveSmoothingCache();
    
    // Cache operations
    bool hasProgressiveResult(const std::string& baseKey, int targetLevel, PreviewQuality quality) const;
    Mesh getProgressiveResult(const std::string& baseKey, int targetLevel, PreviewQuality quality);
    void cacheProgressiveResult(const std::string& baseKey, const Mesh& mesh, 
                               int smoothingLevel, PreviewQuality quality, bool isProgressive = false);
    
    // Progressive operations
    std::string startProgressiveSmoothing(const std::string& baseKey, int targetLevel, PreviewQuality quality);
    void updateProgressiveResult(const std::string& progressKey, const Mesh& mesh, int currentLevel);
    void finalizeProgressiveResult(const std::string& progressKey, const Mesh& finalMesh);
    void cancelProgressiveSmoothing(const std::string& progressKey);
    
    // Cache management
    void clear();
    void clearExpired(std::chrono::seconds maxAge = std::chrono::seconds(30));
    size_t getMemoryUsage() const { return m_currentMemoryUsage; }
    void setMaxMemoryUsage(size_t maxBytes) { m_maxMemoryUsage = maxBytes; }
    
    // Direct cache access for SurfaceGenerator
    bool hasEntry(const std::string& key) const;
    CacheEntry getEntry(const std::string& key) const;
    
private:
    mutable std::mutex m_cacheMutex;
    std::unordered_map<std::string, CacheEntry> m_cache;
    std::unordered_map<std::string, std::string> m_progressiveKeys; // progressKey -> baseKey mapping
    size_t m_maxMemoryUsage;
    size_t m_currentMemoryUsage;
    
    void evictLRU();
    std::string generateCacheKey(const std::string& baseKey, int level, PreviewQuality quality) const;
};

// Mesh Cache
class MeshCache {
public:
    MeshCache();
    ~MeshCache();
    
    // Cache operations
    bool hasCachedMesh(const std::string& key) const;
    Mesh getCachedMesh(const std::string& key);
    void cacheMesh(const std::string& key, const Mesh& mesh);
    
    // Cache management
    void invalidateRegion(const Math::BoundingBox& region);
    void clear();
    size_t getMemoryUsage() const { return m_currentMemoryUsage; }
    void setMaxMemoryUsage(size_t maxBytes) { m_maxMemoryUsage = maxBytes; }
    
    // Statistics
    size_t getHitCount() const { return m_hitCount; }
    size_t getMissCount() const { return m_missCount; }
    float getHitRate() const;
    
private:
    struct CacheEntry {
        Mesh mesh;
        std::chrono::steady_clock::time_point lastAccess;
        size_t memoryUsage;
        Math::BoundingBox bounds;
        
        void updateAccess();
    };
    
    std::unordered_map<std::string, CacheEntry> m_cache;
    size_t m_maxMemoryUsage;
    size_t m_currentMemoryUsage;
    
    // Statistics
    mutable size_t m_hitCount;
    mutable size_t m_missCount;
    
    // LRU eviction
    void evictLRU();
    void updateAccess(const std::string& key);
    
    mutable std::mutex m_cacheMutex;
};

}
}