#include "SurfaceGenerator.h"
#include "DualContouring.h"
#include "SimpleMesher.h"
#include <thread>
#include <sstream>
#include <algorithm>
#include <atomic>
#include <memory>
#include <future>

namespace VoxelEditor {
namespace SurfaceGen {

// SurfaceGenerator implementation
SurfaceGenerator::SurfaceGenerator(Events::EventDispatcher* eventDispatcher)
    : m_eventDispatcher(eventDispatcher)
    , m_lodEnabled(true)
    , m_cacheEnabled(true)
    , m_cancelRequested(false) {
    
    m_dualContouring = std::make_unique<DualContouring>();
    m_simpleMesher = std::make_unique<SimpleMesher>();
    m_meshBuilder = std::make_unique<MeshBuilder>();
    m_lodManager = std::make_unique<LODManager>();
    m_meshCache = std::make_unique<MeshCache>();
    m_progressiveCache = std::make_unique<ProgressiveSmoothingCache>();
    
    // Set default cache size to 256MB
    m_meshCache->setMaxMemoryUsage(256 * 1024 * 1024);
    
    // Set progressive cache size to 64MB for real-time preview
    m_progressiveCache->setMaxMemoryUsage(64 * 1024 * 1024);
    
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
    // Find all resolutions with data - check ALL resolutions, not just up to targetRes
    std::vector<VoxelData::VoxelResolution> activeResolutions;
    for (int res = static_cast<int>(VoxelData::VoxelResolution::Size_1cm); 
         res <= static_cast<int>(VoxelData::VoxelResolution::Size_512cm); ++res) {
        auto resolution = static_cast<VoxelData::VoxelResolution>(res);
        const VoxelData::VoxelGrid* grid = voxelManager.getGrid(resolution);
        if (grid && !grid->isEmpty()) {
            activeResolutions.push_back(resolution);
        }
    }
    
    if (activeResolutions.empty()) {
        return Mesh();
    }
    
    // Generate meshes for each active resolution
    std::vector<Mesh> meshes;
    for (auto resolution : activeResolutions) {
        const VoxelData::VoxelGrid* grid = voxelManager.getGrid(resolution);
        if (grid) {
            // DEBUG: Log which resolution we're processing
            float voxelSizeCm = VoxelData::getVoxelSize(resolution) * 100.0f;
            Logging::Logger::getInstance().debugfc("SurfaceGenerator",
                "Generating mesh for resolution %d (%gcm voxels)",
                static_cast<int>(resolution), voxelSizeCm);
            
            Mesh mesh = generateSurface(*grid, m_settings);
            if (mesh.isValid()) {
                meshes.push_back(mesh);
                
                // DEBUG: Log mesh details
                Logging::Logger::getInstance().debugfc("SurfaceGenerator",
                    "Generated mesh with %zu vertices for %gcm resolution",
                    mesh.vertices.size(), voxelSizeCm);
            }
        }
    }
    
    // Combine meshes
    if (meshes.size() > 1) {
        return MeshBuilder::combineMeshes(meshes);
    } else if (!meshes.empty()) {
        return meshes[0];
    }
    
    return Mesh();
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
            exportSettings.smoothingLevel = 1;
            exportSettings.simplificationRatio = 0.5f;
            break;
        case ExportQuality::Standard:
            exportSettings.smoothingLevel = 2;
            exportSettings.simplificationRatio = 0.75f;
            break;
        case ExportQuality::High:
            exportSettings.smoothingLevel = 3;
            exportSettings.simplificationRatio = 0.9f;
            break;
        case ExportQuality::Maximum:
            exportSettings.smoothingLevel = 5;
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
        if (settings.smoothingLevel == 0) {
            // Use SimpleMesher for unsmoothed box meshes
            Logging::Logger::getInstance().debug("Generating box mesh with SimpleMesher", "SurfaceGenerator");
            
            // Map voxel grid resolution to mesh resolution
            // The mesh resolution should match the voxel resolution for accurate representation
            SimpleMesher::MeshResolution meshRes = SimpleMesher::MeshResolution::Res_8cm;
            VoxelData::VoxelResolution voxelRes = grid.getResolution();
            
            // Convert voxel resolution to mesh resolution
            switch (voxelRes) {
                case VoxelData::VoxelResolution::Size_1cm:
                    meshRes = SimpleMesher::MeshResolution::Res_1cm;
                    break;
                case VoxelData::VoxelResolution::Size_2cm:
                    meshRes = SimpleMesher::MeshResolution::Res_2cm;
                    break;
                case VoxelData::VoxelResolution::Size_4cm:
                    meshRes = SimpleMesher::MeshResolution::Res_4cm;
                    break;
                case VoxelData::VoxelResolution::Size_8cm:
                    meshRes = SimpleMesher::MeshResolution::Res_8cm;
                    break;
                case VoxelData::VoxelResolution::Size_16cm:
                    meshRes = SimpleMesher::MeshResolution::Res_16cm;
                    break;
                case VoxelData::VoxelResolution::Size_32cm:
                case VoxelData::VoxelResolution::Size_64cm:
                case VoxelData::VoxelResolution::Size_128cm:
                case VoxelData::VoxelResolution::Size_256cm:
                case VoxelData::VoxelResolution::Size_512cm:
                    // For larger voxels, use 16cm mesh resolution as SimpleMesher doesn't support larger
                    meshRes = SimpleMesher::MeshResolution::Res_16cm;
                    break;
                default:
                    meshRes = SimpleMesher::MeshResolution::Res_8cm;
                    break;
            }
            
            // Set progress callback
            m_simpleMesher->setProgressCallback([this](float progress) {
                reportProgress(progress * 0.8f, "Generating box mesh");
            });
            
            mesh = m_simpleMesher->generateMesh(grid, settings, meshRes);
        } else {
            // Use DualContouring for smoothed meshes
            Logging::Logger::getInstance().debug("Generating smooth mesh with DualContouring", "SurfaceGenerator");
            mesh = m_dualContouring->generateMesh(grid, settings);
        }
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
    
    // Apply smoothing if requested
    if (settings.smoothingLevel > 0) {
        applySmoothingToMesh(mesh, settings);
    }
    
    Logging::Logger::getInstance().debugfc("SurfaceGenerator", 
        "Post-processing complete: %zu vertices, %zu UVs", 
        mesh.vertices.size(), mesh.uvCoords.size());
    
    // Apply simplification (but skip if UVs were generated to preserve them)
    if (settings.simplificationRatio < 1.0f && !settings.generateUVs) {
        optimizeMesh(mesh, settings.simplificationRatio);
    }
    
    // Validate mesh for 3D printing if smoothing was applied
    if (settings.smoothingLevel > 0) {
        validateMeshForPrinting(mesh, settings);
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

Mesh SurfaceGenerator::generateSmoothedSurface(const VoxelData::VoxelGrid& grid, int smoothingLevel) {
    // Create settings optimized for smooth toy-like output
    SurfaceSettings settings = SurfaceSettings::Export();
    settings.smoothingLevel = smoothingLevel;
    settings.smoothingAlgorithm = SmoothingAlgorithm::Auto;
    settings.preserveTopology = true;
    settings.minFeatureSize = 1.0f;  // 1mm minimum for 3D printing
    settings.usePreviewQuality = false;
    settings.generateNormals = true;
    settings.simplificationRatio = 1.0f;  // Don't simplify smoothed meshes
    
    return generateSurface(grid, settings);
}

void SurfaceGenerator::applySmoothingToMesh(Mesh& mesh, const SurfaceSettings& settings) {
    if (settings.smoothingLevel <= 0) return;
    
    MeshSmoother smoother;
    MeshSmoother::SmoothingConfig smoothConfig;
    
    // Configure smoothing based on settings
    smoothConfig.smoothingLevel = settings.smoothingLevel;
    smoothConfig.algorithm = (settings.smoothingAlgorithm == SmoothingAlgorithm::Auto) ?
                           MeshSmoother::Algorithm::None : 
                           static_cast<MeshSmoother::Algorithm>(static_cast<int>(settings.smoothingAlgorithm));
    smoothConfig.preserveTopology = settings.preserveTopology;
    smoothConfig.preserveBoundaries = true; // Always preserve boundaries for 3D printing
    smoothConfig.minFeatureSize = settings.minFeatureSize;
    smoothConfig.previewQuality = settings.previewQuality;
    smoothConfig.usePreviewQuality = settings.usePreviewQuality; // Backward compatibility
    
    // Progress callback for smoothing
    auto smoothingProgress = [this](float progress) -> bool {
        reportProgress(0.5f + progress * 0.3f, "Smoothing mesh");
        return !m_cancelRequested;
    };
    
    // Apply smoothing
    Mesh smoothedMesh = smoother.smooth(mesh, smoothConfig, smoothingProgress);
    
    if (smoothedMesh.isValid()) {
        mesh = std::move(smoothedMesh);
        Logging::Logger::getInstance().info("Applied smoothing level " + 
                                          std::to_string(settings.smoothingLevel) + 
                                          " to mesh", "SurfaceGenerator");
    } else {
        Logging::Logger::getInstance().warning("Smoothing failed, keeping original mesh", 
                                             "SurfaceGenerator");
    }
}

void SurfaceGenerator::validateMeshForPrinting(Mesh& mesh, const SurfaceSettings& settings) {
    MeshValidator validator;
    
    auto result = validator.validate(mesh, settings.minFeatureSize);
    
    if (!result.isValid) {
        Logging::Logger::getInstance().warning("Mesh validation failed:", "SurfaceGenerator");
        for (const auto& error : result.errors) {
            Logging::Logger::getInstance().warning("  - " + error, "SurfaceGenerator");
        }
        
        // Attempt basic repairs
        if (!result.isWatertight) {
            MeshUtils::makeWatertight(mesh);
            Logging::Logger::getInstance().info("Applied watertight repair", "SurfaceGenerator");
        }
        
        if (result.flippedNormals > 0) {
            validator.fixFaceOrientation(mesh);
            Logging::Logger::getInstance().info("Fixed face orientations", "SurfaceGenerator");
        }
    } else {
        Logging::Logger::getInstance().debug("Mesh passed validation for 3D printing", 
                                          "SurfaceGenerator");
    }
    
    // Log warnings
    for (const auto& warning : result.warnings) {
        Logging::Logger::getInstance().warning(warning, "SurfaceGenerator");
    }
}

std::string SurfaceGenerator::startProgressiveSmoothing(const VoxelData::VoxelGrid& grid, 
                                                       const SurfaceSettings& settings) {
    // Generate base cache key from grid hash and settings
    size_t gridHash = computeGridHash(grid);
    std::string baseKey = getCacheKey(gridHash, settings, LODLevel::LOD0);
    
    // Start progressive smoothing operation
    std::string progressKey = m_progressiveCache->startProgressiveSmoothing(baseKey, 
                                                                           settings.smoothingLevel, 
                                                                           settings.previewQuality);
    
    // Start background task to generate progressive results
    // Note: We need to capture grid by value despite copy restrictions, so let's work around this
    // by generating the initial mesh immediately and then doing progressive smoothing
    
    // Generate base mesh (without smoothing) immediately
    SurfaceSettings baseSettings = settings;
    baseSettings.smoothingLevel = 0;
    baseSettings.previewQuality = PreviewQuality::Disabled;
    
    Mesh baseMesh = generateInternal(grid, baseSettings, LODLevel::LOD0);
    if (!baseMesh.isValid()) {
        m_progressiveCache->cancelProgressiveSmoothing(progressKey);
        return progressKey;
    }
    
    auto progressTask = std::async(std::launch::async, [this, baseMesh, settings, progressKey, baseKey]() {
        try {
            if (m_cancelRequested) {
                m_progressiveCache->cancelProgressiveSmoothing(progressKey);
                return;
            }
            
            // Apply progressive smoothing if requested
            if (settings.smoothingLevel > 0) {
                MeshSmoother smoother;
                
                // Progressive smoothing: start with fast preview and gradually improve
                std::vector<PreviewQuality> qualityLevels;
                switch (settings.previewQuality) {
                    case PreviewQuality::Fast:
                        qualityLevels = {PreviewQuality::Fast};
                        break;
                    case PreviewQuality::Balanced:
                        qualityLevels = {PreviewQuality::Fast, PreviewQuality::Balanced};
                        break;
                    case PreviewQuality::HighQuality:
                        qualityLevels = {PreviewQuality::Fast, PreviewQuality::Balanced, PreviewQuality::HighQuality};
                        break;
                    default:
                        qualityLevels = {PreviewQuality::Fast, PreviewQuality::Balanced};
                        break;
                }
                
                for (PreviewQuality quality : qualityLevels) {
                    if (m_cancelRequested) {
                        m_progressiveCache->cancelProgressiveSmoothing(progressKey);
                        return;
                    }
                    
                    // Configure smoothing for this quality level
                    MeshSmoother::SmoothingConfig smoothConfig;
                    smoothConfig.smoothingLevel = settings.smoothingLevel;
                    smoothConfig.algorithm = (settings.smoothingAlgorithm == SmoothingAlgorithm::Auto) ?
                                           MeshSmoother::Algorithm::None : 
                                           static_cast<MeshSmoother::Algorithm>(static_cast<int>(settings.smoothingAlgorithm));
                    smoothConfig.preserveTopology = settings.preserveTopology;
                    smoothConfig.preserveBoundaries = true;
                    smoothConfig.minFeatureSize = settings.minFeatureSize;
                    smoothConfig.previewQuality = quality;
                    
                    // Progress callback for smoothing
                    auto smoothingProgress = [this, progressKey](float progress) -> bool {
                        if (m_progressCallback) {
                            m_progressCallback(0.5f + progress * 0.5f, "Progressive smoothing");
                        }
                        return !m_cancelRequested;
                    };
                    
                    // Apply smoothing
                    Mesh smoothedMesh = smoother.smooth(baseMesh, smoothConfig, smoothingProgress);
                    
                    if (smoothedMesh.isValid() && !m_cancelRequested) {
                        // Update progressive result
                        bool isProgressive = (quality != qualityLevels.back());
                        m_progressiveCache->updateProgressiveResult(progressKey, smoothedMesh, settings.smoothingLevel);
                        
                        if (!isProgressive) {
                            // This is the final result
                            m_progressiveCache->finalizeProgressiveResult(progressKey, smoothedMesh);
                        }
                    } else if (m_cancelRequested) {
                        m_progressiveCache->cancelProgressiveSmoothing(progressKey);
                        return;
                    }
                }
            } else {
                // No smoothing requested, just cache the base mesh
                m_progressiveCache->finalizeProgressiveResult(progressKey, baseMesh);
            }
            
        } catch (...) {
            // Cancel on any exception
            m_progressiveCache->cancelProgressiveSmoothing(progressKey);
        }
    });
    
    // Store the future (we don't wait for it)
    m_activeProgressiveGenerations.push_back(std::move(progressTask));
    
    return progressKey;
}

Mesh SurfaceGenerator::getProgressiveResult(const std::string& progressKey) {
    std::lock_guard<std::mutex> lock(m_generationMutex);
    
    // Clean up completed futures first
    m_activeProgressiveGenerations.erase(
        std::remove_if(m_activeProgressiveGenerations.begin(), m_activeProgressiveGenerations.end(),
            [](std::future<void>& f) {
                return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
            }),
        m_activeProgressiveGenerations.end()
    );
    
    // Try to get result from progressive cache
    if (m_progressiveCache->hasEntry(progressKey)) {
        auto entry = m_progressiveCache->getEntry(progressKey);
        return entry.mesh;
    }
    
    return Mesh();
}

void SurfaceGenerator::cancelProgressiveSmoothing(const std::string& progressKey) {
    m_cancelRequested = true;
    m_progressiveCache->cancelProgressiveSmoothing(progressKey);
}

bool SurfaceGenerator::isProgressiveSmoothingComplete(const std::string& progressKey) {
    if (m_progressiveCache->hasEntry(progressKey)) {
        auto entry = m_progressiveCache->getEntry(progressKey);
        return !entry.isProgressive;
    }
    return false;
}

// ProgressiveSmoothingCache implementation
ProgressiveSmoothingCache::ProgressiveSmoothingCache()
    : m_maxMemoryUsage(64 * 1024 * 1024) // 64MB default for preview cache
    , m_currentMemoryUsage(0) {
}

ProgressiveSmoothingCache::~ProgressiveSmoothingCache() = default;

bool ProgressiveSmoothingCache::hasProgressiveResult(const std::string& baseKey, int targetLevel, PreviewQuality quality) const {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    std::string cacheKey = generateCacheKey(baseKey, targetLevel, quality);
    return m_cache.find(cacheKey) != m_cache.end();
}

Mesh ProgressiveSmoothingCache::getProgressiveResult(const std::string& baseKey, int targetLevel, PreviewQuality quality) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    std::string cacheKey = generateCacheKey(baseKey, targetLevel, quality);
    
    auto it = m_cache.find(cacheKey);
    if (it != m_cache.end()) {
        it->second.timestamp = std::chrono::steady_clock::now();
        return it->second.mesh;
    }
    
    return Mesh();
}

void ProgressiveSmoothingCache::cacheProgressiveResult(const std::string& baseKey, const Mesh& mesh, 
                                                     int smoothingLevel, PreviewQuality quality, bool isProgressive) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    std::string cacheKey = generateCacheKey(baseKey, smoothingLevel, quality);
    
    size_t meshSize = mesh.getMemoryUsage();
    
    // Evict entries if necessary
    while (m_currentMemoryUsage + meshSize > m_maxMemoryUsage && !m_cache.empty()) {
        evictLRU();
    }
    
    CacheEntry entry;
    entry.mesh = mesh;
    entry.smoothingLevel = smoothingLevel;
    entry.quality = quality;
    entry.timestamp = std::chrono::steady_clock::now();
    entry.isProgressive = isProgressive;
    
    m_cache[cacheKey] = std::move(entry);
    m_currentMemoryUsage += meshSize;
}

std::string ProgressiveSmoothingCache::startProgressiveSmoothing(const std::string& baseKey, int targetLevel, PreviewQuality quality) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    // Generate unique progress key
    static std::atomic<int> progressCounter{0};
    std::string progressKey = "progress_" + std::to_string(progressCounter.fetch_add(1)) + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    
    m_progressiveKeys[progressKey] = baseKey;
    
    return progressKey;
}

void ProgressiveSmoothingCache::updateProgressiveResult(const std::string& progressKey, const Mesh& mesh, int currentLevel) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    auto it = m_progressiveKeys.find(progressKey);
    if (it != m_progressiveKeys.end()) {
        const std::string& baseKey = it->second;
        
        // Store intermediate result
        CacheEntry entry;
        entry.mesh = mesh;
        entry.smoothingLevel = currentLevel;
        entry.quality = PreviewQuality::Fast; // Intermediate results are fast quality
        entry.timestamp = std::chrono::steady_clock::now();
        entry.isProgressive = true;
        
        m_cache[progressKey] = std::move(entry);
    }
}

void ProgressiveSmoothingCache::finalizeProgressiveResult(const std::string& progressKey, const Mesh& finalMesh) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    auto it = m_progressiveKeys.find(progressKey);
    if (it != m_progressiveKeys.end()) {
        // Update the final result
        auto cacheIt = m_cache.find(progressKey);
        if (cacheIt != m_cache.end()) {
            cacheIt->second.mesh = finalMesh;
            cacheIt->second.isProgressive = false;
            cacheIt->second.timestamp = std::chrono::steady_clock::now();
        }
    }
}

void ProgressiveSmoothingCache::cancelProgressiveSmoothing(const std::string& progressKey) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    // Remove from both maps
    m_progressiveKeys.erase(progressKey);
    auto it = m_cache.find(progressKey);
    if (it != m_cache.end()) {
        m_currentMemoryUsage -= it->second.mesh.getMemoryUsage();
        m_cache.erase(it);
    }
}

void ProgressiveSmoothingCache::clear() {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_cache.clear();
    m_progressiveKeys.clear();
    m_currentMemoryUsage = 0;
}

void ProgressiveSmoothingCache::clearExpired(std::chrono::seconds maxAge) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    auto now = std::chrono::steady_clock::now();
    auto it = m_cache.begin();
    
    while (it != m_cache.end()) {
        if (now - it->second.timestamp > maxAge) {
            m_currentMemoryUsage -= it->second.mesh.getMemoryUsage();
            
            // Also remove from progressive keys if it's a progressive entry
            auto progressIt = std::find_if(m_progressiveKeys.begin(), m_progressiveKeys.end(),
                [&](const auto& pair) { return pair.first == it->first; });
            if (progressIt != m_progressiveKeys.end()) {
                m_progressiveKeys.erase(progressIt);
            }
            
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
}

void ProgressiveSmoothingCache::evictLRU() {
    if (m_cache.empty()) return;
    
    // Find oldest entry
    auto oldest = m_cache.begin();
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it->second.timestamp < oldest->second.timestamp) {
            oldest = it;
        }
    }
    
    m_currentMemoryUsage -= oldest->second.mesh.getMemoryUsage();
    
    // Also remove from progressive keys if it's a progressive entry
    auto progressIt = std::find_if(m_progressiveKeys.begin(), m_progressiveKeys.end(),
        [&](const auto& pair) { return pair.first == oldest->first; });
    if (progressIt != m_progressiveKeys.end()) {
        m_progressiveKeys.erase(progressIt);
    }
    
    m_cache.erase(oldest);
}

std::string ProgressiveSmoothingCache::generateCacheKey(const std::string& baseKey, int level, PreviewQuality quality) const {
    return baseKey + "_level" + std::to_string(level) + "_quality" + std::to_string(static_cast<int>(quality));
}

bool ProgressiveSmoothingCache::hasEntry(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    return m_cache.find(key) != m_cache.end();
}

ProgressiveSmoothingCache::CacheEntry ProgressiveSmoothingCache::getEntry(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        // Update timestamp (const_cast because this is conceptually non-const but mutex is needed)
        const_cast<CacheEntry&>(it->second).timestamp = std::chrono::steady_clock::now();
        return it->second;
    }
    return CacheEntry{}; // Return empty entry if not found
}

}
}