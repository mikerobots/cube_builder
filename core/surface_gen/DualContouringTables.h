#pragma once

#include <array>
#include <cstdint>

namespace VoxelEditor {
namespace SurfaceGen {

// Pre-computed tables for dual contouring optimization
namespace DualContouringTables {

// Edge connection table - which vertices form each edge
constexpr std::array<std::pair<uint8_t, uint8_t>, 12> EDGE_CONNECTIONS = {{
    {0, 1}, {1, 3}, {3, 2}, {2, 0},  // Bottom face edges
    {4, 5}, {5, 7}, {7, 6}, {6, 4},  // Top face edges  
    {0, 4}, {1, 5}, {3, 7}, {2, 6}   // Vertical edges
}};

// Edge sharing table - which cells share each edge
// Each edge can be shared by up to 4 cells
struct EdgeSharing {
    uint8_t count;  // Number of cells sharing this edge
    std::array<std::array<int8_t, 3>, 4> offsets;  // Cell offsets
    uint8_t localEdgeIndices[4];  // Local edge index in each cell
};

// Pre-computed edge sharing information
constexpr std::array<EdgeSharing, 12> EDGE_SHARING = {{
    // Edge 0: (0,0,0)-(1,0,0) - bottom face, parallel to X
    {4, {{{0,0,0}, {0,-1,0}, {0,0,-1}, {0,-1,-1}}}, {0, 2, 4, 6}},
    // Edge 1: (1,0,0)-(1,1,0) - bottom face, parallel to Y
    {4, {{{0,0,0}, {1,0,0}, {0,0,-1}, {1,0,-1}}}, {1, 3, 5, 7}},
    // ... (remaining edges omitted for brevity)
}};

// Cell configuration lookup table
// Given 8 vertex inside/outside states, determine which edges have intersections
constexpr uint16_t EDGE_INTERSECTION_TABLE[256] = {
    0x000, 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
    // ... (full 256-entry table for all vertex configurations)
};

// Vertex position offsets for cube corners
constexpr std::array<std::array<int8_t, 3>, 8> VERTEX_OFFSETS = {{
    {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
    {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}
}};

// Quick lookup for opposite vertex across edge
constexpr uint8_t EDGE_OPPOSITE_VERTEX[12][2] = {
    {2, 3}, {0, 2}, {1, 3}, {0, 1},  // Bottom edges
    {6, 7}, {4, 6}, {5, 7}, {4, 5},  // Top edges
    {1, 5}, {0, 4}, {2, 6}, {3, 7}   // Vertical edges
};

// Optimized gradient stencil offsets
struct GradientStencil {
    std::array<int8_t, 3> offset;
    float weight;
};

// Central difference stencil for gradient computation
constexpr std::array<GradientStencil, 6> GRADIENT_STENCIL = {{
    {{ 1,  0,  0},  0.5f},   // +X
    {{-1,  0,  0}, -0.5f},   // -X
    {{ 0,  1,  0},  0.5f},   // +Y
    {{ 0, -1,  0}, -0.5f},   // -Y
    {{ 0,  0,  1},  0.5f},   // +Z
    {{ 0,  0, -1}, -0.5f}    // -Z
}};

// Face adjacency for quad generation
constexpr std::array<std::array<uint8_t, 4>, 6> FACE_VERTICES = {{
    {0, 2, 3, 1},  // Bottom (-Y)
    {4, 5, 7, 6},  // Top (+Y)
    {0, 1, 5, 4},  // Front (-Z)
    {2, 6, 7, 3},  // Back (+Z)
    {0, 4, 6, 2},  // Left (-X)
    {1, 3, 7, 5}   // Right (+X)
}};

// Quick test for whether a cell configuration has any surface
inline bool hasSurface(uint8_t vertexMask) {
    return vertexMask != 0x00 && vertexMask != 0xFF;
}

// Get edge intersection mask for vertex configuration
inline uint16_t getEdgeIntersections(uint8_t vertexMask) {
    return EDGE_INTERSECTION_TABLE[vertexMask];
}

// Check if specific edge has intersection
inline bool hasEdgeIntersection(uint16_t edgeMask, int edgeIndex) {
    return (edgeMask & (1 << edgeIndex)) != 0;
}

} // namespace DualContouringTables
} // namespace SurfaceGen
} // namespace VoxelEditor