#include "../include/visual_feedback/OutlineRenderer.h"
#include <algorithm>
#include <map>
#include <cstddef>

#ifdef __APPLE__
    #include <OpenGL/gl3.h>
#else
    #include <GL/gl.h>
#endif

namespace VoxelEditor {
namespace VisualFeedback {

OutlineRenderer::OutlineRenderer()
    : m_batchMode(false)
    , m_vertexBuffer(0)
    , m_indexBuffer(0)
    , m_outlineShader(0)
    , m_patternScale(1.0f)
    , m_patternOffset(0.0f)
    , m_animationTime(0.0f)
    , m_initialized(false) {
    
    // Delay OpenGL resource creation until first use
}

OutlineRenderer::~OutlineRenderer() {
    // Clean up GPU resources only if initialized
    if (m_initialized) {
        if (m_vertexBuffer) {
            glDeleteBuffers(1, &m_vertexBuffer);
        }
        if (m_indexBuffer) {
            glDeleteBuffers(1, &m_indexBuffer);
        }
        if (m_outlineShader) {
            glDeleteProgram(m_outlineShader);
        }
    }
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
    if (m_batches.empty()) return;
    
    ensureInitialized();
    
    // Enable line rendering settings
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    
    // Use outline shader
    glUseProgram(m_outlineShader);
    
    // Get uniform locations
    GLint mvpLoc = glGetUniformLocation(m_outlineShader, "mvpMatrix");
    GLint patternScaleLoc = glGetUniformLocation(m_outlineShader, "patternScale");
    GLint patternOffsetLoc = glGetUniformLocation(m_outlineShader, "patternOffset");
    GLint linePatternLoc = glGetUniformLocation(m_outlineShader, "linePattern");
    GLint animationTimeLoc = glGetUniformLocation(m_outlineShader, "animationTime");
    
    // Calculate MVP matrix
    Math::Matrix4f mvpMatrix = camera.getProjectionMatrix() * camera.getViewMatrix();
    
    // Set common uniforms
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvpMatrix.data());
    glUniform1f(animationTimeLoc, m_animationTime);
    
    // Setup vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    
    glEnableVertexAttribArray(0); // position
    glEnableVertexAttribArray(1); // color
    glEnableVertexAttribArray(2); // patternCoord
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(OutlineVertex),
                          (void*)offsetof(OutlineVertex, position));
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(OutlineVertex),
                          (void*)offsetof(OutlineVertex, color));
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(OutlineVertex),
                          (void*)offsetof(OutlineVertex, patternCoord));
    
    // Render each batch
    for (const auto& batch : m_batches) {
        if (batch.vertices.empty()) continue;
        
        updateBuffers(batch);
        
        // Set line width
        glLineWidth(batch.style.lineWidth);
        
        // Set pattern uniforms
        glUniform1f(patternScaleLoc, m_patternScale);
        glUniform1f(patternOffsetLoc, batch.style.animated ? m_patternOffset : 0.0f);
        glUniform1i(linePatternLoc, static_cast<int>(batch.style.pattern));
        
        // Draw lines
        glDrawElements(GL_LINES, static_cast<GLsizei>(batch.indices.size()),
                       GL_UNSIGNED_INT, nullptr);
    }
    
    // Cleanup
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);
    
    // Reset line width
    glLineWidth(1.0f);
    glDisable(GL_LINE_SMOOTH);
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

void OutlineRenderer::ensureInitialized() {
    if (!m_initialized) {
        createBuffers();
        m_initialized = true;
    }
}

void OutlineRenderer::createBuffers() {
    // Create vertex buffer
    glGenBuffers(1, &m_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    // Allocate initial buffer size (will be resized as needed)
    glBufferData(GL_ARRAY_BUFFER, sizeof(OutlineVertex) * 1024, nullptr, GL_DYNAMIC_DRAW);
    
    // Create index buffer
    glGenBuffers(1, &m_indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    // Allocate initial buffer size (will be resized as needed)
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * 2048, nullptr, GL_DYNAMIC_DRAW);
    
    // Create shader program for outline rendering
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec4 color;
        layout(location = 2) in float patternCoord;
        
        uniform mat4 mvpMatrix;
        
        out vec4 fragColor;
        out float fragPatternCoord;
        
        void main() {
            gl_Position = mvpMatrix * vec4(position, 1.0);
            fragColor = color;
            fragPatternCoord = patternCoord;
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec4 fragColor;
        in float fragPatternCoord;
        
        uniform float patternScale;
        uniform float patternOffset;
        uniform int linePattern; // 0=solid, 1=dashed, 2=dotted, 3=dashdot
        uniform float animationTime;
        
        out vec4 color;
        
        void main() {
            // Calculate pattern value based on pattern type
            float coord = (fragPatternCoord + patternOffset) * patternScale;
            float alpha = 1.0;
            
            if (linePattern == 1) { // Dashed
                alpha = step(0.5, fract(coord));
            } else if (linePattern == 2) { // Dotted
                alpha = step(0.7, fract(coord * 3.0));
            } else if (linePattern == 3) { // DashDot
                float phase = fract(coord * 0.5);
                alpha = (phase < 0.4) ? 1.0 : (phase < 0.5 || phase > 0.8) ? 0.0 : 1.0;
            }
            
            color = vec4(fragColor.rgb, fragColor.a * alpha);
        }
    )";
    
    // Compile vertex shader
    uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    // Compile fragment shader
    uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    // Create and link shader program
    m_outlineShader = glCreateProgram();
    glAttachShader(m_outlineShader, vertexShader);
    glAttachShader(m_outlineShader, fragmentShader);
    glLinkProgram(m_outlineShader);
    
    // Clean up shader objects
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Unbind buffers
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void OutlineRenderer::updateBuffers(const OutlineBatch& batch) {
    if (batch.vertices.empty()) return;
    
    // Update vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    
    // Check if we need to resize the buffer
    GLint bufferSize = 0;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
    size_t requiredSize = batch.vertices.size() * sizeof(OutlineVertex);
    
    if (requiredSize > static_cast<size_t>(bufferSize)) {
        // Resize buffer with some extra space
        glBufferData(GL_ARRAY_BUFFER, requiredSize * 2, nullptr, GL_DYNAMIC_DRAW);
    }
    
    // Upload vertex data
    glBufferSubData(GL_ARRAY_BUFFER, 0, requiredSize, batch.vertices.data());
    
    // Update index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    
    // Check if we need to resize the buffer
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
    size_t requiredIndexSize = batch.indices.size() * sizeof(uint32_t);
    
    if (requiredIndexSize > static_cast<size_t>(bufferSize)) {
        // Resize buffer with some extra space
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, requiredIndexSize * 2, nullptr, GL_DYNAMIC_DRAW);
    }
    
    // Upload index data
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, requiredIndexSize, batch.indices.data());
    
    // Unbind buffers
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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
    // Map to track edge usage count
    std::map<std::pair<Math::Vector3f, Math::Vector3f>, uint32_t> edgeCount;
    
    // Helper to create ordered edge key (ensures consistent ordering)
    auto makeEdgeKey = [](const Math::Vector3f& a, const Math::Vector3f& b) {
        if (a.x < b.x || (a.x == b.x && a.y < b.y) || (a.x == b.x && a.y == b.y && a.z < b.z)) {
            return std::make_pair(a, b);
        }
        return std::make_pair(b, a);
    };
    
    // Process each voxel
    for (VoxelId voxelId : voxels) {
        // Extract position and resolution from VoxelId
        // VoxelId encoding: high 32 bits = position hash, low 32 bits = resolution + flags
        uint32_t positionHash = static_cast<uint32_t>(voxelId >> 32);
        uint32_t resolutionData = static_cast<uint32_t>(voxelId & 0xFFFFFFFF);
        
        // Decode position (simplified - assumes voxel positions are small enough)
        int32_t x = static_cast<int32_t>((positionHash >> 20) & 0x3FF) - 512;
        int32_t y = static_cast<int32_t>((positionHash >> 10) & 0x3FF) - 512;
        int32_t z = static_cast<int32_t>(positionHash & 0x3FF) - 512;
        
        // Get resolution
        VoxelData::VoxelResolution resolution = static_cast<VoxelData::VoxelResolution>(resolutionData & 0xF);
        float voxelSize = VoxelData::getVoxelSize(resolution);
        
        Math::Vector3f basePos(x * voxelSize, y * voxelSize, z * voxelSize);
        
        // Define the 8 corners of the voxel
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
        
        // Define the 12 edges of a cube
        std::pair<int, int> edgeIndices[12] = {
            // Bottom face
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            // Top face
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            // Vertical edges
            {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };
        
        // Count each edge
        for (const auto& edge : edgeIndices) {
            auto key = makeEdgeKey(corners[edge.first], corners[edge.second]);
            edgeCount[key]++;
        }
    }
    
    // Extract edges that appear exactly once (external edges)
    std::vector<Edge> externalEdges;
    for (const auto& pair : edgeCount) {
        if (pair.second == 1) {
            Edge edge;
            edge.start = pair.first.first;
            edge.end = pair.first.second;
            edge.faceCount = 1;
            externalEdges.push_back(edge);
        }
    }
    
    return externalEdges;
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
    
    if (voxels.empty()) return std::vector<Math::Vector3f>();
    
    // Find all unique edges in the group
    auto uniqueEdges = findUniqueEdges(voxels);
    
    // Convert edge keys to line list
    std::vector<Math::Vector3f> outline;
    outline.reserve(uniqueEdges.size() * 2);
    
    for (const auto& edge : uniqueEdges) {
        outline.push_back(edge.start);
        outline.push_back(edge.end);
    }
    
    return outline;
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
    
    // Process each voxel
    for (VoxelId voxelId : voxels) {
        // Extract position and resolution from VoxelId
        uint32_t positionHash = static_cast<uint32_t>(voxelId >> 32);
        uint32_t resolutionData = static_cast<uint32_t>(voxelId & 0xFFFFFFFF);
        
        // Decode position
        int32_t x = static_cast<int32_t>((positionHash >> 20) & 0x3FF) - 512;
        int32_t y = static_cast<int32_t>((positionHash >> 10) & 0x3FF) - 512;
        int32_t z = static_cast<int32_t>(positionHash & 0x3FF) - 512;
        
        // Get resolution
        VoxelData::VoxelResolution resolution = static_cast<VoxelData::VoxelResolution>(resolutionData & 0xF);
        float voxelSize = getVoxelSize(resolution);
        
        Math::Vector3f basePos(x * voxelSize, y * voxelSize, z * voxelSize);
        
        // Define the 8 corners of the voxel
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
        
        // Define the 12 edges of a cube
        std::pair<int, int> edgeIndices[12] = {
            // Bottom face
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            // Top face
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            // Vertical edges
            {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };
        
        // Add each edge to the map
        for (const auto& edge : edgeIndices) {
            EdgeKey key;
            // Ensure consistent edge ordering
            if (corners[edge.first].x < corners[edge.second].x ||
                (corners[edge.first].x == corners[edge.second].x && corners[edge.first].y < corners[edge.second].y) ||
                (corners[edge.first].x == corners[edge.second].x && corners[edge.first].y == corners[edge.second].y && 
                 corners[edge.first].z < corners[edge.second].z)) {
                key.start = corners[edge.first];
                key.end = corners[edge.second];
            } else {
                key.start = corners[edge.second];
                key.end = corners[edge.first];
            }
            edgeCount[key]++;
        }
    }
    
    // Extract edges that appear exactly once (external edges)
    std::vector<EdgeKey> uniqueEdges;
    for (const auto& pair : edgeCount) {
        if (pair.second == 1) {
            uniqueEdges.push_back(pair.first);
        }
    }
    
    return uniqueEdges;
}

} // namespace VisualFeedback
} // namespace VoxelEditor