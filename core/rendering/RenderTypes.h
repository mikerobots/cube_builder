#pragma once

#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/Vector2f.h"
#include "../../foundation/math/CoordinateTypes.h"
#include <cstdint>
#include <vector>

namespace VoxelEditor {
namespace Rendering {

// Forward declarations
class RenderEngine;
class OpenGLRenderer;
class ShaderManager;
class RenderState;

// Common type aliases
using ShaderId = uint32_t;
using TextureId = uint32_t;
using BufferId = uint32_t;
using VertexBufferId = uint32_t;
using IndexBufferId = uint32_t;
using VertexArrayId = uint32_t;

// Invalid ID constant
constexpr uint32_t InvalidId = 0;

// Enumerations
enum class VertexAttribute {
    Position,
    Normal,
    TexCoord0,
    TexCoord1,
    Color,
    Tangent,
    Bitangent,
    BoneIndices,
    BoneWeights,
    Custom0,
    Custom1,
    Custom2,
    Custom3
};

enum class VertexAttributeType {
    Float,
    Int,
    UInt,
    Byte,
    UByte
};
enum class RenderMode {
    Solid,              // Solid surfaces only
    Wireframe,          // Wireframe only
    Combined,           // Solid + wireframe overlay
    Points              // Point cloud rendering
};

enum class BlendMode {
    Opaque,
    Alpha,
    Additive,
    Multiply
};

enum class CullMode {
    None,
    Front,
    Back
};

enum class BufferUsage {
    Static,
    Dynamic,
    Stream
};

// Clear flags for framebuffer clearing
enum class ClearFlags : uint32_t {
    COLOR = 0x01,
    DEPTH = 0x02,
    STENCIL = 0x04,
    All = COLOR | DEPTH | STENCIL
};

// Enable bitwise operations for ClearFlags
inline ClearFlags operator|(ClearFlags a, ClearFlags b) {
    return static_cast<ClearFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ClearFlags operator&(ClearFlags a, ClearFlags b) {
    return static_cast<ClearFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool operator&(ClearFlags a, uint32_t b) {
    return (static_cast<uint32_t>(a) & b) != 0;
}

enum class TextureFormat {
    RGB8,
    RGBA8,
    Depth24Stencil8,
    R32F,
    RGB32F,
    RGBA32F
};

enum class ColorFormat {
    RGB8,
    RGBA8,
    RGB16F,
    RGBA16F,
    RGB32F,
    RGBA32F
};

enum class DepthFormat {
    Depth16,
    Depth24,
    Depth32F,
    Depth24Stencil8,
    Depth32FStencil8
};

enum class ShaderType {
    Vertex,
    Fragment,
    Geometry,
    Compute
};

enum class PrimitiveType {
    Triangles,
    Lines,
    Points,
    TriangleStrip,
    LineStrip
};

enum class IndexType {
    UInt16,
    UInt32
};


// Vertex layout description
struct VertexLayout {
    struct Attribute {
        VertexAttribute attribute;
        int components; // 1-4
        VertexAttributeType type;
        bool normalized;
        size_t offset;
        
        Attribute(VertexAttribute attr, int comp, VertexAttributeType t, bool norm = false, size_t off = 0)
            : attribute(attr), components(comp), type(t), normalized(norm), offset(off) {}
    };
    
    std::vector<Attribute> attributes;
    size_t stride = 0;
    
    void addAttribute(VertexAttribute attr, int components, VertexAttributeType type, bool normalized = false) {
        attributes.emplace_back(attr, components, type, normalized, stride);
        size_t typeSize = 4; // Assume 4 bytes for now (float/int)
        if (type == VertexAttributeType::Byte || type == VertexAttributeType::UByte) {
            typeSize = 1;
        }
        stride += components * typeSize;
    }
    
    void clear() {
        attributes.clear();
        stride = 0;
    }
};

// Color structure
struct Color {
    float r, g, b, a;
    
    Color(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f)
        : r(r), g(g), b(b), a(a) {}
    
    static Color White() { return Color(1.0f, 1.0f, 1.0f, 1.0f); }
    static Color Black() { return Color(0.0f, 0.0f, 0.0f, 1.0f); }
    static Color Red() { return Color(1.0f, 0.0f, 0.0f, 1.0f); }
    static Color Green() { return Color(0.0f, 1.0f, 0.0f, 1.0f); }
    static Color Blue() { return Color(0.0f, 0.0f, 1.0f, 1.0f); }
    static Color Transparent() { return Color(0.0f, 0.0f, 0.0f, 0.0f); }
    
    // Comparison operators
    bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
    
    bool operator!=(const Color& other) const {
        return !(*this == other);
    }
};

// Vertex structure for rendering
struct Vertex {
    Math::WorldCoordinates position;
    Math::Vector3f normal;
    Math::Vector2f texCoords;
    Color color;
    
    Vertex() : position(Math::WorldCoordinates::zero()), normal(Math::Vector3f::UnitZ()), 
               texCoords(Math::Vector2f::zero()), color(Color::White()) {}
    
    Vertex(const Math::WorldCoordinates& pos, const Math::Vector3f& norm = Math::Vector3f::UnitZ(), 
           const Math::Vector2f& tex = Math::Vector2f::zero(), const Color& col = Color::White())
        : position(pos), normal(norm), texCoords(tex), color(col) {}
    
    // Backward compatibility constructor
    Vertex(const Math::Vector3f& pos, const Math::Vector3f& norm = Math::Vector3f::UnitZ(), 
           const Math::Vector2f& tex = Math::Vector2f::zero(), const Color& col = Color::White())
        : position(Math::WorldCoordinates(pos)), normal(norm), texCoords(tex), color(col) {}
};

// Mesh structure
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    BufferId vertexBuffer = 0;
    BufferId indexBuffer = 0;
    VertexArrayId vertexArray = 0;
    bool dirty = true;
    
    void clear() {
        vertices.clear();
        indices.clear();
        dirty = true;
    }
    
    bool isEmpty() const {
        return vertices.empty();
    }
    
    size_t getVertexCount() const {
        return vertices.size();
    }
    
    size_t getIndexCount() const {
        return indices.size();
    }
    
    size_t getTriangleCount() const {
        return indices.size() / 3;
    }
};

// Transform structure
struct Transform {
    Math::WorldCoordinates position = Math::WorldCoordinates::zero();
    Math::Vector3f rotation = Math::Vector3f::Zero(); // Euler angles in degrees
    Math::Vector3f scale = Math::Vector3f::One();
    
    Transform() = default;
    
    Transform(const Math::WorldCoordinates& pos, const Math::Vector3f& rot = Math::Vector3f::Zero(), 
              const Math::Vector3f& scl = Math::Vector3f::One())
        : position(pos), rotation(rot), scale(scl) {}
    
    // Backward compatibility constructor
    Transform(const Math::Vector3f& pos, const Math::Vector3f& rot = Math::Vector3f::Zero(), 
              const Math::Vector3f& scl = Math::Vector3f::One())
        : position(Math::WorldCoordinates(pos)), rotation(rot), scale(scl) {}
};

// Material structure
struct Material {
    Color albedo = Color::White();
    float metallic = 0.0f;
    float roughness = 0.5f;
    float emission = 0.0f;
    TextureId albedoTexture = 0;
    TextureId normalTexture = 0;
    TextureId metallicRoughnessTexture = 0;
    ShaderId shader = 0;
    
    // Rendering properties
    bool doubleSided = false;
    BlendMode blendMode = BlendMode::Opaque;
    CullMode cullMode = CullMode::Back;
    
    Material() = default;
    
    static Material createDefault() {
        Material mat;
        mat.albedo = Color::White();
        mat.metallic = 0.0f;
        mat.roughness = 0.5f;
        return mat;
    }
    
    static Material createVoxel(const Color& color) {
        Material mat;
        mat.albedo = color;
        mat.metallic = 0.0f;
        mat.roughness = 0.8f;
        return mat;
    }
    
    static Material createWireframe(const Color& color) {
        Material mat;
        mat.albedo = color;
        mat.metallic = 0.0f;
        mat.roughness = 1.0f;
        mat.blendMode = BlendMode::Alpha;
        return mat;
    }
};

}
}