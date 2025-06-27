#include "SurfaceTypes.h"
#include <numeric>
#include <cmath>
#include <algorithm>

namespace VoxelEditor {
namespace SurfaceGen {

// Mesh implementation
void Mesh::calculateNormals() {
    // Clear existing normals
    normals.clear();
    normals.resize(vertices.size(), Math::Vector3f(0, 0, 0));
    
    // Calculate face normals and accumulate at vertices
    for (size_t i = 0; i < indices.size(); i += 3) {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];
        
        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
            continue;
        }
        
        const Math::WorldCoordinates& v0 = vertices[i0];
        const Math::WorldCoordinates& v1 = vertices[i1];
        const Math::WorldCoordinates& v2 = vertices[i2];
        
        // Calculate edge vectors in world space
        Math::WorldCoordinates edge1 = v1 - v0;
        Math::WorldCoordinates edge2 = v2 - v0;
        
        // Cross product gives face normal (convert to direction vector)
        Math::Vector3f faceNormal = edge1.value().cross(edge2.value());
        
        normals[i0] = normals[i0] + faceNormal;
        normals[i1] = normals[i1] + faceNormal;
        normals[i2] = normals[i2] + faceNormal;
    }
    
    // Normalize all vertex normals
    for (auto& normal : normals) {
        float length = normal.length();
        if (length > 0.0001f) {
            normal = normal / length;
        } else {
            normal = Math::Vector3f(0, 1, 0); // Default up vector
        }
    }
}

void Mesh::calculateBounds() {
    if (vertices.empty()) {
        bounds = Math::BoundingBox();
        return;
    }
    
    bounds.min = bounds.max = vertices[0].value();
    
    for (const auto& vertex : vertices) {
        const Math::Vector3f& pos = vertex.value();
        bounds.min.x = std::min(bounds.min.x, pos.x);
        bounds.min.y = std::min(bounds.min.y, pos.y);
        bounds.min.z = std::min(bounds.min.z, pos.z);
        
        bounds.max.x = std::max(bounds.max.x, pos.x);
        bounds.max.y = std::max(bounds.max.y, pos.y);
        bounds.max.z = std::max(bounds.max.z, pos.z);
    }
}

bool Mesh::isValid() const {
    // Empty mesh is valid
    if (vertices.empty() && indices.empty()) {
        return true;
    }
    
    // Check basic validity
    if (indices.empty()) {
        return true; // Just vertices is valid
    }
    
    // Check if indices are valid
    uint32_t maxIndex = static_cast<uint32_t>(vertices.size() - 1);
    for (uint32_t index : indices) {
        if (index > maxIndex) {
            return false;
        }
    }
    
    // Check if we have complete triangles
    if (indices.size() % 3 != 0) {
        return false;
    }
    
    // Check normals if they exist
    if (!normals.empty() && normals.size() != vertices.size()) {
        return false;
    }
    
    // Check UVs if they exist
    if (!uvCoords.empty() && uvCoords.size() != vertices.size()) {
        return false;
    }
    
    return true;
}

size_t Mesh::getMemoryUsage() const {
    size_t usage = sizeof(*this);
    usage += vertices.capacity() * sizeof(Math::WorldCoordinates);
    usage += normals.capacity() * sizeof(Math::Vector3f);
    usage += uvCoords.capacity() * sizeof(Math::Vector2f);
    usage += indices.capacity() * sizeof(uint32_t);
    return usage;
}

void Mesh::clear() {
    vertices.clear();
    normals.clear();
    uvCoords.clear();
    indices.clear();
    bounds = Math::BoundingBox();
    materialId = 0;
}

void Mesh::reserve(size_t vertexCount, size_t indexCount) {
    vertices.reserve(vertexCount);
    normals.reserve(vertexCount);
    indices.reserve(indexCount);
}

// Mesh transformation
void Mesh::transform(const Math::Matrix4f& matrix) {
    // Transform vertices
    for (auto& vertex : vertices) {
        const Math::Vector3f& pos = vertex.value();
        Math::Vector4f v4(pos.x, pos.y, pos.z, 1.0f);
        Math::Vector4f transformed = matrix * v4;
        vertex = Math::WorldCoordinates(Math::Vector3f(transformed.x, transformed.y, transformed.z));
    }
    
    // Transform normals (use inverse transpose for correct normal transformation)
    if (!normals.empty()) {
        // For affine transformations, we can use the upper 3x3 part of the inverse transpose
        // Extract the 3x3 rotation/scale part
        Math::Matrix4f upper3x3 = matrix;
        upper3x3.m[3] = upper3x3.m[7] = upper3x3.m[11] = 0.0f;
        upper3x3.m[12] = upper3x3.m[13] = upper3x3.m[14] = 0.0f;
        upper3x3.m[15] = 1.0f;
        
        // For now, use the matrix directly but normalize afterwards
        // This works correctly for orthogonal transformations (rotations)
        // and reasonably well for uniform scaling
        for (auto& normal : normals) {
            Math::Vector4f n4(normal.x, normal.y, normal.z, 0.0f);
            Math::Vector4f transformed = upper3x3 * n4;
            normal = Math::Vector3f(transformed.x, transformed.y, transformed.z).normalized();
        }
    }
    
    // Recalculate bounds
    calculateBounds();
}

// SurfaceSettings implementation
SurfaceSettings SurfaceSettings::Default() {
    SurfaceSettings settings;
    settings.adaptiveError = 0.01f;
    settings.generateNormals = true;
    settings.generateUVs = false;
    settings.smoothingIterations = 0;
    settings.simplificationRatio = 1.0f;
    settings.preserveSharpFeatures = true;
    settings.sharpFeatureAngle = 30.0f;
    // New smoothing defaults
    settings.smoothingLevel = 0;  // No smoothing by default
    settings.smoothingAlgorithm = SmoothingAlgorithm::Auto;
    settings.preserveTopology = true;
    settings.minFeatureSize = 1.0f;
    settings.previewQuality = PreviewQuality::Disabled;
    settings.usePreviewQuality = false;  // Deprecated
    return settings;
}

SurfaceSettings SurfaceSettings::Preview() {
    SurfaceSettings settings;
    settings.adaptiveError = 0.05f;
    settings.generateNormals = false;
    settings.generateUVs = false;
    settings.smoothingIterations = 0;
    settings.simplificationRatio = 0.5f;
    settings.preserveSharpFeatures = false;
    settings.sharpFeatureAngle = 30.0f;
    // Preview quality smoothing
    settings.smoothingLevel = 3;  // Light smoothing for preview
    settings.smoothingAlgorithm = SmoothingAlgorithm::Auto;
    settings.preserveTopology = true;
    settings.minFeatureSize = 2.0f;  // Less strict for preview
    settings.previewQuality = PreviewQuality::Balanced;  // Balanced preview mode
    settings.usePreviewQuality = true;  // Deprecated but maintain compatibility
    return settings;
}

SurfaceSettings SurfaceSettings::Export() {
    SurfaceSettings settings;
    settings.adaptiveError = 0.001f;
    settings.generateNormals = true;
    settings.generateUVs = true;
    settings.smoothingIterations = 2;
    settings.simplificationRatio = 0.95f;
    settings.preserveSharpFeatures = true;
    settings.sharpFeatureAngle = 45.0f;
    // Export quality smoothing
    settings.smoothingLevel = 5;  // Medium smoothing for export
    settings.smoothingAlgorithm = SmoothingAlgorithm::Auto;
    settings.preserveTopology = true;
    settings.minFeatureSize = 1.0f;  // 3D printing standard
    settings.previewQuality = PreviewQuality::Disabled;  // Full quality for export
    settings.usePreviewQuality = false;  // Deprecated but maintain compatibility
    return settings;
}

SurfaceSettings SurfaceSettings::FastPreview() {
    SurfaceSettings settings;
    settings.adaptiveError = 0.1f;  // More relaxed for speed
    settings.generateNormals = false;  // Skip normals for speed
    settings.generateUVs = false;
    settings.smoothingIterations = 0;
    settings.simplificationRatio = 0.3f;  // Aggressive simplification
    settings.preserveSharpFeatures = false;
    settings.sharpFeatureAngle = 30.0f;
    // Fast preview smoothing
    settings.smoothingLevel = 2;  // Minimal smoothing
    settings.smoothingAlgorithm = SmoothingAlgorithm::Laplacian;  // Force fastest algorithm
    settings.preserveTopology = false;  // Skip topology preservation for speed
    settings.minFeatureSize = 3.0f;  // Very relaxed feature constraints
    settings.previewQuality = PreviewQuality::Fast;
    settings.usePreviewQuality = true;
    return settings;
}

SurfaceSettings SurfaceSettings::BalancedPreview() {
    SurfaceSettings settings;
    settings.adaptiveError = 0.05f;
    settings.generateNormals = false;  // Skip for performance
    settings.generateUVs = false;
    settings.smoothingIterations = 0;
    settings.simplificationRatio = 0.5f;
    settings.preserveSharpFeatures = false;
    settings.sharpFeatureAngle = 30.0f;
    // Balanced preview smoothing
    settings.smoothingLevel = 3;
    settings.smoothingAlgorithm = SmoothingAlgorithm::Auto;
    settings.preserveTopology = true;
    settings.minFeatureSize = 2.0f;
    settings.previewQuality = PreviewQuality::Balanced;
    settings.usePreviewQuality = true;
    return settings;
}

SurfaceSettings SurfaceSettings::HighQualityPreview() {
    SurfaceSettings settings;
    settings.adaptiveError = 0.02f;
    settings.generateNormals = true;  // Generate normals for better quality
    settings.generateUVs = false;  // Still skip UVs for performance
    settings.smoothingIterations = 0;
    settings.simplificationRatio = 0.8f;  // Minimal simplification
    settings.preserveSharpFeatures = true;
    settings.sharpFeatureAngle = 30.0f;
    // High quality preview smoothing
    settings.smoothingLevel = 4;
    settings.smoothingAlgorithm = SmoothingAlgorithm::Auto;
    settings.preserveTopology = true;
    settings.minFeatureSize = 1.5f;
    settings.previewQuality = PreviewQuality::HighQuality;
    settings.usePreviewQuality = true;
    return settings;
}

bool SurfaceSettings::operator==(const SurfaceSettings& other) const {
    return adaptiveError == other.adaptiveError &&
           generateNormals == other.generateNormals &&
           generateUVs == other.generateUVs &&
           smoothingIterations == other.smoothingIterations &&
           simplificationRatio == other.simplificationRatio &&
           preserveSharpFeatures == other.preserveSharpFeatures &&
           sharpFeatureAngle == other.sharpFeatureAngle &&
           smoothingLevel == other.smoothingLevel &&
           smoothingAlgorithm == other.smoothingAlgorithm &&
           preserveTopology == other.preserveTopology &&
           minFeatureSize == other.minFeatureSize &&
           previewQuality == other.previewQuality &&
           usePreviewQuality == other.usePreviewQuality;
}

size_t SurfaceSettings::hash() const {
    size_t h = 0;
    
    // Simple hash combination
    auto hashCombine = [&h](size_t value) {
        h ^= value + 0x9e3779b9 + (h << 6) + (h >> 2);
    };
    
    hashCombine(std::hash<float>{}(adaptiveError));
    hashCombine(std::hash<bool>{}(generateNormals));
    hashCombine(std::hash<bool>{}(generateUVs));
    hashCombine(std::hash<int>{}(smoothingIterations));
    hashCombine(std::hash<float>{}(simplificationRatio));
    hashCombine(std::hash<bool>{}(preserveSharpFeatures));
    hashCombine(std::hash<float>{}(sharpFeatureAngle));
    hashCombine(std::hash<int>{}(smoothingLevel));
    hashCombine(std::hash<int>{}(static_cast<int>(smoothingAlgorithm)));
    hashCombine(std::hash<bool>{}(preserveTopology));
    hashCombine(std::hash<float>{}(minFeatureSize));
    hashCombine(std::hash<int>{}(static_cast<int>(previewQuality)));
    hashCombine(std::hash<bool>{}(usePreviewQuality));
    
    return h;
}

// SimplificationSettings implementation
SimplificationSettings SimplificationSettings::Aggressive() {
    SimplificationSettings settings;
    settings.targetRatio = 0.25f;
    settings.maxError = 0.05f;
    settings.preserveBoundary = false;
    settings.preserveTopology = false;
    return settings;
}

SimplificationSettings SimplificationSettings::Conservative() {
    SimplificationSettings settings;
    settings.targetRatio = 0.75f;
    settings.maxError = 0.005f;
    settings.preserveBoundary = true;
    settings.preserveTopology = true;
    return settings;
}

SimplificationSettings SimplificationSettings::Balanced() {
    SimplificationSettings settings;
    settings.targetRatio = 0.5f;
    settings.maxError = 0.01f;
    settings.preserveBoundary = true;
    settings.preserveTopology = true;
    return settings;
}

SimplificationSettings SimplificationSettings::Quality() {
    SimplificationSettings settings;
    settings.targetRatio = 0.8f;
    settings.maxError = 0.001f;
    settings.preserveBoundary = true;
    settings.preserveTopology = true;
    return settings;
}

}
}