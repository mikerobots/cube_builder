#pragma once

#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/Vector2f.h"
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

// Enumerations
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

enum class ClearFlags {
    Color = 1 << 0,
    Depth = 1 << 1,
    Stencil = 1 << 2,
    All = Color | Depth | Stencil
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
};

// Vertex structure for rendering
struct Vertex {
    Math::Vector3f position;
    Math::Vector3f normal;
    Math::Vector2f texCoords;
    Color color;
    
    Vertex() : position(Math::Vector3f::Zero()), normal(Math::Vector3f::UnitZ()), 
               texCoords(Math::Vector2f::zero()), color(Color::White()) {}
    
    Vertex(const Math::Vector3f& pos, const Math::Vector3f& norm = Math::Vector3f::UnitZ(), 
           const Math::Vector2f& tex = Math::Vector2f::zero(), const Color& col = Color::White())
        : position(pos), normal(norm), texCoords(tex), color(col) {}
};

// Mesh structure
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    BufferId vertexBuffer = 0;
    BufferId indexBuffer = 0;
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
    Math::Vector3f position = Math::Vector3f::Zero();
    Math::Vector3f rotation = Math::Vector3f::Zero(); // Euler angles in degrees
    Math::Vector3f scale = Math::Vector3f::One();
    
    Transform() = default;
    
    Transform(const Math::Vector3f& pos, const Math::Vector3f& rot = Math::Vector3f::Zero(), 
              const Math::Vector3f& scl = Math::Vector3f::One())
        : position(pos), rotation(rot), scale(scl) {}
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