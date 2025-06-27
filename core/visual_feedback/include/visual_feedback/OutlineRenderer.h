#pragma once

#include <vector>
#include <memory>

#include "FeedbackTypes.h"
#include "Camera.h"

namespace VoxelEditor {
namespace VisualFeedback {

class OutlineRenderer {
public:
    OutlineRenderer();
    ~OutlineRenderer();
    
    // Voxel outlines
    void renderVoxelOutline(const Math::Vector3i& position, 
                           VoxelData::VoxelResolution resolution, 
                           const OutlineStyle& style);
    
    // Box outlines
    void renderBoxOutline(const Math::BoundingBox& box, const OutlineStyle& style);
    
    // Group outlines
    void renderGroupOutline(const std::vector<VoxelId>& voxels, const OutlineStyle& style);
    
    // Custom outlines
    void renderCustomOutline(const std::vector<Math::Vector3f>& points, 
                            const OutlineStyle& style, bool closed = true);
    
    // Batch rendering
    void beginBatch();
    void endBatch();
    void renderBatch(const Camera::Camera& camera);
    void clearBatch();
    
    // Line patterns
    void setPatternScale(float scale) { m_patternScale = scale; }
    void setPatternOffset(float offset) { m_patternOffset = offset; }
    
    // Animation
    void update(float deltaTime);
    
private:
    struct OutlineVertex {
        Math::Vector3f position;
        Rendering::Color color;
        float patternCoord;
    };
    
    struct OutlineBatch {
        std::vector<OutlineVertex> vertices;
        std::vector<uint32_t> indices;
        OutlineStyle style;
    };
    
    std::vector<OutlineBatch> m_batches;
    OutlineBatch m_currentBatch;
    bool m_batchMode;
    
    uint32_t m_vertexArray;  // VAO
    uint32_t m_vertexBuffer;
    uint32_t m_indexBuffer;
    uint32_t m_outlineShader;
    
    float m_patternScale;
    float m_patternOffset;
    float m_animationTime;
    bool m_initialized;
    
    // Line generation
    void addLineSegment(const Math::Vector3f& start, const Math::Vector3f& end, 
                       const Rendering::Color& color, float patternStart = 0.0f);
    
    void addBox(const Math::BoundingBox& box, const Rendering::Color& color);
    
    void addVoxelEdges(const Math::Vector3i& position, 
                      VoxelData::VoxelResolution resolution, 
                      const Rendering::Color& color);
    
    // Group outline generation
    std::vector<Math::Vector3f> generateGroupOutline(const std::vector<VoxelId>& voxels);
    
    // Buffer management
    void createBuffers();
    void updateBuffers(const OutlineBatch& batch);
    void ensureInitialized();
    
    // Pattern calculation
    float calculatePatternCoordinate(float distance, LinePattern pattern, 
                                   float scale) const;
    
    // Edge detection for groups
    struct Edge {
        Math::Vector3f start;
        Math::Vector3f end;
        uint32_t faceCount;
    };
    
    std::vector<Edge> findExternalEdges(const std::vector<VoxelId>& voxels);
    void removeInternalEdges(std::vector<Edge>& edges);
    std::vector<Math::Vector3f> edgesToLineList(const std::vector<Edge>& edges);
};

// Utility class for generating voxel outlines
class VoxelOutlineGenerator {
public:
    static std::vector<Math::Vector3f> generateVoxelEdges(
        const Math::Vector3i& position, 
        VoxelData::VoxelResolution resolution);
    
    static std::vector<Math::Vector3f> generateGroupOutline(
        const std::vector<VoxelId>& voxels);
    
    // static std::vector<Math::Vector3f> generateSelectionOutline(
    //     const Selection::SelectionSet& selection);
    
private:
    static float getVoxelSize(VoxelData::VoxelResolution resolution);
    static Math::BoundingBox getVoxelBounds(const VoxelId& voxel);
    
    struct EdgeKey {
        Math::Vector3f start;
        Math::Vector3f end;
        
        bool operator<(const EdgeKey& other) const;
    };
    
    static std::vector<EdgeKey> findUniqueEdges(const std::vector<VoxelId>& voxels);
};

} // namespace VisualFeedback
} // namespace VoxelEditor