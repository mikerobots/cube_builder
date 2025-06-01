#include "../include/visual_feedback/OutlineRenderer.h"
#include <algorithm>
#include <map>

namespace VoxelEditor {
namespace VisualFeedback {

OutlineRenderer::OutlineRenderer()
    : m_batchMode(false)
    , m_vertexBuffer(0)
    , m_indexBuffer(0)
    , m_outlineShader(0)
    , m_patternScale(1.0f)
    , m_patternOffset(0.0f)
    , m_animationTime(0.0f) {
    
    createBuffers();
}

OutlineRenderer::~OutlineRenderer() {
    // TODO: Clean up GPU resources
}

void OutlineRenderer::renderVoxelOutline(const Math::Vector3i& position, 
                                        VoxelData::VoxelResolution resolution, 
                                        const OutlineStyle& style) {
    if (m_batchMode) {
        addVoxelEdges(position, resolution, style.color);
        m_currentBatch.style = style;
    } else {
        beginBatch();
        addVoxelEdges(position, resolution, style.color);
        m_currentBatch.style = style;
        endBatch();
    }
}

void OutlineRenderer::renderBoxOutline(const Math::BoundingBox& box, const OutlineStyle& style) {
    if (m_batchMode) {
        addBox(box, style.color);
        m_currentBatch.style = style;
    } else {
        beginBatch();
        addBox(box, style.color);
        m_currentBatch.style = style;
        endBatch();
    }
}

void OutlineRenderer::renderGroupOutline(const std::vector<VoxelId>& voxels, 
                                        const OutlineStyle& style) {
    if (voxels.empty()) return;
    
    auto outline = generateGroupOutline(voxels);
    renderCustomOutline(outline, style);
}

void OutlineRenderer::renderCustomOutline(const std::vector<Math::Vector3f>& points, 
                                         const OutlineStyle& style, bool closed) {
    if (points.size() < 2) return;
    
    if (m_batchMode) {
        for (size_t i = 0; i < points.size() - 1; ++i) {
            addLineSegment(points[i], points[i + 1], style.color);
        }
        
        if (closed && points.size() > 2) {
            addLineSegment(points.back(), points.front(), style.color);
        }
        
        m_currentBatch.style = style;
    } else {
        beginBatch();
        
        for (size_t i = 0; i < points.size() - 1; ++i) {
            addLineSegment(points[i], points[i + 1], style.color);
        }
        
        if (closed && points.size() > 2) {
            addLineSegment(points.back(), points.front(), style.color);
        }
        
        m_currentBatch.style = style;
        endBatch();
    }
}

void OutlineRenderer::beginBatch() {
    m_batchMode = true;
    m_currentBatch.vertices.clear();
    m_currentBatch.indices.clear();
}

void OutlineRenderer::endBatch() {
    if (!m_currentBatch.vertices.empty()) {
        m_batches.push_back(std::move(m_currentBatch));
    }
    
    m_currentBatch = OutlineBatch();
    m_batchMode = false;
}

void OutlineRenderer::renderBatch(const Camera::Camera& camera) {
    for (const auto& batch : m_batches) {
        updateBuffers(batch);
        
        // TODO: Set shader uniforms based on batch.style
        // - Line width
        // - Pattern settings
        // - Color
        // - Animation parameters
        
        // TODO: Render the batch
    }
}

void OutlineRenderer::clearBatch() {
    m_batches.clear();
    m_currentBatch.vertices.clear();
    m_currentBatch.indices.clear();
}

void OutlineRenderer::update(float deltaTime) {
    m_animationTime += deltaTime;
    
    // Update pattern offset for animated outlines
    for (auto& batch : m_batches) {
        if (batch.style.animated) {
            m_patternOffset += deltaTime * batch.style.animationSpeed;
        }
    }
}

void OutlineRenderer::addLineSegment(const Math::Vector3f& start, const Math::Vector3f& end, 
                                    const Rendering::Color& color, float patternStart) {
    uint32_t startIndex = static_cast<uint32_t>(m_currentBatch.vertices.size());
    
    OutlineVertex startVertex;
    startVertex.position = start;
    startVertex.color = color;
    startVertex.patternCoord = patternStart;
    
    OutlineVertex endVertex;
    endVertex.position = end;
    endVertex.color = color;
    endVertex.patternCoord = patternStart + (end - start).length();
    
    m_currentBatch.vertices.push_back(startVertex);
    m_currentBatch.vertices.push_back(endVertex);
    
    m_currentBatch.indices.push_back(startIndex);
    m_currentBatch.indices.push_back(startIndex + 1);
}

void OutlineRenderer::addBox(const Math::BoundingBox& box, const Rendering::Color& color) {
    // 12 edges of a box
    Math::Vector3f corners[8] = {
        Math::Vector3f(box.min.x, box.min.y, box.min.z), // 0
        Math::Vector3f(box.max.x, box.min.y, box.min.z), // 1
        Math::Vector3f(box.max.x, box.max.y, box.min.z), // 2
        Math::Vector3f(box.min.x, box.max.y, box.min.z), // 3
        Math::Vector3f(box.min.x, box.min.y, box.max.z), // 4
        Math::Vector3f(box.max.x, box.min.y, box.max.z), // 5
        Math::Vector3f(box.max.x, box.max.y, box.max.z), // 6
        Math::Vector3f(box.min.x, box.max.y, box.max.z)  // 7
    };
    
    // Bottom face
    addLineSegment(corners[0], corners[1], color);
    addLineSegment(corners[1], corners[2], color);
    addLineSegment(corners[2], corners[3], color);
    addLineSegment(corners[3], corners[0], color);
    
    // Top face
    addLineSegment(corners[4], corners[5], color);
    addLineSegment(corners[5], corners[6], color);
    addLineSegment(corners[6], corners[7], color);
    addLineSegment(corners[7], corners[4], color);
    
    // Vertical edges
    addLineSegment(corners[0], corners[4], color);
    addLineSegment(corners[1], corners[5], color);
    addLineSegment(corners[2], corners[6], color);
    addLineSegment(corners[3], corners[7], color);
}

void OutlineRenderer::addVoxelEdges(const Math::Vector3i& position, 
                                   VoxelData::VoxelResolution resolution, 
                                   const Rendering::Color& color) {
    float voxelSize = VoxelData::getVoxelSize(resolution);
    
    Math::Vector3f basePos(
        position.x * voxelSize,
        position.y * voxelSize,
        position.z * voxelSize
    );
    
    Math::BoundingBox voxelBox(basePos, basePos + Math::Vector3f(voxelSize, voxelSize, voxelSize));
    addBox(voxelBox, color);
}

std::vector<Math::Vector3f> OutlineRenderer::generateGroupOutline(const std::vector<VoxelId>& voxels) {
    // Find external edges of the group
    auto edges = findExternalEdges(voxels);
    removeInternalEdges(edges);
    return edgesToLineList(edges);
}

void OutlineRenderer::createBuffers() {
    // TODO: Create GPU buffers for vertices and indices
}

void OutlineRenderer::updateBuffers(const OutlineBatch& batch) {
    // TODO: Update GPU buffers with batch data
}

float OutlineRenderer::calculatePatternCoordinate(float distance, LinePattern pattern, 
                                                 float scale) const {
    switch (pattern) {
        case LinePattern::Solid:
            return 0.0f;
        case LinePattern::Dashed:
            return distance * scale;
        case LinePattern::Dotted:
            return distance * scale * 3.0f;
        case LinePattern::DashDot:
            return distance * scale * 1.5f;
    }
    return 0.0f;
}

std::vector<OutlineRenderer::Edge> OutlineRenderer::findExternalEdges(const std::vector<VoxelId>& voxels) {
    std::vector<Edge> edges;
    
    // TODO: Implement edge detection algorithm
    // This would involve:
    // 1. Converting VoxelIds to positions
    // 2. Finding all edges of all voxels
    // 3. Counting how many faces share each edge
    // 4. Keeping only edges shared by exactly one face (external edges)
    
    return edges;
}

void OutlineRenderer::removeInternalEdges(std::vector<Edge>& edges) {
    // Remove edges that are shared by multiple faces (internal edges)
    edges.erase(
        std::remove_if(edges.begin(), edges.end(),
            [](const Edge& edge) { return edge.faceCount > 1; }),
        edges.end()
    );
}

std::vector<Math::Vector3f> OutlineRenderer::edgesToLineList(const std::vector<Edge>& edges) {
    std::vector<Math::Vector3f> lineList;
    
    for (const auto& edge : edges) {
        lineList.push_back(edge.start);
        lineList.push_back(edge.end);
    }
    
    return lineList;
}

// VoxelOutlineGenerator implementation
std::vector<Math::Vector3f> VoxelOutlineGenerator::generateVoxelEdges(
    const Math::Vector3i& position, 
    VoxelData::VoxelResolution resolution) {
    
    std::vector<Math::Vector3f> edges;
    float voxelSize = getVoxelSize(resolution);
    
    Math::Vector3f basePos(
        position.x * voxelSize,
        position.y * voxelSize,
        position.z * voxelSize
    );
    
    // Generate 12 edges of a cube
    Math::Vector3f corners[8] = {
        basePos + Math::Vector3f(0, 0, 0),
        basePos + Math::Vector3f(voxelSize, 0, 0),
        basePos + Math::Vector3f(voxelSize, voxelSize, 0),
        basePos + Math::Vector3f(0, voxelSize, 0),
        basePos + Math::Vector3f(0, 0, voxelSize),
        basePos + Math::Vector3f(voxelSize, 0, voxelSize),
        basePos + Math::Vector3f(voxelSize, voxelSize, voxelSize),
        basePos + Math::Vector3f(0, voxelSize, voxelSize)
    };
    
    // Bottom face edges
    edges.push_back(corners[0]); edges.push_back(corners[1]);
    edges.push_back(corners[1]); edges.push_back(corners[2]);
    edges.push_back(corners[2]); edges.push_back(corners[3]);
    edges.push_back(corners[3]); edges.push_back(corners[0]);
    
    // Top face edges
    edges.push_back(corners[4]); edges.push_back(corners[5]);
    edges.push_back(corners[5]); edges.push_back(corners[6]);
    edges.push_back(corners[6]); edges.push_back(corners[7]);
    edges.push_back(corners[7]); edges.push_back(corners[4]);
    
    // Vertical edges
    edges.push_back(corners[0]); edges.push_back(corners[4]);
    edges.push_back(corners[1]); edges.push_back(corners[5]);
    edges.push_back(corners[2]); edges.push_back(corners[6]);
    edges.push_back(corners[3]); edges.push_back(corners[7]);
    
    return edges;
}

std::vector<Math::Vector3f> VoxelOutlineGenerator::generateGroupOutline(
    const std::vector<VoxelId>& voxels) {
    
    // TODO: Implement group outline generation
    // This is a complex algorithm that finds the external boundary of a set of voxels
    return std::vector<Math::Vector3f>();
}

// std::vector<Math::Vector3f> VoxelOutlineGenerator::generateSelectionOutline(
//     const Selection::SelectionSet& selection) {
//     
//     std::vector<VoxelId> voxelIds;
//     // TODO: Convert selection to voxel IDs
//     return generateGroupOutline(voxelIds);
// }

float VoxelOutlineGenerator::getVoxelSize(VoxelData::VoxelResolution resolution) {
    return VoxelData::getVoxelSize(resolution);
}

Math::BoundingBox VoxelOutlineGenerator::getVoxelBounds(const VoxelId& voxel) {
    // TODO: Extract position and resolution from VoxelId
    // For now, return a unit box
    return Math::BoundingBox(Math::Vector3f(0, 0, 0), Math::Vector3f(1, 1, 1));
}

bool VoxelOutlineGenerator::EdgeKey::operator<(const EdgeKey& other) const {
    if (start.x != other.start.x) return start.x < other.start.x;
    if (start.y != other.start.y) return start.y < other.start.y;
    if (start.z != other.start.z) return start.z < other.start.z;
    if (end.x != other.end.x) return end.x < other.end.x;
    if (end.y != other.end.y) return end.y < other.end.y;
    return end.z < other.end.z;
}

std::vector<VoxelOutlineGenerator::EdgeKey> VoxelOutlineGenerator::findUniqueEdges(
    const std::vector<VoxelId>& voxels) {
    
    std::map<EdgeKey, int> edgeCount;
    
    // TODO: For each voxel, add its 12 edges to the map
    // Increment count for each edge
    
    std::vector<EdgeKey> uniqueEdges;
    for (const auto& pair : edgeCount) {
        if (pair.second == 1) { // Edge appears only once (external edge)
            uniqueEdges.push_back(pair.first);
        }
    }
    
    return uniqueEdges;
}

} // namespace VisualFeedback
} // namespace VoxelEditor