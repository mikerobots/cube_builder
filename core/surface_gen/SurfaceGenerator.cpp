#include "SurfaceGenerator.h"
#include "DualContouringSparse.h"
#include <thread>
#include <sstream>
#include <algorithm>
#include <atomic>

namespace VoxelEditor {
namespace SurfaceGen {

// SurfaceGenerator implementation
SurfaceGenerator::SurfaceGenerator(Events::EventDispatcher* eventDispatcher)
    : m_eventDispatcher(eventDispatcher)
    , m_lodEnabled(true)
    , m_cacheEnabled(true)
    , m_cancelRequested(false) {
    
    m_dualContouring = std::make_unique<DualContouringSparse>();
    m_meshBuilder = std::make_unique<MeshBuilder>();
    m_lodManager = std::make_unique<LODManager>();
    m_meshCache = std::make_unique<MeshCache>();
    
    // Set default cache size to 256MB
    m_meshCache->setMaxMemoryUsage(256 * 1024 * 1024);
    
    // Set default settings
    m_settings = SurfaceSettings::Default();
}

SurfaceGenerator::~SurfaceGenerator() {
    // Cancel any active generations
    m_cancelRequested = true;
    
    // Wait for active generations to complete
    for (auto& future : m_activeGenerations) {
        if (future.valid()) {
            future.wait();
        }
    }
}

Mesh SurfaceGenerator::generateSurface(const VoxelData::VoxelGrid& grid, 
                                      const SurfaceSettings& settings) {
    Math::Vector3i dims = grid.getGridDimensions();
    Logging::Logger::getInstance().debugfc("SurfaceGenerator", 
        "Generating surface from grid (%dx%dx%d, resolution=%dcm)", 
        dims.x, dims.y, dims.z, 1 << static_cast<int>(grid.getResolution()));
    
    return generateInternal(grid, settings, LODLevel::LOD0);
}

Mesh SurfaceGenerator::generatePreviewMesh(const VoxelData::VoxelGrid& grid, int lodLevel) {
    SurfaceSettings previewSettings = SurfaceSettings::Preview();
    LODLevel lod = static_cast<LODLevel>(std::min(lodLevel, static_cast<int>(LODLevel::LOD4)));
    return generateInternal(grid, previewSettings, lod);
}

Mesh SurfaceGenerator::generateMultiResMesh(const VoxelData::VoxelDataManager& voxelManager,
                                           VoxelData::VoxelResolution targetRes) {
    // Get grid at target resolution
    const VoxelData::VoxelGrid* targetGrid = voxelManager.getGrid(targetRes);
    if (!targetGrid) {
        return Mesh();
    }
    
    // Generate base mesh for target resolution
    Mesh baseMesh = generateSurface(*targetGrid, m_settings);
    
    // Check if we need to incorporate details from higher resolutions
    if (targetRes == VoxelData::VoxelResolution::Size_1cm) {
        // Already at highest resolution
        return baseMesh;
    }
    
    // Find all resolutions with data
    std::vector<VoxelData::VoxelResolution> activeResolutions;
    for (int res = static_cast<int>(VoxelData::VoxelResolution::Size_1cm); 
         res <= static_cast<int>(targetRes); ++res) {
        auto resolution = static_cast<VoxelData::VoxelResolution>(res);
        const VoxelData::VoxelGrid* grid = voxelManager.getGrid(resolution);
        if (grid && !grid->isEmpty()) {
            activeResolutions.push_back(resolution);
        }
    }
    
    if (activeResolutions.empty()) {
        return baseMesh;
    }
    
    // Generate meshes for each active resolution
    std::vector<Mesh> meshes;
    for (auto resolution : activeResolutions) {
        const VoxelData::VoxelGrid* grid = voxelManager.getGrid(resolution);
        if (grid) {
            Mesh mesh = generateSurface(*grid, m_settings);
            if (mesh.isValid()) {
                meshes.push_back(mesh);
            }
        }
    }
    
    // Combine meshes
    if (meshes.size() > 1) {
        return MeshBuilder::combineMeshes(meshes);
    } else if (!meshes.empty()) {
        return meshes[0];
    }
    
    return baseMesh;
}

std::vector<Mesh> SurfaceGenerator::generateAllResolutions(const VoxelData::VoxelDataManager& voxelManager) {
    std::vector<Mesh> meshes;
    
    // Generate meshes for all 10 resolution levels
    for (int res = static_cast<int>(VoxelData::VoxelResolution::Size_1cm); 
         res <= static_cast<int>(VoxelData::VoxelResolution::Size_512cm); ++res) {
        auto resolution = static_cast<VoxelData::VoxelResolution>(res);
        const VoxelData::VoxelGrid* grid = voxelManager.getGrid(resolution);
        
        if (grid && !grid->isEmpty()) {
            reportProgress(static_cast<float>(res) / 10.0f, 
                         "Generating resolution " + std::to_string(1 << res) + "cm");
            
            Mesh mesh = generateSurface(*grid, m_settings);
            if (mesh.isValid()) {
                meshes.push_back(mesh);
            }
            
            if (m_cancelRequested) {
                break;
            }
        }
    }
    
    return meshes;
}

Mesh SurfaceGenerator::generateExportMesh(const VoxelData::VoxelGrid& grid, 
                                         ExportQuality quality) {
    SurfaceSettings exportSettings = SurfaceSettings::Export();
    
    // Adjust settings based on quality
    switch (quality) {
        case ExportQuality::Draft:
            exportSettings.smoothingIterations = 1;
            exportSettings.simplificationRatio = 0.5f;
            break;
        case ExportQuality::Standard:
            exportSettings.smoothingIterations = 2;
            exportSettings.simplificationRatio = 0.75f;
            break;
        case ExportQuality::High:
            exportSettings.smoothingIterations = 3;
            exportSettings.simplificationRatio = 0.9f;
            break;
        case ExportQuality::Maximum:
            exportSettings.smoothingIterations = 5;
            exportSettings.simplificationRatio = 1.0f;
            break;
    }
    
    return generateInternal(grid, exportSettings, LODLevel::LOD0);
}

Mesh SurfaceGenerator::generateInternal(const VoxelData::VoxelGrid& grid, 
                                       const SurfaceSettings& settings,
                                       LODLevel lod) {
    std::lock_guard<std::mutex> lock(m_generationMutex);
    
    // Check cache first
    if (m_cacheEnabled) {
        size_t gridHash = computeGridHash(grid);
        std::string cacheKey = getCacheKey(gridHash, settings, lod);
        
        if (m_meshCache->hasCachedMesh(cacheKey)) {
            reportProgress(1.0f, "Loaded from cache");
            return m_meshCache->getCachedMesh(cacheKey);
        }
    }
    
    reportProgress(0.0f, "Starting mesh generation");
    
    // Generate mesh
    Mesh mesh;
    
    if (lod == LODLevel::LOD0) {
        // Full resolution
        Logging::Logger::getInstance().debug("Generating full resolution mesh", "SurfaceGenerator");
        mesh = m_dualContouring->generateMesh(grid, settings);
    } else {
        // Generate LOD
        Logging::Logger::getInstance().debugfc("SurfaceGenerator", "Generating LOD%d mesh", static_cast<int>(lod));
        mesh = m_lodManager->generateLOD(grid, lod, settings, m_dualContouring.get());
    }
    
    if (mesh.isValid()) {
        Logging::Logger::getInstance().debugfc("SurfaceGenerator", 
            "Generated mesh: %zu vertices, %zu triangles", 
            mesh.vertices.size(), mesh.indices.size() / 3);
    } else {
        Logging::Logger::getInstance().warning("Generated invalid mesh", "SurfaceGenerator");
    }
    
    if (m_cancelRequested) {
        return Mesh();
    }
    
    // Apply post-processing
    reportProgress(0.8f, "Post-processing");
    applyPostProcessing(mesh, settings);
    
    // Cache the result
    if (m_cacheEnabled && mesh.isValid()) {
        size_t gridHash = computeGridHash(grid);
        std::string cacheKey = getCacheKey(gridHash, settings, lod);
        m_meshCache->cacheMesh(cacheKey, mesh);
    }
    
    reportProgress(1.0f, "Complete");
    
    // Notify completion
    if (m_eventDispatcher) {
        MeshGenerationEvent event(MeshGenerationEventType::Completed);
        event.lodLevel = lod;
        // m_eventDispatcher->dispatch(event);
    }
    
    return mesh;
}

void SurfaceGenerator::applyPostProcessing(Mesh& mesh, const SurfaceSettings& settings) {
    if (!mesh.isValid()) return;
    
    // Remove duplicate vertices
    MeshBuilder builder;
    builder.beginMesh();
    
    // Copy mesh data
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        if (i < mesh.normals.size()) {
            if (i < mesh.uvCoords.size()) {
                builder.addVertex(mesh.vertices[i], mesh.normals[i], mesh.uvCoords[i]);
            } else {
                builder.addVertex(mesh.vertices[i], mesh.normals[i]);
            }
        } else {
            builder.addVertex(mesh.vertices[i]);
        }
    }
    
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        builder.addTriangle(mesh.indices[i], mesh.indices[i+1], mesh.indices[i+2]);
    }
    
    builder.removeDuplicateVertices();
    
    // Generate UVs if requested
    if (settings.generateUVs) {
        builder.generateBoxUVs();
        Logging::Logger::getInstance().debugfc("SurfaceGenerator", 
            "Generated UVs for %zu vertices", builder.getCurrentVertexCount());
    }
    
    mesh = builder.endMesh();
    
    Logging::Logger::getInstance().debugfc("SurfaceGenerator", 
        "Post-processing complete: %zu vertices, %zu UVs", 
        mesh.vertices.size(), mesh.uvCoords.size());
    
    // Apply simplification (but skip if UVs were generated to preserve them)
    if (settings.simplificationRatio < 1.0f && !settings.generateUVs) {
        optimizeMesh(mesh, settings.simplificationRatio);
    }
}

void SurfaceGenerator::optimizeMesh(Mesh& mesh, float targetRatio) {
    if (targetRatio >= 1.0f) return;
    
    MeshSimplifier simplifier;
    SimplificationSettings simplifySettings = SimplificationSettings::Balanced();
    simplifySettings.targetRatio = targetRatio;
    mesh = simplifier.simplify(mesh, simplifySettings);
}

int SurfaceGenerator::calculateLOD(float distance, const Math::BoundingBox& bounds) const {
    return static_cast<int>(m_lodManager->calculateLOD(distance, bounds));
}

std::future<Mesh> SurfaceGenerator::generateSurfaceAsync(const VoxelData::VoxelGrid& grid, 
                                                        const SurfaceSettings& settings) {
    // Clean up completed futures first
    m_activeGenerations.erase(
        std::remove_if(m_activeGenerations.begin(), m_activeGenerations.end(),
            [](std::future<Mesh>& f) {
                return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
            }),
        m_activeGenerations.end()
    );
    
    return std::async(std::launch::async, [this, &grid, settings]() {
        return generateSurface(grid, settings);
    });
}

void SurfaceGenerator::clearCache() {
    m_meshCache->clear();
}

size_t SurfaceGenerator::getCacheMemoryUsage() const {
    return m_meshCache->getMemoryUsage();
}

void SurfaceGenerator::setCacheMaxMemory(size_t maxBytes) {
    m_meshCache->setMaxMemoryUsage(maxBytes);
}

void SurfaceGenerator::onVoxelDataChanged(const Math::BoundingBox& region, 
                                         VoxelData::VoxelResolution resolution) {
    // Invalidate cache for affected region
    if (m_cacheEnabled) {
        m_meshCache->invalidateRegion(region);
    }
}

size_t SurfaceGenerator::computeGridHash(const VoxelData::VoxelGrid& grid) const {
    // Simple hash based on grid dimensions and content
    size_t hash = 0;
    
    auto hashCombine = [&hash](size_t value) {
        hash ^= value + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    };
    
    Math::Vector3i dims = grid.getGridDimensions();
    hashCombine(std::hash<int>{}(dims.x));
    hashCombine(std::hash<int>{}(dims.y));
    hashCombine(std::hash<int>{}(dims.z));
    
    // Sample some voxels for hash
    int step = std::max(1, dims.x * dims.y * dims.z / 1000);
    for (int i = 0; i < dims.x * dims.y * dims.z; i += step) {
        int z = i / (dims.x * dims.y);
        int y = (i / dims.x) % dims.y;
        int x = i % dims.x;
        
        if (grid.getVoxel(Math::Vector3i(x, y, z))) {
            hashCombine(i);
        }
    }
    
    return hash;
}

std::string SurfaceGenerator::getCacheKey(size_t gridHash, const SurfaceSettings& settings, 
                                         LODLevel lod) const {
    std::stringstream ss;
    ss << gridHash << "_" << settings.hash() << "_" << static_cast<int>(lod);
    return ss.str();
}

void SurfaceGenerator::reportProgress(float progress, const std::string& status) {
    if (m_progressCallback) {
        m_progressCallback(progress, status);
    }
}

float LODManager::getSimplificationRatio(LODLevel level) const {
    auto it = m_simplificationRatios.find(level);
    return (it != m_simplificationRatios.end()) ? it->second : 1.0f;
}

void LODManager::setSimplificationRatio(LODLevel level, float ratio) {
    m_simplificationRatios[level] = std::clamp(ratio, 0.0f, 1.0f);
}

void LODManager::setLODDistances(const std::vector<float>& distances) {
    if (distances.size() == 5) {
        m_lodDistances = distances;
    }
}

// LODManager implementation
LODManager::LODManager() {
    // Default LOD distances
    m_lodDistances = {0.0f, 10.0f, 25.0f, 50.0f, 100.0f};
    
    // Default simplification ratios
    m_simplificationRatios[LODLevel::LOD0] = 1.0f;
    m_simplificationRatios[LODLevel::LOD1] = 0.5f;
    m_simplificationRatios[LODLevel::LOD2] = 0.25f;
    m_simplificationRatios[LODLevel::LOD3] = 0.125f;
    m_simplificationRatios[LODLevel::LOD4] = 0.0625f;
}

LODManager::~LODManager() = default;

Mesh LODManager::generateLOD(const VoxelData::VoxelGrid& grid, LODLevel level, 
                             const SurfaceSettings& settings, DualContouring* algorithm) {
    if (level == LODLevel::LOD0) {
        return algorithm->generateMesh(grid, settings);
    }
    
    // Downsample grid
    int factor = 1 << static_cast<int>(level); // 2^level
    auto downsampledGrid = downsampleGrid(grid, factor);
    
    if (!downsampledGrid) {
        return Mesh();
    }
    
    // Generate mesh from downsampled grid
    Mesh mesh = algorithm->generateMesh(*downsampledGrid, settings);
    
    // Apply additional simplification if needed
    float ratio = getSimplificationRatio(level);
    if (ratio < 1.0f) {
        MeshSimplifier simplifier;
        SimplificationSettings simplifySettings = SimplificationSettings::Balanced();
        simplifySettings.targetRatio = ratio;
        mesh = simplifier.simplify(mesh, simplifySettings);
    }
    
    return mesh;
}

LODLevel LODManager::calculateLOD(float distance, const Math::BoundingBox& bounds) const {
    // Use bounding box size to adjust LOD calculation
    float size = (bounds.max - bounds.min).length();
    float adjustedDistance = distance / std::max(1.0f, size);
    
    // Find appropriate LOD level
    for (int i = static_cast<int>(LODLevel::LOD4); i >= 0; --i) {
        if (adjustedDistance >= m_lodDistances[i]) {
            return static_cast<LODLevel>(i);
        }
    }
    
    return LODLevel::LOD0;
}

std::unique_ptr<VoxelData::VoxelGrid> LODManager::downsampleGrid(const VoxelData::VoxelGrid& grid, 
                                                                 int factor) {
    Math::Vector3i oldDims = grid.getGridDimensions();
    Math::Vector3i newDims(
        (oldDims.x + factor - 1) / factor,
        (oldDims.y + factor - 1) / factor,
        (oldDims.z + factor - 1) / factor
    );
    
    auto newGrid = std::make_unique<VoxelData::VoxelGrid>(
        grid.getResolution(),
        grid.getWorkspaceSize()
    );
    
    // Downsample by taking majority vote in each block
    for (int z = 0; z < newDims.z; ++z) {
        for (int y = 0; y < newDims.y; ++y) {
            for (int x = 0; x < newDims.x; ++x) {
                int filledCount = 0;
                int totalCount = 0;
                
                // Sample block
                for (int dz = 0; dz < factor; ++dz) {
                    for (int dy = 0; dy < factor; ++dy) {
                        for (int dx = 0; dx < factor; ++dx) {
                            Math::Vector3i oldPos(
                                x * factor + dx,
                                y * factor + dy,
                                z * factor + dz
                            );
                            
                            if (oldPos.x < oldDims.x && 
                                oldPos.y < oldDims.y && 
                                oldPos.z < oldDims.z) {
                                if (grid.getVoxel(oldPos)) {
                                    filledCount++;
                                }
                                totalCount++;
                            }
                        }
                    }
                }
                
                // Set voxel if majority are filled
                if (filledCount > totalCount / 2) {
                    newGrid->setVoxel(Math::Vector3i(x, y, z), true);
                }
            }
        }
    }
    
    return newGrid;
}

void MeshCache::CacheEntry::updateAccess() {
    lastAccess = std::chrono::steady_clock::now();
}

// MeshCache implementation
MeshCache::MeshCache() 
    : m_maxMemoryUsage(256 * 1024 * 1024) // 256MB default
    , m_currentMemoryUsage(0)
    , m_hitCount(0)
    , m_missCount(0) {
}

MeshCache::~MeshCache() = default;

bool MeshCache::hasCachedMesh(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    return m_cache.find(key) != m_cache.end();
}

Mesh MeshCache::getCachedMesh(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_hitCount++;
        it->second.updateAccess();
        return it->second.mesh;
    }
    
    m_missCount++;
    return Mesh();
}

void MeshCache::cacheMesh(const std::string& key, const Mesh& mesh) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    size_t meshSize = mesh.getMemoryUsage();
    
    // Evict entries if necessary
    while (m_currentMemoryUsage + meshSize > m_maxMemoryUsage && !m_cache.empty()) {
        evictLRU();
    }
    
    // Add new entry
    CacheEntry entry;
    entry.mesh = mesh;
    entry.updateAccess();
    entry.memoryUsage = meshSize;
    entry.bounds = mesh.bounds;
    
    m_cache[key] = std::move(entry);
    m_currentMemoryUsage += meshSize;
}

void MeshCache::invalidateRegion(const Math::BoundingBox& region) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    auto it = m_cache.begin();
    while (it != m_cache.end()) {
        if (it->second.bounds.intersects(region)) {
            m_currentMemoryUsage -= it->second.memoryUsage;
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
}

void MeshCache::clear() {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_cache.clear();
    m_currentMemoryUsage = 0;
}

float MeshCache::getHitRate() const {
    size_t total = m_hitCount + m_missCount;
    return (total > 0) ? static_cast<float>(m_hitCount) / total : 0.0f;
}

void MeshCache::evictLRU() {
    if (m_cache.empty()) return;
    
    // Find oldest entry
    auto oldest = m_cache.begin();
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it->second.lastAccess < oldest->second.lastAccess) {
            oldest = it;
        }
    }
    
    m_currentMemoryUsage -= oldest->second.memoryUsage;
    m_cache.erase(oldest);
}

}
}