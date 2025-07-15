#pragma once

#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/Vector2f.h"
#include "../../foundation/math/Vector3i.h"
#include "../../foundation/math/Vector4f.h"
#include "../../foundation/math/Matrix4f.h"
#include "../../foundation/math/BoundingBox.h"
#include "../../foundation/math/CoordinateTypes.h"
#include "../../foundation/math/CoordinateConverter.h"
#include "../../foundation/voxel_math/include/voxel_math/VoxelMath.h"
#include "../voxel_data/VoxelTypes.h"
#include <vector>
#include <cstdint>
#include <chrono>

namespace VoxelEditor {
namespace SurfaceGen {

// Forward declarations
struct Mesh;
struct SurfaceSettings;

// Export quality levels
enum class ExportQuality {
    Draft,      // Fast generation, lower quality
    Standard,   // Balanced quality/performance
    High,       // High quality, slower generation
    Maximum     // Best possible quality
};

// Level of detail levels
enum class LODLevel {
    LOD0 = 0,  // Full resolution (100%)
    LOD1 = 1,  // Half resolution (50%)
    LOD2 = 2,  // Quarter resolution (25%)
    LOD3 = 3,  // Eighth resolution (12.5%)
    LOD4 = 4   // Sixteenth resolution (6.25%)
};

// Material ID for mesh sections
using MaterialId = uint32_t;

// Mesh data structure
struct Mesh {
    std::vector<Math::WorldCoordinates> vertices;
    std::vector<Math::Vector3f> normals;  // Normals are direction vectors, not positions
    std::vector<Math::Vector2f> uvCoords;
    std::vector<uint32_t> indices;
    Math::BoundingBox bounds;  // Will need to use WorldCoordinates bounds
    MaterialId materialId = 0;
    
    // Utility functions
    void calculateNormals();
    void calculateBounds();
    bool isValid() const;
    size_t getMemoryUsage() const;
    size_t getVertexCount() const { return vertices.size(); }
    size_t getTriangleCount() const { return indices.size() / 3; }
    
    // Clear all data
    void clear();
    
    // Reserve memory
    void reserve(size_t vertexCount, size_t indexCount);
    
    // Transform mesh
    void transform(const Math::Matrix4f& matrix);
};

// Smoothing algorithms available
enum class SmoothingAlgorithm {
    Auto,        // Automatically select based on smoothing level
    Laplacian,   // Basic smoothing (levels 1-3)
    Taubin,      // Feature-preserving smoothing (levels 4-7)
    BiLaplacian  // Aggressive smoothing (levels 8-10+)
};

// Preview quality levels for real-time updates
enum class PreviewQuality {
    Disabled,    // No preview mode optimizations
    Fast,        // Fastest preview: minimal iterations, simplified algorithm
    Balanced,    // Balanced preview: moderate quality with good performance  
    HighQuality  // High quality preview: near-final quality with slight optimizations
};

// Surface generation settings
struct SurfaceSettings {
    float adaptiveError = 0.01f;        // Error threshold for adaptive subdivision
    bool generateNormals = true;        // Generate vertex normals
    bool generateUVs = false;           // Generate UV coordinates
    int smoothingIterations = 0;        // Number of smoothing iterations (deprecated - use smoothingLevel)
    float simplificationRatio = 1.0f;   // Mesh simplification ratio (0-1)
    bool preserveSharpFeatures = true;  // Preserve sharp features (deprecated - controlled by smoothing)
    float sharpFeatureAngle = 30.0f;    // Angle threshold for sharp features
    
    // New smoothing parameters
    int smoothingLevel = 0;             // Smoothing level (0-10+), 0 = no smoothing
    SmoothingAlgorithm smoothingAlgorithm = SmoothingAlgorithm::Auto; // Algorithm selection
    bool preserveTopology = true;       // Preserve holes and loops during smoothing
    float minFeatureSize = 1.0f;        // Minimum feature size in mm for 3D printing
    PreviewQuality previewQuality = PreviewQuality::Disabled; // Preview optimization level
    bool usePreviewQuality = false;     // Deprecated: use previewQuality instead
    
    static SurfaceSettings Default();
    static SurfaceSettings Preview();
    static SurfaceSettings Export();
    
    // Convenience methods for different preview quality levels
    static SurfaceSettings FastPreview();      // Fastest preview mode
    static SurfaceSettings BalancedPreview();  // Balanced preview mode  
    static SurfaceSettings HighQualityPreview(); // High quality preview mode
    
    bool operator==(const SurfaceSettings& other) const;
    bool operator!=(const SurfaceSettings& other) const { return !(*this == other); }
    
    // Hash for cache key
    size_t hash() const;
};

// Hermite data for dual contouring
struct HermiteData {
    Math::WorldCoordinates position;  // World space position
    Math::Vector3f normal;            // Direction vector
    float value;
    bool hasIntersection;
    
    HermiteData() : value(0.0f), hasIntersection(false) {}
};

// Edge data for dual contouring
struct EdgeData {
    Math::IncrementCoordinates v0;  // Start voxel position in increment space
    Math::IncrementCoordinates v1;  // End voxel position in increment space
    HermiteData hermite;
    int direction;  // 0=X, 1=Y, 2=Z
    
    EdgeData() : direction(0) {}
};

// Quad face for mesh generation
struct QuadFace {
    uint32_t vertices[4];
    Math::Vector3f normal;
    MaterialId material;
    
    QuadFace() : material(0) {
        vertices[0] = vertices[1] = vertices[2] = vertices[3] = 0;
    }
};

// Mesh generation event types
enum class MeshGenerationEventType {
    Started,
    Progress,
    Completed,
    Failed,
    Cancelled
};

// Mesh generation event
struct MeshGenerationEvent {
    MeshGenerationEventType type;
    float progress;  // 0.0 to 1.0
    std::string message;
    VoxelData::VoxelResolution resolution;
    LODLevel lodLevel;
    
    MeshGenerationEvent(MeshGenerationEventType t = MeshGenerationEventType::Started) 
        : type(t), progress(0.0f), resolution(VoxelData::VoxelResolution::Size_1cm), 
          lodLevel(LODLevel::LOD0) {}
};

// Cache entry for mesh cache
struct MeshCacheEntry {
    Mesh mesh;
    std::chrono::steady_clock::time_point lastAccess;
    size_t memoryUsage;
    SurfaceSettings settings;
    
    void updateAccess() {
        lastAccess = std::chrono::steady_clock::now();
    }
};

// Hash for voxel grid (for caching)
struct VoxelGridHash {
    size_t hash;
    Math::BoundingBox bounds;
    VoxelData::VoxelResolution resolution;
    
    bool operator==(const VoxelGridHash& other) const {
        return hash == other.hash && bounds == other.bounds && resolution == other.resolution;
    }
};

// Vertex attributes for advanced rendering
struct VertexAttributes {
    Math::WorldCoordinates position;  // World space position
    Math::Vector3f normal;            // Direction vector
    Math::Vector2f uv;
    Math::Vector3f tangent;           // Direction vector
    Math::Vector3f bitangent;         // Direction vector
    MaterialId material;
};

// Mesh statistics
struct MeshStats {
    size_t vertexCount = 0;
    size_t triangleCount = 0;
    size_t degenerateTriangles = 0;
    size_t duplicateVertices = 0;
    float surfaceArea = 0.0f;
    float volume = 0.0f;
    bool isManifold = true;
    bool isWatertight = true;
    Math::BoundingBox bounds;
    
    void clear() {
        *this = MeshStats();
    }
};

// Simplification settings
struct SimplificationSettings {
    float targetRatio = 0.5f;  // Target triangle count ratio
    float maxError = 0.01f;    // Maximum allowed error
    bool preserveBoundary = true;
    bool preserveTopology = true;
    bool preserveUVs = true;
    
    static SimplificationSettings Aggressive();
    static SimplificationSettings Conservative();
    static SimplificationSettings Balanced();
    static SimplificationSettings Quality();
};

}
}